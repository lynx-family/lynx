# Explorer App

The Explorer app is the demo/development app for Lynx, available on iOS, Android, macOS, and Windows. It hosts a Lynx-rendered homepage and provides navigation to showcase pages.

## Architecture

### Homepage (Lynx Frontend)

- **Source**: `explorer/homepage/` (TypeScript + SCSS, built with `@lynx-js/rspeedy`)
- **Build**: `cd explorer/homepage && npx rspeedy build` produces `dist/main.lynx.bundle`
- **Bundle must be copied** to platform-specific resource directories after build:
  - iOS: `explorer/darwin/ios/lynx_explorer/LynxExplorer/Resource/homepage.lynx.bundle`
  - The full setup script `explorer/darwin/ios/lynx_explorer/bundle_install.sh` handles build + copy + pod install

### iOS Shell (`LynxViewShellViewController`)

- **Path**: `explorer/darwin/ios/lynx_explorer/LynxExplorer/`
- The shell creates a `LynxView` and passes **global props** to the frontend:
  - `isNotchScreen`, `safeAreaTop`, `safeAreaBottom`, `screenWidth`, `screenHeight`
  - `theme` (Light/Dark from system), `preferredTheme` (user preference)
  - Query parameters from the URL are converted to camelCase global props
- **Layout modes**: `fullscreen`, `hiddenNav`, or standard (with nav bar)
- The homepage loads with `fullscreen=true`

### Android Shell (`LynxViewShellActivity`)

- **Path**: `explorer/android/lynx_explorer/src/main/java/com/lynx/explorer/`
- Same global props as iOS

### Global Props (Native → Frontend)

| Prop | Type | Description |
|---|---|---|
| `isNotchScreen` | `boolean` | Whether the device has a notch/cutout |
| `safeAreaTop` | `number` | Top safe area inset in pt/dp |
| `safeAreaBottom` | `number` | Bottom safe area inset in pt/dp |
| `screenWidth` | `number` | Screen width in pt/dp |
| `screenHeight` | `number` | Screen height in pt/dp |
| `theme` | `string` | System theme: `"Light"` or `"Dark"` |
| `preferredTheme` | `string?` | User-selected theme preference |
| `frontendTheme` | `string` | `"light"` or `"dark"` |

Type declarations: `explorer/homepage/typing.d.ts`

## iOS Build

```bash
cd explorer/darwin/ios/lynx_explorer

# Full setup (builds homepage, generates podspecs, pod install):
bash bundle_install.sh

# Or manually:
# 1. Build homepage
cd ../../homepage && npx rspeedy build
cp dist/main.lynx.bundle ../darwin/ios/lynx_explorer/LynxExplorer/Resource/homepage.lynx.bundle

# 2. Build Xcode project
xcodebuild -workspace LynxExplorer.xcworkspace -scheme LynxExplorer \
  -sdk iphonesimulator -destination 'id=<SIMULATOR_UDID>' build

# 3. Install & launch on simulator
xcrun simctl install <UDID> build/Build/Products/Debug-iphonesimulator/LynxExplorer.app
xcrun simctl launch <UDID> com.lynx.LynxExplorer
```

## Pod Dependencies

The iOS project uses CocoaPods with local pods from the monorepo root (`pod 'Lynx', :path => '../../../..'`). Run `bundle exec pod install` from the `lynx_explorer/` directory. Some pods (like `Sparkling`) are private and may not be available in all environments.
