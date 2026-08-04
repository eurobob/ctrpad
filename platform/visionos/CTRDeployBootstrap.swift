#if os(visionOS)

import SwiftUI

// mac-verify needs an Xcode application product so it can apply the local
// development team and install it. The target's post-build phase replaces
// this bootstrap bundle with the complete app built by CMake before Xcode's
// signing phase runs, so this entry point is never present in the deployed app.
@main
private struct CTRDeployBootstrap: App {
    var body: some Scene {
        WindowGroup {
            Text("Preparing CTRPad…")
        }
    }
}

#endif
