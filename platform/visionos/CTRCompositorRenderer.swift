#if os(visionOS)

import ARKit
import CompositorServices
import Darwin
import Foundation
import Metal
import os
import QuartzCore
import simd
import _CompositorServices_SwiftUI

enum CTRImmersiveMode: Int32, Sendable {
    case portal = 1
    case cockpit = 2
}

@MainActor
struct CTRCompositorLayerConfiguration: _CompositorServices_SwiftUI.CompositorLayerConfiguration {
    func makeConfiguration(
        capabilities: LayerRenderer.Capabilities,
        configuration: inout LayerRenderer.Configuration
    ) {
        // CTR composites one inexpensive quad per eye. Dedicated, unfoveated
        // targets avoid applying a layered left-eye rasterization map to the
        // explicitly rendered right-eye slice.
        let layouts = capabilities.supportedLayouts(options: [])
        if layouts.contains(.dedicated) {
            configuration.layout = .dedicated
        } else if layouts.contains(.layered) {
            configuration.layout = .layered
        }
        configuration.colorFormat = .bgra8Unorm_srgb
        configuration.depthFormat = .depth32Float
        configuration.isFoveationEnabled = false
    }
}

extension Notification.Name {
    static let ctrImmersiveSpaceDidClose = Notification.Name("CTRImmersiveSpaceDidClose")
    static let ctrImmersiveSpaceDidFail = Notification.Name("CTRImmersiveSpaceDidFail")
}

private let ctrCompositorLog = Logger(
    subsystem: Bundle.main.bundleIdentifier ?? "io.github.chrissotraidis.ctrpad.vision",
    category: "VisionCompositor"
)

private final class CTRRenderTaskExecutor: TaskExecutor {
    private let queue = DispatchQueue(label: "CTRCompositorRenderQueue", qos: .userInteractive)

    func enqueue(_ job: UnownedJob) {
        queue.async {
            job.runSynchronously(on: self.asUnownedSerialExecutor())
        }
    }

    nonisolated func asUnownedSerialExecutor() -> UnownedTaskExecutor {
        UnownedTaskExecutor(ordinary: self)
    }

    static let shared = CTRRenderTaskExecutor()
}

extension LayerRenderer.Clock.Instant {
    fileprivate var ctrTimeInterval: TimeInterval {
        let components = LayerRenderer.Clock.Instant.epoch.duration(to: self).components
        let nanoseconds = TimeInterval(components.attoseconds / 1_000_000_000)
        return TimeInterval(components.seconds) + nanoseconds / TimeInterval(NSEC_PER_SEC)
    }
}

enum CTRCompositorRenderer {
    @MainActor
    static func startRenderLoop(_ layerRenderer: LayerRenderer, mode: CTRImmersiveMode) {
        ctrCompositorLog.notice("[CTR Compositor] layer created mode=\(mode.rawValue)")
        NativeVision_SetMode(mode.rawValue)
        NativeVision_ResetTracking()
        Task(executorPreference: CTRRenderTaskExecutor.shared) {
            guard let renderer = CTRLayerRenderer(layerRenderer: layerRenderer, mode: mode) else {
                ctrCompositorLog.error("[CTR Compositor] renderer setup failed mode=\(mode.rawValue)")
                NativeVision_SetMode(0)
                await MainActor.run {
                    NotificationCenter.default.post(name: .ctrImmersiveSpaceDidFail, object: nil)
                }
                return
            }
            guard await renderer.startTracking() else {
                ctrCompositorLog.error("[CTR Compositor] tracking startup failed mode=\(mode.rawValue)")
                NativeVision_SetMode(0)
                await MainActor.run {
                    NotificationCenter.default.post(name: .ctrImmersiveSpaceDidFail, object: nil)
                }
                return
            }
            renderer.run()
        }
    }
}

private final class CTRLayerRenderer: @unchecked Sendable {
    private struct EyeUniform {
        var modelViewProjection: simd_float4x4
        var halfSize: SIMD2<Float>
        var mode: UInt32
        var eye: UInt32
    }

