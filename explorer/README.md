# Lynx Explorer

The official app for testing and exploring Lynx. It is featured in Lynx Quick Start Guide at <https://lynxjs.org/guide/start/quick-start.html>.

The dir consists of two main parts:
1. Native mobile applications (Android/iOS) that provide the runtime environment
2. ReactLynx-based web applications that run inside the native apps

## Building the Native Apps

If you want to build and run the native mobile applications from source, please refer to the platform-specific dirs and guides:

### android/
Contains the native Android apps that integrated Lynx. See [Android Build Guide](android/README.md) for instructions.

### darwin/
Contains the native iOS apps that integrated Lynx. See [iOS Build Guide](darwin/ios/README.md) for instructions.

## Developing the Bundled Lynx Projects

If you already have a built Lynx Explorer app (or any other Lynx-integrated environment), you can focus on developing the Lynx screens that run inside it. There are currently two screens: 

### homepage/
Contains the home screen of Lynx Explorer implemented with ReactLynx. This is the entry point of the application.

### showcase/
Contains the showcase screen of Lynx Explorer implemented with ReactLynx. This demonstrates various Lynx features and capabilities by integrating the official Lynx examples at <https://github.com/lynx-family/lynx-examples>.
