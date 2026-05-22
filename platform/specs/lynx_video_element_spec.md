# Lynx `<video>` Element Spec

## Intro

Lynx needs an open source `<video>` element for video playback. For now, this spec focuses on implementations built on top of the native video capabilities of Android, iOS, and Harmony, with a unified abstract interface across platforms.

## API

### Attributes

- `src`: Video source URL. Only online network URLs are supported. After `src` changes, playback of the previous `src` stops immediately and the player tries to render the first frame of the new `src`. If `src` is invalid  or fails to load, playback of the previous `src` still stops, and `binderror` should be dispatched.
- `loop`: Whether to loop playback. Defaults to `false`.
- `volume`: Volume from 0 to 1. Defaults to `1.0`.
- `muted`: Whether the video is muted. Defaults to `false`. This is independent from `volume`; after unmuting, the previous volume is restored.
- `speed`: Playback speed from 0.1 to 2.0. Defaults to `1.0`.
- `object-fit`: Video scaling strategy. Semantics are consistent with CSS. Supported values are `contain`, `cover`, and `fill`. Defaults to `contain`.
- `mode`: UIMethod execution mode. Operations such as `play` and `stop` are asynchronous. Supported values:
  - `queue`: Execute operations in queue order and wait for the previous operation callback to be invoked. For example, after calling `play`, the next operation runs only after playback actually starts and the `bindplaying` event is dispatched.
  - `direct`: Execute immediately without waiting for the callback for previous operation to be dispatched.
  - `latest`: If the callback for previous operation has not been dispatched, keep only the latest pending operation and overwrite earlier pending operations.
  Defaults to `queue`.
- `timeupdate-interval`: Minimum dispatch interval for the `bindtimeupdate` event, in seconds. Defaults to `0.33`.

### UIMethods

- `play`: Play the video. Requires a callback.
- `stop`: Stop video playback. Requires a callback.
- `pause`: Pause video playback. Requires a callback.
- `seek`: The `position` parameter is the target playback position in seconds. After seeking, preserve the previous playing or paused state. Requires a callback. If the target position is out of range, callback with an error.

### Events

- `bindfirstframe`: Fired when the first video frame has loaded. If a seek operation occurs before the first frame, this event fires when the first frame after the seek is available. The parameter is `duration`, the total video duration in seconds.
- `bindplaying`: Fired when video playback starts. This fires for the first playback and when playback resumes after a pause. It does not fire again when looping automatically restarts playback from the beginning.
- `bindpaused`: Fired when video playback pauses.
- `bindstopped`: Fired only when video playback is stopped by the `stop` method.
- `bindtimeupdate`: Fired when the playback position updates. Parameters are `current` and `duration`, both in seconds.
- `bindended`: Fired when video playback fully ends. This does not fire when `loop = true`.
- `bindlooped`: Fired at the end of each loop iteration. When `loop = true`, it fires each time playback reaches the end and before playback automatically returns to the beginning. It does not fire when `loop = false`.
- `binderror`: Fired when a video playback error occurs. Parameters are `errorCode` and `errorMsg`. Keep these values as consistent as possible across the three platforms.
- `bindbuffering`: Fired while the video is buffering. The parameter is `buffering`, which represents the buffered end position on the timeline in seconds (i.e. the maximum playable position at the moment).

## Reference

Official custom Element documentation for iOS:

https://lynxjs.org/next/guide/custom-native-component.html?platform=ios

Official custom Element documentation for Harmony:

https://lynxjs.org/next/guide/custom-native-component.html?platform=harmony

Official custom Element documentation for Android:

https://lynxjs.org/next/guide/custom-native-component.html?platform=android

iOS native Video documentation:

- AVKit / AVPlayerViewController standard playback UI:
  https://developer.apple.com/documentation/AVKit/playing-video-content-in-a-standard-user-interface
- AVPlayerViewController API:
  https://developer.apple.com/documentation/AVKit/AVPlayerViewController
- AVPlayer API:
  https://developer.apple.com/documentation/avfoundation/avplayer

Android native Video documentation:

- Media3 ExoPlayer:
  https://developer.android.com/media/media3/exoplayer

