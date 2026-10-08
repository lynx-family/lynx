# Building Lynx Explorer for iOS

This document provides instructions for building the Lynx Explorer iOS app from source. If you just want to try out Lynx, you can download the pre-built app from the releases page instead.

## System Requirements

- 100GB or more of disk space
- Git/Python3(>=3.9)/Ruby(>=2.7, <3.4) installed

## Install Dependencies

The following dependencies are needed:

- Xcode(>=15.0)
- Cocoapods(>=1.11.3)
- Python library

We recommend using [Homebrew](https://brew.sh/) to install the dependencies.

### Xcode

Lynx requires Xcode 15.0 or later. It is recommended to keep Xcode up to date. You can install or update it on the [App Store](https://developer.apple.com/xcode/).

- Open Xcode->Settings->Locations, to make sure the `Command Line Tools` are configured
- You can run `xcode-select -p` in the terminal, and if it prints a correct path, it's configure successfully.

### Python Library

The yaml dependency needs to be installed to execute some auto-generation logic.

```
# use the virtual environment to manage python environment
python3 -m venv venv
source venv/bin/activate
# install PyYAML package
pip3 install pyyaml
```

## Get the Source Code

### Pull the Repository

Pull the code from the Github repository.

```
git clone https://github.com/lynx-family/lynx.git
```

### Install Third-party Library

Run the following commands from the root of the repository to install the dependencies.

```
cd lynx
tools/hab sync .
source tools/envsetup.sh
```

## Build iOS App

1. Install iOS project dependencies
```
cd explorer/darwin/ios/lynx_explorer
./bundle_install.sh
```
2. After step 1, `LynxExplorer.xcworkspace` will be generated in the lynx_explorer directory. Open `LynxExplorer.xcworkspace` by Xcode.
3. Select `LynxExplorer` to execute the build in Xcode.

## Troubleshooting

### ProMotion and main-thread requestAnimationFrame

On iOS 15+ ProMotion displays, the first pending main-thread `requestAnimationFrame` callback requests the display's maximum cadence, using a preferred frame-rate range of 80 to that maximum. The preference remains active during callback execution and while callbacks registered for the next frame are pending. It returns to `CAFrameRateRangeDefault` when execution finishes without another request, the last request is cancelled, or the manager is destroyed. Cancelled callbacks do not count as pending. An already-requested native tick may still arrive to drain a cancelled batch through the existing VSync path.

A frame request expresses demand regardless of what its callback changes. There is no geometry allowlist, motion timeout, or requirement for consecutive requests. A continuous rAF FPS counter therefore measures callback cadence under an active rAF workload; it does not measure idle panel refresh rate or successfully presented frames. Stop unnecessary loops when measuring idle behavior or power.

The implementation uses existing VSync links and pauses them between requested ticks. It tracks each rAF manager separately, holds the monitor weakly, and updates the iOS preference on the main queue. Runtime teardown cancels pending callbacks even when a queued VSync still retains their manager. It does not create a new display link or timer. The existing engine `preferredFps: "high"` override and `low` animation throttle retain their behavior; the main-thread rAF monitor is separate from that engine setting.

Host apps still need `CADisableMinimumFrameDurationOnPhone = YES` in Info.plist on iPhone; Lynx Explorer already includes it. iOS decides the actual cadence according to device capabilities, power, thermal, and accessibility conditions. A preferred maximum is not a guarantee of 120 Hz or a particular first-callback latency. See [Apple's ProMotion guidance](https://developer.apple.com/documentation/quartzcore/optimizing-iphone-and-ipad-apps-to-support-promotion-displays).

### Using a Personal Team to Run on Device

By default, the project doesn't configure the "Team" for signing. If you want to change it to your Personal Team (associated with your Apple ID), follow these steps:

1. Open `LynxExplorer.xcworkspace` by Xcode.
2. Navigate to "Signing & Capabilities", and select your Personal Team under the "Team" dropdown.
3. Update the "Bundle Identifier" from `com.lynx.LynxExplorer` to a unique identifier like `com.<your-name>.LynxExplorer`. This step ensures the identifier is unique and available for your use.
4. Enable the "Automatically manage signing" option to allow Xcode to handle the app signing process automatically.
