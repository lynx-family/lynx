# Setup Execution Environment

Integration test uses [Lynx-E2E](https://pypi.org/project/lynx-e2e-appium) as the execution framework and [Appium](http://appium.io/docs/en/latest/) as the underlying execution engine. Therefore, before execution, it is necessary to first set up the local Appium environment.

## Install Appium

```bash
# First execute the environment preparation in the root directory of Lynx
source tools/envsetup.sh
tools/hab sync -f .
# Install Appium by npm
npm install -g appium@2.11.2
# Install appium-doctor
npm install -g appium-doctor
# Use appium-doctor to check if the environment is set up successfully.
appium-doctor
```

Check the appium driver list.
```bash
appium driver list --installed
✔ Listing installed drivers
- xcuitest@7.24.18 [installed (npm)]
- uiautomator2@3.7.7 [installed (npm)]
- espresso@3.3.1 [installed (npm)]
```

The above three drivers are essential for running. If they do not exist locally, install them using the following commands:

```bash
appium driver install xcuitest
appium driver install uiautomator2
appium driver install espresso
```

## Prepare for Android execution

- Set the `ANDROID_SDK_ROOT` environment variable

```bash
export ANDROID_SDK_ROOT=<path-to-android-sdk>
```

- Resign the APK using Espresso

```bash
java -jar $ANDROID_SDK_ROOT/build-tools/34.0.0/lib/apksigner.jar sign --key $HOME/.appium/node_modules/appium-espresso-driver/node_modules/appium-adb/keys/testkey.pk8 --cert $HOME/.appium/node_modules/appium-espresso-driver/node_modules/appium-adb/keys/testkey.x509.pem  --out <target-apk-path> <origin-apk-path>
```

## Prepare for iOS execution

On the iOS side, Appium uses [WebDriverAgent](https://github.com/appium/WebDriverAgent) as the WebDriver.

First, clone the WebDriverAgent repository to the local machine.

```bash
git clone https://github.com/appium/WebDriverAgent.git
```

Then, open `WebDriverAgent/WebDriverAgent.xcodeproj` in XCode. And modify following items before building:

- Bundle Identifier: Due to differences in signatures, the default Bundle Identifier cannot be used. It is recommended to add a custom suffix to the default value.
- Team: You need to switch to a personal developer account or a team account.

Finally, execute the Build operation and install WebDriverAgentRunner onto the device.

## Start Appium Server

```bash
appium --port 4723
# The Appium process is a persistent process and needs to be retained throughout the execution of test cases.
```