    private let layerRenderer: LayerRenderer
    private let mode: CTRImmersiveMode
    private let clock = LayerRenderer.Clock()
    private let arSession = ARKitSession()
    private let worldTracking = WorldTrackingProvider()
    private let commandQueue: MTLCommandQueue
    private var pipeline: MTLRenderPipelineState?
    private var depthState: MTLDepthStencilState?
    private var sampler: MTLSamplerState?
    private var pipelineColorFormat: MTLPixelFormat = .invalid
    private var pipelineDepthFormat: MTLPixelFormat = .invalid
    private var trackingBasis: simd_float4x4?
    private var cockpitReference: simd_float4x4?
    private var portalTransform: simd_float4x4?
    private var lastAnchor: DeviceAnchor?
    private var didLogFirstFrame = false

    init?(layerRenderer: LayerRenderer, mode: CTRImmersiveMode) {
        self.layerRenderer = layerRenderer
        self.mode = mode
        guard let queue = layerRenderer.device.makeCommandQueue() else { return nil }
        commandQueue = queue
    }

    func startTracking() async -> Bool {
        guard WorldTrackingProvider.isSupported else {
            ctrCompositorLog.error("[CTR Compositor] world tracking unsupported")
            return false
        }
#if !targetEnvironment(simulator)
        let results = await arSession.requestAuthorization(
            for: Array(WorldTrackingProvider.requiredAuthorizations)
        )
        for (authorization, status) in results {
            ctrCompositorLog.notice(
                "[CTR Compositor] authorization \(String(describing: authorization), privacy: .public)=\(String(describing: status), privacy: .public)"
            )
        }
        if results.values.contains(.denied) {
            ctrCompositorLog.error("[CTR Compositor] world tracking authorization denied")
            return false
        }
#endif
        do {
            try await arSession.run([worldTracking])
        } catch {
            NSLog("CTR world tracking failed: %@", error.localizedDescription)
            return false
        }

        for _ in 0..<200 {
            if worldTracking.state == .running,
               worldTracking.queryDeviceAnchor(atTimestamp: CACurrentMediaTime()) != nil {
                ctrCompositorLog.notice("[CTR Compositor] world tracking ready mode=\(self.mode.rawValue)")
                return true
            }
            try? await Task.sleep(nanoseconds: 10_000_000)
        }
        ctrCompositorLog.error(
            "[CTR Compositor] world tracking timed out state=\(String(describing: self.worldTracking.state), privacy: .public)"
        )
        return false
    }

    func run() {
        ctrCompositorLog.notice("[CTR Compositor] render loop entered mode=\(self.mode.rawValue)")
        var isRendering = true
        while isRendering {
            autoreleasepool {
                switch layerRenderer.state {
                case .paused:
                    layerRenderer.waitUntilRunning()
                case .running:
                    renderFrame()
                case .invalidated:
                    isRendering = false
                @unknown default:
                    isRendering = false
                }
            }
        }

        if NativeVision_GetMode() == mode.rawValue {
            NativeVision_SetMode(0)
            NativeVision_ResetTracking()
        }
        ctrCompositorLog.notice("[CTR Compositor] render loop closed mode=\(self.mode.rawValue)")
        DispatchQueue.main.async {
            NotificationCenter.default.post(name: .ctrImmersiveSpaceDidClose, object: nil)
        }
    }

    private func renderFrame() {
        guard let frame = layerRenderer.queryNextFrame(),
              let timing = frame.predictTiming() else {
            return
        }

        frame.startUpdate()
        frame.endUpdate()
        clock.wait(until: timing.optimalInputTime)
        guard layerRenderer.state == .running else { return }

        frame.startSubmission()
        let drawables: [LayerRenderer.Drawable]
        if #available(visionOS 26.0, *) {
            drawables = frame.queryDrawables()
        } else if let drawable = frame.queryDrawable() {
            drawables = [drawable]
        } else {
            drawables = []
        }
        guard !drawables.isEmpty else {
            frame.endSubmission()
            return
        }

        guard let commandBuffer = commandQueue.makeCommandBuffer() else {
            submitEmpty(drawables: drawables, frame: frame)
            return
        }

        let anchor = queryAnchor(for: drawables[0])
        if let anchor {
            for drawable in drawables {
                drawable.deviceAnchor = anchor
            }
            updateTracking(anchor: anchor, drawables: drawables)
        }