## Detailed Code Design

- Two-layer architecture:
  - The first layer is `LynxUIVideo`, which inherits from `LynxUI` on Android and iOS and is implemented in ETS on Harmony. It handles all Lynx-layer attributes, events, and methods, keeps cross-platform behavior consistent and clean, and does not own the details of the concrete video capability. It only defines the abstract `LynxVideoPlayable` interface.
  - The second layer is `LynxVideoView`, which implements `LynxVideoPlayable`, connects to the underlying platform player, and handles all player details.

## Other Rules

- The open source `<video>` implementation should live under the `lynx_xelement` directory in `platform`, and each platform should register it in the `lynx_explorer` App.
- Typings should live in `js_libraries/types/types/common/element` in a file named `video.d.ts`. Add test cases and refer to other elements for the expected pattern.
- All time units are seconds.
- Audio focus management: the first version does not expose audio focus controls to the Lynx layer. Each platform player should use the platform default behavior internally. Android requests long-term focus by default, and iOS uses the Playback Category by default.

## Testing

### E2E Coverage Requirements

E2E demos must cover all the APIs, exercise as many combinations as possible, use the ReactLynx demo as the E2E case, and capture screenshots as verification evidence.

### UnitTest

#### Key Coverage

- Attribute updates: `src`, `loop`, `volume`, `muted`, `speed`, `object-fit`, `mode`, and `timeupdate-interval` synchronize correctly to the underlying `LynxVideoPlayable`.
- UIMethods: Success and failure callback paths for `play`, `pause`, `stop`, and `seek`, plus the `queue`, `direct`, and `latest` execution modes.
- Event dispatch: Parameters and trigger timing for `bindfirstframe`, `bindplaying`, `bindpaused`, `bindstopped`, `bindtimeupdate`, `bindended`, `bindlooped`, `binderror`, and `bindbuffering`.
- Boundary behavior: Invalid `src`, out-of-range `seek`, stopping the previous playback when switching `src`, and suppressing `bindended` when `loop = true`.

#### Android

##### Overview

Use this workflow to add and execute Android UI E2E tests for Lynx xelement components. The supported path is the repo integration-test framework under `testing/integration_test`, running against `LynxExplorer` with Appium Espresso.

Before proposing significant changes, read repo guidance:

- `AGENTS.md`
- Relevant files under `agents/`
- `testing/integration_test/README.md`
- `testing/integration_test/ENV_SETUP.md`

##### Where Tests Live

Use these locations:

- Demo page source: `testing/integration_test/demo_pages/<page_name>/`
- Case set: `testing/integration_test/test_script/case_sets/<suite_name>/`
- Android suite entry: `testing/integration_test/test_script/android_test/<suite_name>.py`
- Built/copy target: `explorer/android/lynx_explorer/src/main/assets/automation/<page_name>/main.lynx.bundle`

Prefer one suite for xelement coverage:

- `testing/integration_test/test_script/case_sets/xelement/`
- `testing/integration_test/test_script/android_test/xelement.py`

##### Add A Demo Page

Create a package under `testing/integration_test/demo_pages/<page_name>/`:

```text
package.json
lynx.config.mjs
index.tsx
index.css
```

The page must expose stable `lynx-test-tag` values for automation. Example:

```tsx
<text lynx-test-tag="auto-result">{autoResult}</text>
<view lynx-test-tag="auto-start-refresh" bindtap={handleAutoStart} />
<refresh lynx-test-tag="refresh-root" bindstartrefresh={handleStartRefresh}>
```

Register the package in `testing/integration_test/demo_pages/pnpm-workspace.yaml`:

```yaml
packages:
  - 'dom-focus'
  - 'event'
  - '<page_name>'
```

Update the demo lockfile:

```bash
cd testing/integration_test/demo_pages
pnpm install --lockfile-only
```

Keep lockfile churn scoped to the new importer if possible.

##### Add A Case Set

Create:

```text
testing/integration_test/test_script/case_sets/xelement/
  __init__.py
  runner.py
  <CaseName>.py
```

Use the existing `core/runner.py` pattern:

