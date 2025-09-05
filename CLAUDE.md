@@ -0,0 +1,179 @@
# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Lynx is a cross-platform UI framework that enables developers to build native UIs for Android, iOS, and Web from a single codebase using web technologies (CSS and React-like syntax). This repository contains the core engine written primarily in C++ with platform-specific bindings.

## Build System and Architecture

- **Build System**: Uses GN (Generate Ninja) build system with BUILD.gn files throughout the codebase
- **Configuration**: Main configuration in `config.gni` with platform-specific imports
- **Cross-Platform Support**: Android, iOS (Darwin), HarmonyOS, and Web
- **Core Engine**: Written in C++ with JavaScript bindings via V8/QuickJS
- **Platforms**: Native implementations for each target platform in `platform/` directory

## Key Directories

- `core/`: Core engine implementation (animation, rendering, runtime, etc.)
- `platform/`: Platform-specific implementations (android/, darwin/, harmony/)
- `base/`: Foundational utilities and data structures
- `devtool/`: Development and debugging tools
- `js_libraries/`: TypeScript/JavaScript SDK and type definitions
- `third_party/`: External dependencies (v8, googletest, rapidjson, etc.)
- `tools/`: Build tools, code generators, and utilities
- `testing/`: Test infrastructure and configurations

## Common Development Commands

### Environment Setup
```bash
source tools/envsetup.sh
tools/hab sync . -f
```

### Building

#### Android

Enter the `explorer/android` directory from the project root directory and execute the following command.

```
cd explorer/android
./gradlew :LynxExplorer:assembleNoAsanDebug --no-daemon
```

This command will generate LynxExplorer-noasan-debug.apk in the `lynx_explorer/build/outputs/apk/noasan/debug/` folder.

#### iOS

1. Install iOS project dependencies
```
cd explorer/darwin/ios/lynx_explorer
./bundle_install.sh
```
2. After step 1, `LynxExplorer.xcworkspace` will be generated in the lynx_explorer directory. Open `LynxExplorer.xcworkspace` by Xcode.
3. Select `LynxExplorer` to execute the build in Xcode.

### Code Quality
```bash
# Run all static analysis checks
git lynx check

# Format code automatically
git lynx format

# Run specific checks
git lynx check --checkers=api-check
git lynx check --checkers=cpplint
git lynx check --checkers=coding-style
```

### Testing
```bash
# Run C++ unit tests via RTF tool
tools/rtf/rtf native-ut run --names lynx --target <test_target>

# Run Android unit tests
tools/rtf/rtf android-ut run --name lynx

# For iOS tests, use Xcode with LynxExplorer.xcworkspace
```

### JavaScript/TypeScript Development
```bash
# Build core JavaScript libraries
pnpm run build:core:android
pnpm run build:core:darwin
pnpm run export:core
```

## Commit Message Format

Use the following format for commits:
```
[Label] Title of the commit message

Summary of change:
Longer description of change addressing why the change
is made, context if it is part of many changes, description 
of previous behavior and newly introduced differences, etc.

issue: #xxx (optional)
doc: https://xxxxxxxx (optional, required for Feature/Refactor)
TEST: test_case_1, test_case_2 (optional)
```

Required labels: `[Feature]`, `[BugFix]`, `[Refactor]`, `[Optimize]`, `[Infra]`, `[Testing]`, `[Doc]`

## Code Style

- Follows Google Style Guides for C++, Java, Objective-C, and Python
- TypeScript formatted with Prettier
- GN files formatted with `gn format`
- Automatic formatting available via `git lynx format`

## Testing Conventions

- C++ unit tests use Google Test framework
- Test files named with `_unittest.cc` suffix in same directory as source
- Android tests use JUnit4 with instrumentation testing
- iOS tests use XCTest framework
- RTF tool manages test execution and configuration

## Special Build Configurations

- **Trace Support**: Configure via `enable_trace` (perfetto/systrace/none)
- **V8 Engine**: Enable/disable via `enable_v8=true/false`
- **Lite Mode**: Use `enable_lite=true` for minimal builds
- **Unit Tests**: Enable via `enable_unittests=true`
- **Inspector**: Enable via `enable_inspector=true`

## Platform-Specific Notes

- **Android**: Uses CMake + Gradle integration, JNI bindings
- **iOS/macOS**: Uses CocoaPods, Objective-C++ bindings  
- **HarmonyOS**: Uses N-API bindings, special build configurations
- **Web**: Uses WebAssembly compilation target

No newline at end of file
