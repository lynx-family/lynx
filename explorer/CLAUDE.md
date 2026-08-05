# LynxExplorer — Agent Notes

## iOS Build Prerequisites

Before building the iOS app with Xcode, you **must** run the full bundle preparation pipeline. Without this, showcase pages and example bundles will be missing (blank pages).

```bash
cd lynx/explorer/darwin/ios/lynx_explorer
./bundle_install.sh
```

This script (see `bundle_install.sh`) does the following in order:
1. Builds `explorer/homepage` → copies to `Resource/homepage.lynx.bundle`
2. Runs `explorer/showcase/build_and_copy.py` → installs `@lynx-example/*` npm packages, builds all showcase bundles, and copies them into `Resource/showcase/<category>/` with nested directory structure
3. Generates iOS podspecs via GN
4. Runs `bundle install` + `pod install`

**Common mistake**: manually building individual packages (e.g., `cd homepage && pnpm build`) and copying bundles by hand. This skips the showcase examples from npm and produces a flat `Resource/` layout instead of the expected nested structure (`Resource/showcase/menu/`, `Resource/showcase/css/`, etc.).

If you only need to rebuild bundles without re-running pod install, you can run the steps individually:
```bash
# Homepage only
cd lynx/explorer/homepage && pnpm install && pnpm build
cp dist/main.lynx.bundle ../darwin/ios/lynx_explorer/LynxExplorer/Resource/homepage.lynx.bundle

# All showcase bundles (homepage + examples)
python3 lynx/explorer/showcase/build_and_copy.py
```

## Bundle Naming Convention

Bundles use **nested paths** (not flat): `showcase/menu/main.lynx.bundle`, `showcase/css/bg.lynx.bundle`, etc. This is required because multiple npm example packages produce bundles with the same name (e.g., `main.lynx.bundle`), and the directory path disambiguates them.

## Deeplinks

The app supports `lynx://` deeplinks:
- `lynx://open?url=<template_url>` — open any template URL
- `lynx://lynxview_page?bundle=<bundle_path>` — open a local bundle (e.g., `lynx://lynxview_page?bundle=showcase/menu/main.lynx.bundle`)

## Platform-Specific Build Docs

- iOS: `darwin/ios/README.md`
- Android: `android/README.md`