```python
import os
import sys

from lynx_e2e.api.config import settings

sys.path.append(settings.PROJECT_ROOT)

from lib.test_runner.case_set import CaseSet
from lib.test_runner.test_runner import TestRunner
from lib.test_runner.plugins.devtool_connect_plugin import DevtoolConnectPlugin


def run(test):
    runner = TestRunner(test)
    runner.add_plugin(DevtoolConnectPlugin(test))
    runner.add_case(CaseSet(case_set_path=os.path.dirname(__file__)))
    runner.run_test()
```

Each case file needs `config` and `run(test)`:

```python
from lynx_e2e.api.lynx_view import LynxView


config = {
    "type": "custom",
    "path": "automation/<page_name>/main",
    "platform": "android",
}


def run(test):
    lynxview = test.app.get_lynxview("lynxview", LynxView)
    target = lynxview.get_by_test_tag("<tag>")
    test.wait_for_equal("message", target, "text", "success")
```

For gesture tests, prefer Android shell input when component behavior depends on native touch handling:

```python
rect = refresh.rect
x = int(rect.left + rect.width / 2)
start_y = int(rect.top + 120)
end_y = int(min(rect.bottom - 20, start_y + 420))
test.device.shell_command("input swipe %d %d %d %d 700" % (x, start_y, x, end_y))
```

##### Add Android Suite Entry

Create `testing/integration_test/test_script/android_test/xelement.py`:

```python
import os
import sys

search_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(search_path)

from lib.android.test import LynxTest
from case_sets.xelement.runner import run


class xelementTest(LynxTest):
    timeout = 1800

    def run_test(self, test=None):
        if test is None:
            test = self
        test.start_step("--------Test: start to test xelement;-------")
        run(test=test)


if __name__ == "__main__":
    xelementTest.debug_run()
```

The run target is:

```bash
python3 manage.py runtest android_test.xelement
```

##### Environment Setup

From repo root:

```bash
source tools/envsetup.sh
tools/hab sync . -f
python3 tools/android_tools/prepare_android_build.py
```

Do not use only:

```bash
tools/hab sync . -f --target dev --target-only
```

That target-only sync may fetch `buildtools/android_sdk_manager` but miss base `buildtools/gn`, `buildtools/ninja`, `buildtools/node`, and `buildtools/corepack`, causing `Failed to find buildtools`.

##### Build Demo Bundles

Build and copy integration demo resources:

```bash
source tools/envsetup.sh
python3 testing/integration_test/demo_pages/build_and_copy.py
```

Focused page build:

```bash
cd testing/integration_test/demo_pages
pnpm --filter @demo_pages/<page_name> run build
```

The integration copy script should produce:

```text
explorer/android/lynx_explorer/src/main/assets/automation/<page_name>/main.lynx.bundle
```

Generated `dist/`, `node_modules/`, Explorer resource bundles, `buildtools/`, `build/`, `out/`, `.cxx/`, and `CMakeLists_impl/` are build artifacts unless explicitly intended.

##### Build LynxExplorer

From repo root:

```bash
source tools/envsetup.sh
cd explorer/android
./gradlew :LynxExplorer:assembleNoAsanDebug -PIntegrationTest --no-daemon --stacktrace
```

Expected APK:

```text
explorer/android/lynx_explorer/build/outputs/apk/noasan/debug/LynxExplorer-noasan-debug.apk
```

##### Start Appium And Emulator

Start Appium:

```bash
appium --port 4723
```

Check drivers:

```bash
appium driver list --installed
appium driver install espresso
```

Start an emulator:

```bash
$ANDROID_HOME/emulator/emulator -list-avds
$ANDROID_HOME/emulator/emulator -avd <AVD_NAME> -netdelay none -netspeed full
```

Wait for boot:

```bash
adb wait-for-device
adb shell 'while [ "$(getprop sys.boot_completed)" != "1" ]; do sleep 1; done; input keyevent 82'
```

##### Sign And Install For Espresso

Espresso instrumentation requires the target APK and `io.appium.espressoserver.test` to have matching signatures. If the run fails with:

```text
Permission Denial: starting instrumentation ... io.appium.espressoserver.test does not have a signature matching the target com.lynx.explorer
```

