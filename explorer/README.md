# Lynx Explorer

The official app for testing and exploring Lynx. It is featured in Lynx Quick Start Guide at <https://lynxjs.org/guide/start/quick-start.html>.

The dir consists of two main parts:
1. Native applications (Android/iOS/Harmony/Windows/macOS) that provide the runtime environment
2. ReactLynx-based web applications that run inside the native apps

## Building the Native Apps

If you want to build and run the native applications from source, please refer to the platform-specific dirs and guides:

### android/
Contains the native Android apps that integrated Lynx. See [Android Build Guide](android/README.md) for instructions.

### darwin/ios/
Contains the native iOS apps that integrated Lynx. See [iOS Build Guide](darwin/ios/README.md) for instructions.

### harmony/
Contains the native Harmony apps that integrated Lynx. See [Harmony Build Guide](harmony/README.md) for instructions.

### windows/
Contains the native Windows apps that integrated Lynx. See [Windows Build Guide](windows/README.md) for instructions.

### darwin/macos/
Contains the native macOS apps that integrated Lynx. See [macOS Build Guide](darwin/macos/README.md) for instructions.

## Node-API Addons

Lynx provides an experimental capability for exposing Node-API addons to Lynx pages. The open-source Explorer apps include a sample module named `LynxNodeAPIModule` to demonstrate how a host app can expose this capability to Lynx pages. Apple platforms prefer static integration; other platforms in Explorer continue to demonstrate dynamic addon binaries.

For the experimental capability model, SDK prerequisites, the `LynxNodeAPIModule` sample entry points, and addon authoring guidance, see [Lynx Node-API Addons](docs/lynx-node-api.md).

Packaging and IDE/build integration remain platform-specific. See:
- Android: [Android Build Guide](android/README.md)
- iOS: [iOS Build Guide](darwin/ios/README.md)
- Harmony: [Harmony Build Guide](harmony/README.md)
- Windows: [Windows Build Guide](windows/README.md)
- macOS: [macOS Build Guide](darwin/macos/README.md)

## Developing the Bundled Lynx Projects

If you already have a built Lynx Explorer app (or any other Lynx-integrated environment), you can focus on developing the Lynx screens that run inside it. There are currently two screens: 

### homepage/
Contains the home screen of Lynx Explorer implemented with ReactLynx. This is the entry point of the application.

### showcase/
Contains the showcase screen of Lynx Explorer implemented with ReactLynx. This demonstrates various Lynx features and capabilities by integrating the official Lynx examples at <https://github.com/lynx-family/lynx-examples>.
