#if os(visionOS)

import ARKit
import CompositorServices
import Darwin
import Foundation
import Metal
import QuartzCore
import simd
import _CompositorServices_SwiftUI

enum CTRImmersiveMode: Int32, Sendable {
    case portal = 1
    case cockpit = 2
}

extension LayerRenderer.Clock.Instant {
    fileprivate var ctrTimeInterval: TimeInterval {
        let components = LayerRenderer.Clock.Instant.epoch.duration(to: self).components
        let nanoseconds = TimeInterval(components.attoseconds / 1_000_000_000)
        return TimeInterval(components.seconds) + nanoseconds / TimeInterval(NSEC_PER_SEC)
    }
}

@MainActor
struct CTRCompositorConfiguration: _CompositorServices_SwiftUI.CompositorLayerConfiguration {
    func makeConfiguration(
        capabilities: LayerRenderer.Capabilities,
        configuration: inout LayerRenderer.Configuration
    ) {
        let supportsFoveation = capabilities.supportsFoveation
        let layouts = capabilities.supportedLayouts(
            options: supportsFoveation ? [.foveationEnabled] : []
        )
        configuration.layout = layouts.contains(.layered) ? .layered : .dedicated
        configuration.colorFormat = .bgra8Unorm_srgb
        configuration.depthFormat = .depth32Float
        configuration.isFoveationEnabled = supportsFoveation
    }
}

