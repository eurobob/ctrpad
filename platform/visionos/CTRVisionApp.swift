#if os(visionOS)

import Foundation
import GameController
import os
import SwiftUI
import UniformTypeIdentifiers
import _CompositorServices_SwiftUI
import Darwin

private let ctrVisionLog = Logger(
    subsystem: Bundle.main.bundleIdentifier ?? "io.github.chrissotraidis.ctrpad.vision",
    category: "CTRPad"
)

private struct CTRDiscValidationError: LocalizedError {
    let message: String

    var errorDescription: String? { message }
}

private func discValidationFailure(at url: URL) -> String? {
    var message = [CChar](repeating: 0, count: 512)
    let valid = message.withUnsafeMutableBufferPointer { messageBuffer in
        url.path.withCString { path in
            NativeVision_ValidateDiscImage(path, messageBuffer.baseAddress!, messageBuffer.count)
        }
    }
    return valid != 0 ? nil : String(cString: message)
}

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
            if let failure = discValidationFailure(at: importedDiscURL) {
                status = failure
                try? FileManager.default.removeItem(at: importedDiscURL)
            } else {
                discName = "Imported CTR disc"
                status = "Disc ready. Starting CTR…"
            }
        }
    }

    var importedDiscURL: URL {
        let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
        return base.appendingPathComponent("CTRPad", isDirectory: true)
            .appendingPathComponent("ctr-disc.bin", isDirectory: false)
    }

    func startImportedDiscIfReady() {
        guard discName != nil, !didLaunch else { return }
        launch(discURL: importedDiscURL)
    }

    func setPresentationStatus(_ message: String) {
        guard isRunning else { return }
        status = message
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
                if let failure = discValidationFailure(at: staging) {
                    try? FileManager.default.removeItem(at: staging)
                    throw CTRDiscValidationError(message: failure)
                }
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
        ctrVisionLog.notice("[CTR Swift] runtime launch requested")

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
                    CTRNativeMain(Int32(arguments.count), buffer.baseAddress!)
                }
                ctrVisionLog.notice("[CTR Swift] native runtime returned result=\(result)")
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
    @Environment(\.scenePhase) private var scenePhase
    @ObservedObject var runtime: CTRVisionRuntime

    @State private var isImporting = false
    @State private var activeSpaceID: String?
    @State private var isOpeningImmersiveSpace = false
    @AppStorage("CTRPadInternalResolutionScale") private var resolutionScale = 3

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

            TimelineView(.periodic(from: .now, by: 1.0)) { _ in
                let controller = GCController.current ?? GCController.controllers().first
                Label(
                    controller.map { "Controller: " + ($0.vendorName ?? "Game Controller") }
                        ?? "No game controller visible to CTRPad",
                    systemImage: controller == nil ? "gamecontroller.slash" : "gamecontroller.fill"
                )
                .font(.footnote.weight(.semibold))
                .foregroundStyle(controller == nil ? .orange : .green)
            }

            HStack(spacing: 12) {
                Text("Internal resolution")
                    .font(.footnote.weight(.semibold))
                Picker("Internal resolution", selection: $resolutionScale) {
                    ForEach(1...4, id: \.self) { scale in
                        Text("\(scale)×").tag(scale)
                    }
                }
                .pickerStyle(.segmented)
                .frame(maxWidth: 320)
            }

            HStack(spacing: 12) {
                Button("Choose Disc…", systemImage: "opticaldisc") {
                    isImporting = true
                }
                .disabled(runtime.isRunning)

                Button("Portal", systemImage: "rectangle.inset.filled") {
                    open(mode: .portal, spaceID: CTRVisionApp.portalSpaceID)
                }
                .disabled(!runtime.isRunning || isOpeningImmersiveSpace)

                Button("Cockpit VR", systemImage: "visionpro") {
                    open(mode: .cockpit, spaceID: CTRVisionApp.cockpitSpaceID)
                }
                .disabled(!runtime.isRunning || isOpeningImmersiveSpace)

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
        .handlesGameControllerEvents(matching: .gamepad)
        .task {
            // Let SwiftUI establish the controller-event routing surface before
            // the detached native loop begins polling GameController.
            resolutionScale = Int(NativeRenderer_SetInternalResolutionScale(Int32(resolutionScale)))
            await Task.yield()
            runtime.startImportedDiscIfReady()
        }
        .onChange(of: resolutionScale) { scale in
            let applied = Int(NativeRenderer_SetInternalResolutionScale(Int32(scale)))
            if applied != scale {
                resolutionScale = applied
            }
            runtime.setPresentationStatus(
                "Running at \(applied)× internal resolution (\(512 * applied)×\(216 * applied))."
            )
            ctrVisionLog.notice("[CTR Swift] requested internal resolution scale=\(applied)")
        }
        .onChange(of: scenePhase) { phase in
            ctrVisionLog.notice("[CTR Swift] window scene phase=\(String(describing: phase), privacy: .public)")
        }
        .onReceive(NotificationCenter.default.publisher(for: .ctrImmersiveSpaceDidClose)) { _ in
            activeSpaceID = nil
            isOpeningImmersiveSpace = false
            runtime.setPresentationStatus("Immersive space closed. Running in the window.")
        }
        .onReceive(NotificationCenter.default.publisher(for: .ctrImmersiveSpaceDidFail)) { _ in
            Task {
                await dismissImmersiveSpace()
                NativeVision_SetMode(0)
                NativeVision_ResetTracking()
                activeSpaceID = nil
                isOpeningImmersiveSpace = false
                runtime.setPresentationStatus(
                    "Immersive rendering could not start. Check world-sensing permission and try again."
                )
            }
        }
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
        guard !isOpeningImmersiveSpace else { return }
        isOpeningImmersiveSpace = true
        let modeName = mode == .portal ? "Portal" : "Cockpit VR"
        runtime.setPresentationStatus("Opening \(modeName)…")
        ctrVisionLog.notice("[CTR Swift] immersive open requested mode=\(modeName, privacy: .public) id=\(spaceID, privacy: .public)")

        Task {
            if activeSpaceID != nil {
                await dismissImmersiveSpace()
                activeSpaceID = nil
            }
            let result = await openImmersiveSpace(id: spaceID)

            switch result {
            case .opened:
                activeSpaceID = spaceID
                runtime.setPresentationStatus("\(modeName) active. Use Exit to return to the window.")
                ctrVisionLog.notice("[CTR Swift] immersive open succeeded mode=\(modeName, privacy: .public)")
            case .userCancelled:
                NativeVision_SetMode(0)
                NativeVision_ResetTracking()
                activeSpaceID = nil
                runtime.setPresentationStatus("\(modeName) was cancelled.")
                ctrVisionLog.notice("[CTR Swift] immersive open cancelled mode=\(modeName, privacy: .public)")
            case .error:
                NativeVision_SetMode(0)
                NativeVision_ResetTracking()
                activeSpaceID = nil
                runtime.setPresentationStatus("visionOS could not open \(modeName). Try again after closing any other immersive app.")
                ctrVisionLog.error("[CTR Swift] immersive open failed mode=\(modeName, privacy: .public)")
            @unknown default:
                NativeVision_SetMode(0)
                NativeVision_ResetTracking()
                activeSpaceID = nil
                runtime.setPresentationStatus("visionOS returned an unknown result while opening \(modeName).")
                ctrVisionLog.error("[CTR Swift] immersive open returned unknown result mode=\(modeName, privacy: .public)")
            }
            isOpeningImmersiveSpace = false
        }
    }
}

#endif