        var eyeBaseIndex = 0
        for drawable in drawables {
            encode(
                drawable: drawable,
                anchor: anchor,
                eyeBaseIndex: eyeBaseIndex,
                commandBuffer: commandBuffer
            )
            drawable.encodePresent(commandBuffer: commandBuffer)
            eyeBaseIndex += drawable.views.count
        }
        commandBuffer.commit()
        frame.endSubmission()

        if !didLogFirstFrame {
            didLogFirstFrame = true
            let viewCount = drawables.reduce(0) { $0 + $1.views.count }
            let source = CTRVisionFrameHub.currentSceneTexture()
            ctrCompositorLog.notice(
                "[CTR Compositor] first frame mode=\(self.mode.rawValue) drawables=\(drawables.count) views=\(viewCount) source=\(source?.width ?? 0)x\(source?.height ?? 0)"
            )
        }
    }

    private func submitEmpty(drawables: [LayerRenderer.Drawable], frame: LayerRenderer.Frame) {
        if let commandBuffer = commandQueue.makeCommandBuffer() {
            for drawable in drawables {
                drawable.encodePresent(commandBuffer: commandBuffer)
            }
            commandBuffer.commit()
        }
        frame.endSubmission()
    }

    private func queryAnchor(for drawable: LayerRenderer.Drawable) -> DeviceAnchor? {
        if worldTracking.state == .running {
            if let predicted = worldTracking.queryDeviceAnchor(
                atTimestamp: drawable.frameTiming.presentationTime.ctrTimeInterval
            ), predicted.trackingState == .tracked {
                lastAnchor = predicted
                return predicted
            }
            if let current = worldTracking.queryDeviceAnchor(atTimestamp: CACurrentMediaTime()),
               current.trackingState == .tracked {
                lastAnchor = current
                return current
            }
        }
        return lastAnchor
    }

    private func updateTracking(anchor: DeviceAnchor, drawables: [LayerRenderer.Drawable]) {
        let head = anchor.originFromAnchorTransform
        if mode == .portal {
            if trackingBasis == nil {
                let basis = Self.yawAligned(head)
                trackingBasis = basis
                portalTransform = basis * Self.translation(x: 0, y: 0, z: -0.8)
            }
        } else if cockpitReference == nil {
            cockpitReference = head
            trackingBasis = head
        }

        guard let basis = trackingBasis else { return }
        let viewTransforms = drawables.flatMap { drawable in
            drawable.views.map(\.transform)
        }
        guard !viewTransforms.isEmpty else { return }

        let basisInverse = simd_inverse(basis)
        var eyePositions = [SIMD3<Float>](repeating: .zero, count: 2)
        for index in 0..<min(2, viewTransforms.count) {
            let eyeWorld = head * viewTransforms[index]
            let local = basisInverse * eyeWorld
            eyePositions[index] = SIMD3<Float>(local.columns.3.x, local.columns.3.y, local.columns.3.z)
        }
        if viewTransforms.count == 1 {
            eyePositions[1] = eyePositions[0]
        }

        let rotation: SIMD3<Float>
        if mode == .cockpit, let reference = cockpitReference {
            rotation = Self.eulerXYZ(simd_inverse(reference) * head)
        } else {
            rotation = .zero
        }

        let timestamp = UInt64(
            max(0, drawables[0].frameTiming.presentationTime.ctrTimeInterval) * 1_000_000_000
        )
        NativeVision_PublishEyeTracking(
            timestamp,
            eyePositions[0].x, eyePositions[0].y, eyePositions[0].z,
            rotation.x, rotation.y, rotation.z,
            eyePositions[1].x, eyePositions[1].y, eyePositions[1].z,
            rotation.x, rotation.y, rotation.z,
            4.0, 256.0
        )
    }

    private func encode(
        drawable: LayerRenderer.Drawable,
        anchor: DeviceAnchor?,
        eyeBaseIndex: Int,
        commandBuffer: MTLCommandBuffer
    ) {
        let views = drawable.views
        guard !views.isEmpty,
              CTRVisionFrameHub.currentSceneTexture() != nil,
              CTRVisionFrameHub.currentHudTexture() != nil else { return }

        for viewIndex in views.indices {
            let textureIndex = views[viewIndex].textureMap.textureIndex
            guard textureIndex < drawable.colorTextures.count else { continue }
            let depthTexture = textureIndex < drawable.depthTextures.count
                ? drawable.depthTextures[textureIndex]
                : nil
            encodePass(
                drawable: drawable,
                anchor: anchor,
                viewIndex: viewIndex,
                sourceEyeIndex: min(1, eyeBaseIndex + viewIndex),
                colorTexture: drawable.colorTextures[textureIndex],
                depthTexture: depthTexture,
                commandBuffer: commandBuffer
            )
        }
    }

    private func encodePass(
        drawable: LayerRenderer.Drawable,
        anchor: DeviceAnchor?,
        viewIndex: Int,
        sourceEyeIndex: Int,
        colorTexture: MTLTexture,
        depthTexture: MTLTexture?,
        commandBuffer: MTLCommandBuffer
    ) {
        guard preparePipeline(
                  colorFormat: colorTexture.pixelFormat,
                  depthFormat: depthTexture?.pixelFormat ?? .invalid
              ),
              let pipeline,
              let sampler,
              let sceneTexture = CTRVisionFrameHub.currentSceneTexture(),
              let hudTexture = CTRVisionFrameHub.currentHudTexture() else {
            return
        }

        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = colorTexture
        pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].storeAction = .store
        pass.colorAttachments[0].clearColor = mode == .portal
            ? MTLClearColorMake(0, 0, 0, 0)
            : MTLClearColorMake(0, 0, 0, 1)
        let textureMap = drawable.views[viewIndex].textureMap
        if colorTexture.textureType == .type2DArray {
            pass.colorAttachments[0].slice = textureMap.sliceIndex
        }
        if let depthTexture {
            pass.depthAttachment.texture = depthTexture
            pass.depthAttachment.loadAction = .clear
            pass.depthAttachment.storeAction = .store
            pass.depthAttachment.clearDepth = 0.0
            if depthTexture.textureType == .type2DArray {
                pass.depthAttachment.slice = textureMap.sliceIndex
            }
        }
        guard let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: pass) else {
            return
        }
        encoder.label = mode == .portal ? "CTR portal" : "CTR cockpit"
        encoder.setRenderPipelineState(pipeline)
        encoder.setDepthStencilState(depthState)

        let mvp: simd_float4x4
        if mode == .portal, let anchor, let portalTransform {
            let eyeWorld = anchor.originFromAnchorTransform * drawable.views[viewIndex].transform
            mvp = drawable.computeProjection(convention: .rightUpBack, viewIndex: viewIndex)
                * simd_inverse(eyeWorld)
                * portalTransform
        } else {
            mvp = matrix_identity_float4x4
        }
        var uniform = EyeUniform(
            modelViewProjection: mvp,
            halfSize: mode == .portal ? SIMD2<Float>(0.36, 0.27) : SIMD2<Float>(1, 1),
            mode: UInt32(mode.rawValue),
            eye: UInt32(sourceEyeIndex)
        )

        encoder.setViewport(textureMap.viewport)
        encoder.setVertexBytes(&uniform, length: MemoryLayout<EyeUniform>.stride, index: 0)
        encoder.setFragmentTexture(sceneTexture, index: 0)
        encoder.setFragmentTexture(hudTexture, index: 1)
        encoder.setFragmentSamplerState(sampler, index: 0)
        encoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
        encoder.endEncoding()
    }

    private func preparePipeline(colorFormat: MTLPixelFormat, depthFormat: MTLPixelFormat) -> Bool {
        if pipeline != nil,
           pipelineColorFormat == colorFormat,
           pipelineDepthFormat == depthFormat {
            return true
        }

        do {
            let library = try layerRenderer.device.makeLibrary(source: Self.shaderSource, options: nil)
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.label = "CTR immersive composite"
            descriptor.vertexFunction = library.makeFunction(name: "ctrCompositeVertex")
            descriptor.fragmentFunction = library.makeFunction(name: "ctrCompositeFragment")
            descriptor.colorAttachments[0].pixelFormat = colorFormat
            descriptor.depthAttachmentPixelFormat = depthFormat
            pipeline = try layerRenderer.device.makeRenderPipelineState(descriptor: descriptor)

            let depthDescriptor = MTLDepthStencilDescriptor()
            depthDescriptor.depthCompareFunction = .greaterEqual
            depthDescriptor.isDepthWriteEnabled = true
            depthState = layerRenderer.device.makeDepthStencilState(descriptor: depthDescriptor)

            let samplerDescriptor = MTLSamplerDescriptor()
            samplerDescriptor.minFilter = .nearest
            samplerDescriptor.magFilter = .nearest
            samplerDescriptor.sAddressMode = .clampToEdge
            samplerDescriptor.tAddressMode = .clampToEdge
            sampler = layerRenderer.device.makeSamplerState(descriptor: samplerDescriptor)
            pipelineColorFormat = colorFormat
            pipelineDepthFormat = depthFormat
            return true
        } catch {
            NSLog("CTR compositor Metal setup failed: %@", error.localizedDescription)
            pipeline = nil
            return false
        }
    }

    private static func translation(x: Float, y: Float, z: Float) -> simd_float4x4 {
        var matrix = matrix_identity_float4x4
        matrix.columns.3 = SIMD4<Float>(x, y, z, 1)
        return matrix
    }

    private static func yawAligned(_ transform: simd_float4x4) -> simd_float4x4 {
        let yaw = atan2(-transform.columns.0.z, transform.columns.0.x)
        let c = cos(yaw)
        let s = sin(yaw)
        var result = matrix_identity_float4x4
        result.columns.0 = SIMD4<Float>(c, 0, -s, 0)
        result.columns.2 = SIMD4<Float>(s, 0, c, 0)
        result.columns.3 = transform.columns.3
        return result
    }

    private static func eulerXYZ(_ transform: simd_float4x4) -> SIMD3<Float> {
        let q = simd_normalize(simd_quatf(transform))
        let x = q.imag.x
        let y = q.imag.y
        let z = q.imag.z
        let w = q.real
        let pitch = atan2(2 * (w * x + y * z), 1 - 2 * (x * x + y * y))
        let yaw = asin(max(-1, min(1, 2 * (w * y - z * x))))
        let roll = atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z))
        return SIMD3<Float>(pitch, yaw, roll)
    }

    private static let shaderSource = """
    #include <metal_stdlib>
    using namespace metal;

    struct EyeUniform {
        float4x4 modelViewProjection;
        float2 halfSize;
        uint mode;
        uint eye;
    };

    struct CompositeVertexOut {
        float4 position [[position]];
        float2 uv;
        uint eye [[flat]];
    };

    vertex CompositeVertexOut ctrCompositeVertex(
        uint vertexID [[vertex_id]],
        constant EyeUniform &uniform [[buffer(0)]]) {
        constexpr float2 positions[4] = {
            float2(-1.0, -1.0), float2(1.0, -1.0),
            float2(-1.0,  1.0), float2(1.0,  1.0)
        };
        constexpr float2 uvs[4] = {
            float2(0.0, 1.0), float2(1.0, 1.0),
            float2(0.0, 0.0), float2(1.0, 0.0)
        };
        CompositeVertexOut out;
        float2 localPosition = positions[vertexID] * uniform.halfSize;
        out.position = uniform.mode == 2
            ? float4(positions[vertexID], 0.5, 1.0)
            : uniform.modelViewProjection * float4(localPosition, 0.0, 1.0);
        out.uv = uvs[vertexID];
        out.eye = uniform.eye;
        return out;
    }

    fragment float4 ctrCompositeFragment(
        CompositeVertexOut in [[stage_in]],
        texture2d_array<float> scene [[texture(0)]],
        texture2d<float> hud [[texture(1)]],
        sampler nearestSampler [[sampler(0)]]) {
        uint eye = min(in.eye, max(scene.get_array_size(), 1u) - 1u);
        float4 world = scene.sample(nearestSampler, in.uv, eye);
        float4 overlay = hud.sample(nearestSampler, in.uv);
        return float4(mix(world.rgb, overlay.rgb, overlay.a), 1.0);
    }
    """
}

#endif