enum CTRCompositorRenderer {
    @MainActor
    static func startRenderLoop(_ layerRenderer: LayerRenderer, mode: CTRImmersiveMode) {
        NativeVision_SetMode(mode.rawValue)
        NativeVision_ResetTracking()
        Task.detached(priority: .userInteractive) {
            guard let renderer = CTRLayerRenderer(layerRenderer: layerRenderer, mode: mode) else {
                NativeVision_SetMode(0)
                return
            }
            guard await renderer.startTracking() else {
                NativeVision_SetMode(0)
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

    init?(layerRenderer: LayerRenderer, mode: CTRImmersiveMode) {
        self.layerRenderer = layerRenderer
        self.mode = mode
        guard let queue = layerRenderer.device.makeCommandQueue() else { return nil }
        commandQueue = queue
    }

    func startTracking() async -> Bool {
        guard WorldTrackingProvider.isSupported else { return false }
#if !targetEnvironment(simulator)
        let results = await arSession.requestAuthorization(
            for: Array(WorldTrackingProvider.requiredAuthorizations)
        )
        if results.values.contains(.denied) {
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
                return true
            }
            try? await Task.sleep(nanoseconds: 10_000_000)
        }
        return false
    }

    func run() {
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
        guard let drawable = frame.queryDrawable() else {
            frame.endSubmission()
            return
        }

        guard let commandBuffer = commandQueue.makeCommandBuffer() else {
            submitEmpty(drawable: drawable, frame: frame)
            return
        }

        let anchor = queryAnchor(for: drawable)
        if let anchor {
            drawable.deviceAnchor = anchor
            updateTracking(anchor: anchor, drawable: drawable)
        }

        encode(drawable: drawable, anchor: anchor, commandBuffer: commandBuffer)
        drawable.encodePresent(commandBuffer: commandBuffer)
        commandBuffer.commit()
        frame.endSubmission()
    }

    private func submitEmpty(drawable: LayerRenderer.Drawable, frame: LayerRenderer.Frame) {
        if let commandBuffer = commandQueue.makeCommandBuffer() {
            drawable.encodePresent(commandBuffer: commandBuffer)
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

    private func updateTracking(anchor: DeviceAnchor, drawable: LayerRenderer.Drawable) {
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
        let views = drawable.views
        guard !views.isEmpty else { return }

        let basisInverse = simd_inverse(basis)
        var eyePositions = [SIMD3<Float>](repeating: .zero, count: 2)
        for index in 0..<min(2, views.count) {
            let eyeWorld = head * views[index].transform
            let local = basisInverse * eyeWorld
            eyePositions[index] = SIMD3<Float>(local.columns.3.x, local.columns.3.y, local.columns.3.z)
        }
        if views.count == 1 {
            eyePositions[1] = eyePositions[0]
        }

        let rotation: SIMD3<Float>
        if mode == .cockpit, let reference = cockpitReference {
            rotation = Self.eulerXYZ(simd_inverse(reference) * head)
        } else {
            rotation = .zero
        }

        let timestamp = UInt64(max(0, drawable.frameTiming.presentationTime.ctrTimeInterval) * 1_000_000_000)
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
        commandBuffer: MTLCommandBuffer
    ) {
        let views = drawable.views
        guard !views.isEmpty else { return }
        let textureIndex = views[0].textureMap.textureIndex
        guard textureIndex < drawable.colorTextures.count else { return }

        let colorTexture = drawable.colorTextures[textureIndex]
        let depthTexture = textureIndex < drawable.depthTextures.count
            ? drawable.depthTextures[textureIndex]
            : nil
        guard preparePipeline(colorFormat: colorTexture.pixelFormat,
                              depthFormat: depthTexture?.pixelFormat ?? .invalid) else {
            return
        }

        let eyeCount = min(2, views.count)
        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture = colorTexture
        pass.colorAttachments[0].loadAction = .clear
        pass.colorAttachments[0].storeAction = .store
        pass.colorAttachments[0].clearColor = mode == .portal
            ? MTLClearColorMake(0, 0, 0, 0)
            : MTLClearColorMake(0, 0, 0, 1)
        pass.renderTargetArrayLength = eyeCount
        if let depthTexture,
           depthTexture.textureType == .type2DArray,
           depthTexture.arrayLength >= eyeCount {
            pass.depthAttachment.texture = depthTexture
            pass.depthAttachment.loadAction = .clear
            pass.depthAttachment.storeAction = .store
            pass.depthAttachment.clearDepth = 0.0
        }
        pass.rasterizationRateMap = drawable.rasterizationRateMaps.first

        guard let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: pass),
              let pipeline,
              let sampler else {
            return
        }
        encoder.label = mode == .portal ? "CTR portal" : "CTR cockpit"
        encoder.setRenderPipelineState(pipeline)
        encoder.setDepthStencilState(depthState)

        guard let sceneTexture = CTRVisionFrameHub.currentSceneTexture(),
              let hudTexture = CTRVisionFrameHub.currentHudTexture() else {
            encoder.endEncoding()
            return
        }

        var uniforms = [EyeUniform]()
        for index in 0..<eyeCount {
            let mvp: simd_float4x4
            if mode == .portal, let anchor, let portalTransform {
                let eyeWorld = anchor.originFromAnchorTransform * views[index].transform
                mvp = drawable.computeProjection(convention: .rightUpBack, viewIndex: index)
                    * simd_inverse(eyeWorld)
                    * portalTransform
            } else {
                mvp = matrix_identity_float4x4
            }
            uniforms.append(EyeUniform(
                modelViewProjection: mvp,
                halfSize: mode == .portal ? SIMD2<Float>(0.36, 0.27) : SIMD2<Float>(1, 1),
                mode: UInt32(mode.rawValue),
                eye: UInt32(index)
            ))
        }

        let viewports = views.prefix(eyeCount).map(\.textureMap.viewport)
        encoder.setVertexAmplificationCount(eyeCount, viewMappings: nil)
        encoder.setViewports(viewports)
        uniforms.withUnsafeBufferPointer { buffer in
            if let baseAddress = buffer.baseAddress {
                encoder.setVertexBytes(
                    baseAddress,
                    length: buffer.count * MemoryLayout<EyeUniform>.stride,
                    index: 0
                )
            }
        }
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
            descriptor.maxVertexAmplificationCount = 2
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
        uint renderTargetArrayIndex [[render_target_array_index]];
    };

    vertex CompositeVertexOut ctrCompositeVertex(
        uint vertexID [[vertex_id]],
        ushort amplificationID [[amplification_id]],
        constant EyeUniform *uniforms [[buffer(0)]]) {
        constexpr float2 positions[4] = {
            float2(-1.0, -1.0), float2(1.0, -1.0),
            float2(-1.0,  1.0), float2(1.0,  1.0)
        };
        constexpr float2 uvs[4] = {
            float2(0.0, 1.0), float2(1.0, 1.0),
            float2(0.0, 0.0), float2(1.0, 0.0)
        };
        EyeUniform uniform = uniforms[amplificationID];
        CompositeVertexOut out;
        float2 localPosition = positions[vertexID] * uniform.halfSize;
        out.position = uniform.mode == 2
            ? float4(positions[vertexID], 0.5, 1.0)
            : uniform.modelViewProjection * float4(localPosition, 0.0, 1.0);
        out.uv = uvs[vertexID];
        out.eye = uniform.eye;
        out.renderTargetArrayIndex = amplificationID;
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
