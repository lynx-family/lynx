# Lynx Explorer

The official app for testing and exploring Lynx. It is featured in the Lynx Quick Start
Guide at <https://lynxjs.org/guide/start/quick-start.html>.

The directory has two main parts:

1. **Native shell apps** (Android / iOS / Harmony / Windows / macOS) that host the Lynx
   runtime and provide the OS-level entry point.
2. **ReactLynx screens** (TypeScript) that run inside those native apps and make up the
   visible UI.

---

## Native Shell Apps

Each platform is a self-contained project. Refer to the platform guide for build
instructions and system requirements:

| Platform       | Directory          | Build Guide                                    |
|----------------|--------------------|------------------------------------------------|
| Android        | `android/`         | [Android Build Guide](android/README.md)       |
| iOS            | `darwin/ios/`      | [iOS Build Guide](darwin/ios/README.md)        |
| macOS          | `darwin/macos/`    | [macOS Build Guide](darwin/macos/README.md)    |
| HarmonyOS      | `harmony/`         | [Harmony Build Guide](harmony/README.md)       |
| Windows        | `windows/`         | [Windows Build Guide](windows/README.md)       |

---

## ReactLynx Screens (JS development)

If you already have a running Lynx Explorer app (or any Lynx-integrated host), you can
iterate on the JS screens independently without rebuilding the native app.

### Package structure

The JS side is split into **three separate pnpm packages**, each with its own
`package.json` and `pnpm-lock.yaml`:

| Directory        | Screen          | What it contains                                               |
|------------------|-----------------|----------------------------------------------------------------|
| `homepage/`      | Home screen     | Entry point UI: nav cards, settings, scan QR code             |
| `showcase/menu/` | Showcase screen | Menu that launches Lynx-examples demos                         |
| `lib/`           | Shared library  | Shared utilities and context (e.g. `AppContext`, `navigation`) |

> `showcase/` is a pnpm workspace that includes `showcase/menu/` as its only member.
> Run `pnpm install` inside each package separately; there is no root-level lockfile
> that covers all three.

### Building a screen

```bash
# Home screen
cd explorer/homepage
pnpm install --frozen-lockfile
pnpm build              # outputs to dist/

# Showcase menu
cd explorer/showcase/menu
pnpm install --frozen-lockfile
pnpm build
```

The build output (`.lynx.bundle` files) is then loaded by the native app at runtime.
For Android the assets live under `android/lynx_explorer/src/main/assets/`; the
per-platform paths are managed by the scripts below.

### Build scripts

| Script                          | What it does                                                                                   |
|---------------------------------|-----------------------------------------------------------------------------------------------|
| `homepage/build.py`             | Installs deps and builds the home screen bundle, then copies it to the Android assets dir.    |
| `showcase/build_and_copy.py`    | Builds the showcase menu, then distributes all `.lynx.bundle` files from `lynx-examples` to **every** platform's asset directory (Android, iOS, Harmony, Windows, macOS). |

Run these from the repo root via the Gradle `buildHomePage` / `buildShowcase` tasks, or
directly with Python:

```bash
python3 explorer/homepage/build.py
python3 explorer/showcase/build_and_copy.py
```
