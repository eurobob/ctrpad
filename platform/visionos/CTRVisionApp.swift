#if os(visionOS)

import Foundation
import SwiftUI
import UniformTypeIdentifiers
import _CompositorServices_SwiftUI
import Darwin

@main
struct CTRVisionApp: App {
    static let portalSpaceID = "CTRPortal"
    static let cockpitSpaceID = "CTRCockpit"

    @StateObject private var runtime = CTRVisionRuntime()

    var body: some Scene {
        WindowGroup {
            CTRLauncherView(runtime: runtime)
        }
        .defaultSize(width: 760, height: 650)

        ImmersiveSpace(id: Self.portalSpaceID) {
            CompositorLayer(configuration: CTRCompositorConfiguration()) { layerRenderer in
                CTRCompositorRenderer.startRenderLoop(layerRenderer, mode: .portal)
            }
        }
        .immersionStyle(selection: .constant(.mixed), in: .mixed)

        ImmersiveSpace(id: Self.cockpitSpaceID) {
            CompositorLayer(configuration: CTRCompositorConfiguration()) { layerRenderer in
                CTRCompositorRenderer.startRenderLoop(layerRenderer, mode: .cockpit)
            }
        }
        .immersionStyle(selection: .constant(.full), in: .full)
    }
}

@MainActor
final class CTRVisionRuntime: ObservableObject {
    @Published private(set) var isRunning = false
    @Published private(set) var discName: String?
    @Published private(set) var status = "Choose your NTSC-U CTR disc image to begin."

    private var didLaunch = false

    init() {
        if FileManager.default.fileExists(atPath: importedDiscURL.path) {
            discName = "Imported CTR disc"
            launch(discURL: importedDiscURL)
        }
    }

    var importedDiscURL: URL {
        let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
        return base.appendingPathComponent("CTRPad", isDirectory: true)
            .appendingPathComponent("ctr-disc.bin", isDirectory: false)
    }

    func importDisc(from selectedURL: URL) async {
        guard !didLaunch else {
            status = "Restart CTRPad to replace the disc image."
            return
        }

        let didAccess = selectedURL.startAccessingSecurityScopedResource()
        defer {
            if didAccess {
                selectedURL.stopAccessingSecurityScopedResource()
            }
        }

        do {
            let destination = importedDiscURL
            status = "Importing \(selectedURL.lastPathComponent)…"
            try await Task.detached(priority: .userInitiated) {
                try FileManager.default.createDirectory(
                    at: destination.deletingLastPathComponent(),
                    withIntermediateDirectories: true
                )
                let staging = destination.appendingPathExtension("importing")
                if FileManager.default.fileExists(atPath: staging.path) {
                    try FileManager.default.removeItem(at: staging)
                }
                try FileManager.default.copyItem(at: selectedURL, to: staging)
                if FileManager.default.fileExists(atPath: destination.path) {
                    _ = try FileManager.default.replaceItemAt(destination, withItemAt: staging)
                } else {
                    try FileManager.default.moveItem(at: staging, to: destination)
                }
            }.value
            if Task.isCancelled {
                status = "Import cancelled."
                return
            }
            discName = selectedURL.lastPathComponent
            launch(discURL: destination)
        } catch {
            status = "Import failed: \(error.localizedDescription)"
        }
    }

    private func launch(discURL: URL) {
        guard !didLaunch else { return }
        didLaunch = true
        status = "Starting CTR…"
        let path = discURL.path

        Thread.detachNewThread { [weak self] in
            autoreleasepool {
                let arguments = ["CTRPad", "--disc", path]
                var storage: [UnsafeMutablePointer<CChar>?] = arguments.map { argument in
                    argument.withCString { strdup($0) }
                }
                storage.append(nil)
                defer {
                    for case let pointer? in storage {
                        free(pointer)
                    }
                }

                let result = storage.withUnsafeMutableBufferPointer { buffer in
                    CTRNativeMain(Int32(arguments.count), buffer.baseAddress)
                }
                Task { @MainActor [weak self] in
                    self?.isRunning = false
                    self?.didLaunch = false
                    self?.status = result == 0 ? "CTR stopped." : "CTR stopped with error \(result)."
                }
            }
        }

        isRunning = true
        status = "Running. Use a game controller, then choose a presentation mode."
    }
}

private struct CTRLauncherView: View {
    @Environment(\.openImmersiveSpace) private var openImmersiveSpace
    @Environment(\.dismissImmersiveSpace) private var dismissImmersiveSpace
    @ObservedObject var runtime: CTRVisionRuntime

    @State private var isImporting = false
    @State private var activeSpaceID: String?

    var body: some View {
        VStack(spacing: 22) {
            VStack(spacing: 5) {
                Text("CTRPad")
                    .font(.largeTitle.bold())
                Text("visionOS stereo preview")
                    .foregroundStyle(.secondary)
            }

            CTRGameMetalView()
                .aspectRatio(4.0 / 3.0, contentMode: .fit)
                .clipShape(RoundedRectangle(cornerRadius: 18, style: .continuous))
                .overlay {
                    RoundedRectangle(cornerRadius: 18, style: .continuous)
                        .stroke(.white.opacity(0.16), lineWidth: 1)
                }

            Text(runtime.status)
                .font(.footnote)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)

            HStack(spacing: 12) {
                Button("Choose Disc…", systemImage: "opticaldisc") {
                    isImporting = true
                }
                .disabled(runtime.isRunning)

                Button("Portal", systemImage: "rectangle.inset.filled") {
                    open(mode: .portal, spaceID: CTRVisionApp.portalSpaceID)
                }
                .disabled(!runtime.isRunning)

                Button("Cockpit VR", systemImage: "visionpro") {
                    open(mode: .cockpit, spaceID: CTRVisionApp.cockpitSpaceID)
                }
                .disabled(!runtime.isRunning)

                if activeSpaceID != nil {
                    Button("Exit", systemImage: "xmark") {
                        Task {
                            await dismissImmersiveSpace()
                            NativeVision_SetMode(0)
                            NativeVision_ResetTracking()
                            activeSpaceID = nil
                        }
                    }
                }
            }
            .buttonStyle(.borderedProminent)
        }
        .padding(28)
        .fileImporter(
            isPresented: $isImporting,
            allowedContentTypes: [.data],
            allowsMultipleSelection: false
        ) { result in
            if case .success(let urls) = result, let url = urls.first {
                Task {
                    await runtime.importDisc(from: url)
                }
            }
        }
    }

    private func open(mode: CTRImmersiveMode, spaceID: String) {
        Task {
            if activeSpaceID != nil {
                await dismissImmersiveSpace()
            }
            NativeVision_SetMode(mode.rawValue)
            NativeVision_ResetTracking()
            let result = await openImmersiveSpace(id: spaceID)
            if case .opened = result {
                activeSpaceID = spaceID
            } else {
                NativeVision_SetMode(0)
                NativeVision_ResetTracking()
                activeSpaceID = nil
            }
        }
    }
}

#endif