re-sign the Explorer APK with Appium's Espresso test key:

```bash
$ANDROID_HOME/build-tools/34.0.0/apksigner sign \
  --key node_modules/.pnpm/appium-adb@12.13.1/node_modules/appium-adb/keys/testkey.pk8 \
  --cert node_modules/.pnpm/appium-adb@12.13.1/node_modules/appium-adb/keys/testkey.x509.pem \
  --out /tmp/LynxExplorer-noasan-debug-espresso-testkey.apk \
  explorer/android/lynx_explorer/build/outputs/apk/noasan/debug/LynxExplorer-noasan-debug.apk
```

Install it:

```bash
adb uninstall com.lynx.explorer || true
adb install -r /tmp/LynxExplorer-noasan-debug-espresso-testkey.apk
```

Confirm signature match if needed:

```bash
adb shell dumpsys package com.lynx.explorer | rg 'signatures=|apkSigningVersion|versionCode'
adb shell dumpsys package io.appium.espressoserver.test | rg 'signatures=|apkSigningVersion|versionCode'
```

##### Run The E2E Suite

Install Python dependencies if `lynx_e2e` is missing:

```bash
python3 -m venv /tmp/lynx-e2e-venv
/tmp/lynx-e2e-venv/bin/python -m pip install -r testing/integration_test/test_script/requirements.txt
```

Run:

```bash
cd testing/integration_test/test_script
platform=android /tmp/lynx-e2e-venv/bin/python manage.py runtest android_test.xelement
```

`platform=android` is required. Without it, `lynx_e2e.api.testcase` may fail during import with:

```text
AttributeError: 'NoneType' object has no attribute 'lower'
```

Expected pass summary:

```text
Totals: 1
Passed: 1
Failed: 0
```

##### Debugging Checklist

- `Failed to find buildtools`: run full `tools/hab sync . -f`, then re-source `tools/envsetup.sh`.
- `No module named lynx_e2e`: install `testing/integration_test/test_script/requirements.txt` into a venv.
- `platform.lower()` `NoneType`: set `platform=android`.
- Espresso server terminated: inspect logcat first:

```bash
adb logcat -d -v time | rg -i 'espresso|lynx|fatal|exception|crash|AndroidRuntime|instrumentation|Permission Denial'
```

- Signature mismatch: re-sign with Appium Espresso `testkey.pk8` and `testkey.x509.pem`.
- No devices: start emulator, then confirm `adb devices`.
- Test page not found: verify `build_and_copy.py` copied `automation/<page_name>/main.lynx.bundle`.
- Test tags not found: check `lynx-test-tag` in page source and avoid duplicate tags unless the test intentionally selects the first match.

##### Final Verification

Before reporting completion, run:

```bash
python3 -B -m py_compile testing/integration_test/test_script/android_test/xelement.py \
  testing/integration_test/test_script/case_sets/xelement/runner.py \
  testing/integration_test/test_script/case_sets/xelement/<CaseName>.py

cd testing/integration_test/demo_pages
pnpm --filter @demo_pages/<page_name> run build

source tools/envsetup.sh
python3 testing/integration_test/demo_pages/build_and_copy.py

cd explorer/android
./gradlew :LynxExplorer:assembleNoAsanDebug -PIntegrationTest --no-daemon --stacktrace

cd ../../testing/integration_test/test_script
platform=android /tmp/lynx-e2e-venv/bin/python manage.py runtest android_test.xelement
```

Then clean generated artifacts and check:

```bash
git status --short
git diff --check
```

#### iOS

Add iOS XElement testspec unit tests. Put test files under `platform/darwin/ios/lynx_xelement/video/` and register them in the `UnitTests` testspec in `platform/darwin/ios/lynx_xelement/BUILD.gn`. Use the `XElement-Unit-UnitTests` target. Select this scheme from `explorer/darwin/ios/lynx_explorer/LynxExplorer.xcworkspace` to run it. Before generating the project for the first time, run `tools/hab sync .` from the repo root, then run `./bundle_install.sh` in `explorer/darwin/ios/lynx_explorer`.

If the current project does not yet have a runnable test scheme or CocoaPods testspec, add the test project integration first:

