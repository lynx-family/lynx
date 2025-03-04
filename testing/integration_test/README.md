# Integration test

This package contains self-driving testing for Lynx on devices and emulators. It is driven by the [Lynx-E2E](https://pypi.org/project/lynx-e2e-appium/) framework which is a self-developed UI automation framework of Lynx.

## Environment Setup

Please refer to [ENV_SETUP.md](ENV_SETUP.md) for environment setup.

## Run Test

The integration tests are run based on the Explorer. You can use a real device or an emulator to run them.

### Build Explorer with demo pages

#### For Android
Enter the `explorer/android` directory from the project root directory and execute the following command

```
./gradlew :LynxExplorer:assembleNoAsanDebug -PIntegrationTest
```

#### For iOS
1. Install iOS project dependencies
```
cd explorer/darwin/ios/lynx_explorer
./bundle_install.sh --integration-test
```
2. Open `LynxExplorer.xcworkspace` by Xcode.
3. Select `LynxExplorer` to execute the build in Xcode.

### Run the specified test suite

All casesets are stored in `testing/integration_test/test_script/case_sets`. You can enter the `testing/integration_test/test_script` directory from the project root directory and execute the following command to run the `core` test suite.

```bash
# run on android
python3 manage.py runtest android_test.core
# run on ios
python3 manage.py runtest ios_test.core
```

If you want to run other test suite, you only need to replace `core` with the folder name of the test suite.

## Add Test

If you want to contribute a new test case or test suite to the Lynx repository, please refer to the following steps.

### Add a new test suite

First, create a new directory in the `testing/integration_test/test_script/case_sets` directory. The directory name should be the same as the test suite name.

Then, copy the `runner.py` file in the `testing/integration_test/test_script/case_sets/core` folder to the current folder. This file is used to run the test suite.

After that, write the test script file in the current test suite folder. You can refer to the next subsection `Add a new case` to add a test script.

### Add a new case

First, create a new file in the `testing/integration_test/test_script/case_sets/<test_suite>` directory. The file name should be the same as the case name.

Then, write the test script running based on the Lynx-E2E framework. 

The following is a simplest test case script used to locate the button on the page and click it. Then, it asserts whether the output text is incremented by one.

```python
from lynx_e2e.api.lynx_view import LynxView

config = {
    "type": "custom",
    "path": "automation/event"
}
def run(test):
    lynxview = self.app.get_lynxview('lynxview', LynxView)

    count = lynxview.get_by_test_tag("count")
    button = lynxview.get_by_test_tag("button")
    button.click()
    test.wait_for_equal('setData failed', count_view, 'text', '1')
```

In the above script, "config" is used to specify the basic information of the current test case, including the script type, the page opening path, etc.

You can set the `type` in `config` to `check_image` to directly conduct screenshot comparison. Or set it to `check_result` to assert whether the output in the page is success. In the above script, the `type` is set to `custom` to run the test script writing in `run()` function.

The page opened by the test case will automatically have a prefix spliced in the format as `file://lynx?local://<test_script_path>.js?width=720&height=1280&density=320`. You can use the `path` in `config` to specify the path of the test script.