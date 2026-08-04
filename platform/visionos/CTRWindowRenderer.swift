#if os(visionOS)

import Foundation
import Metal
import MetalKit
import SwiftUI

struct CTRGameMetalView: UIViewRepresentable {
    func makeCoordinator() -> Coordinator {
        Coordinator()
    }

    func makeUIView(context: Context) -> MTKView {
        let view = MTKView(frame: .zero, device: MTLCreateSystemDefaultDevice())
        view.colorPixelFormat = .bgra8Unorm_srgb
        view.clearColor = MTLClearColorMake(0.015, 0.015, 0.02, 1.0)
        view.framebufferOnly = true
        view.preferredFramesPerSecond = 60
        view.enableSetNeedsDisplay = false
        view.isPaused = false
        context.coordinator.attach(to: view)
        return view
    }

    func updateUIView(_ uiView: MTKView, context: Context) {}

    final class Coordinator: NSObject, MTKViewDelegate {
        private var commandQueue: MTLCommandQueue?
        private var pipeline: MTLRenderPipelineState?
        private var sampler: MTLSamplerState?
        private var transparentTexture: MTLTexture?

        func attach(to view: MTKView) {
            guard let device = view.device else { return }
            commandQueue = device.makeCommandQueue()

            do {
                let library = try device.makeLibrary(source: Self.shaderSource, options: nil)
                let descriptor = MTLRenderPipelineDescriptor()
                descriptor.label = "CTR window presentation"
                descriptor.vertexFunction = library.makeFunction(name: "ctrWindowVertex")
                descriptor.fragmentFunction = library.makeFunction(name: "ctrWindowFragment")
                descriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat
                pipeline = try device.makeRenderPipelineState(descriptor: descriptor)
            } catch {
                NSLog("CTR window Metal setup failed: %@", error.localizedDescription)
            }

            let samplerDescriptor = MTLSamplerDescriptor()
            samplerDescriptor.minFilter = .nearest
            samplerDescriptor.magFilter = .nearest
            samplerDescriptor.sAddressMode = .clampToEdge
            samplerDescriptor.tAddressMode = .clampToEdge
            sampler = device.makeSamplerState(descriptor: samplerDescriptor)

            let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(
                pixelFormat: .bgra8Unorm_srgb,
                width: 1,
                height: 1,
                mipmapped: false
            )
            textureDescriptor.usage = .shaderRead
            textureDescriptor.storageMode = .shared
            transparentTexture = device.makeTexture(descriptor: textureDescriptor)
            var transparent: UInt32 = 0
            transparentTexture?.replace(
                region: MTLRegionMake2D(0, 0, 1, 1),
                mipmapLevel: 0,
                withBytes: &transparent,
                bytesPerRow: MemoryLayout<UInt32>.stride
            )

            view.delegate = self
        }

        func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}

        func draw(in view: MTKView) {
            guard let queue = commandQueue,
                  let pipeline,
                  let sampler,
                  let pass = view.currentRenderPassDescriptor,
                  let drawable = view.currentDrawable,
                  let commandBuffer = queue.makeCommandBuffer(),
                  let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: pass) else {
                return
            }

            if let scene = CTRVisionFrameHub.currentSceneTexture() {
                encoder.setRenderPipelineState(pipeline)
                encoder.setFragmentTexture(scene, index: 0)
                encoder.setFragmentTexture(CTRVisionFrameHub.currentHudTexture() ?? transparentTexture, index: 1)
                encoder.setFragmentSamplerState(sampler, index: 0)
                encoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
            }
            encoder.endEncoding()
            commandBuffer.present(drawable)
            commandBuffer.commit()
        }

        private static let shaderSource = """
        #include <metal_stdlib>
        using namespace metal;

        struct WindowVertexOut {
            float4 position [[position]];
            float2 uv;
        };

        vertex WindowVertexOut ctrWindowVertex(uint vertexID [[vertex_id]]) {
            constexpr float2 positions[4] = {
                float2(-1.0, -1.0), float2(1.0, -1.0),
                float2(-1.0,  1.0), float2(1.0,  1.0)
            };
            constexpr float2 uvs[4] = {
                float2(0.0, 1.0), float2(1.0, 1.0),
                float2(0.0, 0.0), float2(1.0, 0.0)
            };
            WindowVertexOut out;
            out.position = float4(positions[vertexID], 0.0, 1.0);
            out.uv = uvs[vertexID];
            return out;
        }

        fragment float4 ctrWindowFragment(WindowVertexOut in [[stage_in]],
                                          texture2d_array<float> scene [[texture(0)]],
                                          texture2d<float> hud [[texture(1)]],
                                          sampler nearestSampler [[sampler(0)]]) {
            float4 world = scene.sample(nearestSampler, in.uv, 0);
            float4 overlay = hud.sample(nearestSampler, in.uv);
            return float4(mix(world.rgb, overlay.rgb, overlay.a), 1.0);
        }
        """
    }
}

#endif