- Add a `UnitTests` test subspec to `XElement_podspec` in `platform/darwin/ios/lynx_xelement/BUILD.gn`, and add the video unit test source files to `sources`.
- Add `:testspecs => [ 'UnitTests' ]` to `pod 'XElement'` in `explorer/darwin/ios/lynx_explorer/Podfile`, so `pod install` generates `XElement-Unit-UnitTests`.
- Run `./bundle_install.sh` or `pod install` again, then open `LynxExplorer.xcworkspace` and confirm that `XElement-Unit-UnitTests` exists in the Xcode scheme list.

Command-line regression can use:

```bash
tools/hab sync .
source tools/envsetup.sh

cd explorer/darwin/ios/lynx_explorer
./bundle_install.sh
SIMULATOR_ID=$(xcrun simctl list devices available | grep "iPhone" | head -n 1 | awk -F'[()]' '{print $2}')
xcodebuild test \
  ARCHS=x86_64 \
  -project Pods/XElement.xcodeproj \
  -scheme XElement-Unit-UnitTests \
  -configuration Debug \
  -sdk iphonesimulator \
  COMPILER_INDEX_STORE_ENABLE=NO \
  -destination "platform=iOS Simulator,id=${SIMULATOR_ID},arch=x86_64" \
  -only-testing:XElement-Unit-UnitTests/LynxUIVideoUnitTest
```

#### Harmony

Verified with `platform/harmony/lynx_xelement/svg`: ETS for Harmony xelement can add `ohosTest` unit tests. HAR modules cannot use `assembleHar` to build the test target. Use `genOnDeviceTestHap` to generate the on-device test HAP.

Test project integration:

- Add `ohosTest` to `targets` in the corresponding HAR `build-profile.json5`.
- Add `src/ohosTest/module.json5` under the HAR. Use `feature` as the module type.
- Put Hypium tests under `src/ohosTest/ets/test/`, and use `List.test.ets` to collect the test entries.
- Add `@ohos/hypium` to `devDependencies` in `explorer/harmony/oh-package.json5`. Otherwise `OhosTestCompileArkTS` reports `Failed to resolve OhmUrl` for `@ohos/hypium`.

Suitable ETS UnitTest coverage includes logic that does not depend on the real ArkUI render tree, device player, or Lynx runtime registration, such as attribute parsing, internal size updates after layout, the UIMethod scheduling state machine, and event payload assembly. When `UIBase` context is required, use a test mock that implements `IContext`. Keep real player behavior, ArkUI rendering, Lynx page registration, and end-to-end event timing in Explorer `ohosTest` or the E2E demo.

Commands verified with SVG:

```bash
source tools/envsetup.sh --target harmony
tools/hab sync .

pushd platform/harmony && ohpm install && popd
pushd explorer/harmony && ohpm install && popd

cd explorer/harmony
hvigorw --no-daemon genOnDeviceTestHap --mode module -p module=xelement_svg@ohosTest

hdc install -r platform/harmony/lynx_xelement/svg/build/default/outputs/ohosTest/xelement_svg-ohosTest-unsigned.hap
hdc shell aa test -b org.lynxjs.explorer -m xelement_svg_test -s unittest OpenHarmonyTestRunner -w 60000
```

Verification result: this passed locally on a `Mate 70 Pro` Harmony emulator. `hdc list targets` detected `127.0.0.1:5555`, and `aa test` returned `Tests run: 2, Failure: 0, Error: 0, Pass: 2`. When `<video>` is integrated later, replace the module name with the actual `xelement_video@ohosTest`.

Note: when HAR-level `ohosTest` directly imports UI classes that inherit from `UIBase`, it also pulls in `@lynx/lynx` and native Lynx `.so` dependencies. In the current SVG verification, importing `UISVG` too early caused the process to exit because the test HAP did not include `liblynx.so` / `liblynxbase.so`. Therefore, it is better to split unit-testable pure logic into independent ETS helpers and cover them with HAR `ohosTest`. Put flows that really depend on the Lynx runtime, ArkUI tree, and player in Explorer `ohosTest` or the E2E demo.
