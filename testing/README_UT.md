# Lynx Unit Test

## C++ Unit Test
For the C++ unit tests of Lynx, test cases are written using the [Google Test testing framework](https://github.com/google/googletest). The build dependencies are managed through [GN](https://gn.googlesource.com/gn/+/main/docs/reference.md), and the test process is managed using the [RTF](../tools/rtf/README.md) tool.

### Edit and Submit
1. *Select an appropriate location for the test code.*
Lynx follows the specification that the test code should be in the same directory as the source file. Meanwhile, it requires that the name of the test file should be named by adding the "unittests" suffix to the name of the file being tested. 
Therefore, if you want to add tests to a certain source file, first check whether there is a corresponding test file in the directory where the source file is located. If it exists, you can just add tests in the corresponding file. If it doesn't exist, you need to create the corresponding test file in the same directory. 
For example, if you want to test the `hello.cc` file, you should create a `hello_unittests.cc` file in the same directory.

2. *Write test cases based on Google Test*

```c++
#include "third_party/googletest/googletest/include/gtest/gtest.h"
// optional: If the capabilities of gmock need to be used.
#include "third_party/googletest/googlemock/include/gmock/gmock.h" 

TEST(HelloTest, HelloWorld) {
  EXPECT_EQ(1, 1);
}
```

2. *Add test file to gn tree*
If you have made modifications in the existing unit test files, you can skip this section.
If you have added a brand-new unit test file, you need to follow the instructions in this section to add the newly added unit test file to the GN file.
You can find the BUILD.gn file in the directory where the source file is located or in its parent directory, and it contains the following configurations.
Here, the "example_testset" stores the collection of all test files of a unified module through the "sources" field, while the "example_unittest_exec" can be built to generate an executable file that contains all the test files of the module and is used for test verification. 
```bazel
unittest_set("example_testset") {
  # ...
  sources = [
    # ....
    "hello_unittests.cc",
  ]
  # ...
}

unittest_exec("example_unittest_exec") {
  deps = [ ":hello_testset" ]
}
```
Sometimes, you may think that hello_unittests doesn't belong to any existing test modules. In that case, please be brave and create your own testset and exec.

```bazel
unittest_set("hello_testset") {
  #...
  sources = [
    "hello_unittests.cc",
  ]
  #...
}

unittest_exec("hello_unittest_exec") {
  deps = [ ":hello_testset" ]
}
```

### Build and Run
The build and run process of Lynx unit tests is managed by the RTF tool. For more details, please refer to the [RTF tool README.md](../tools/rtf/README.md).
1. Add build targets.
If you didn't create a new exec in the previous steps, you can skip this step.
If a new exec has been created, it needs to be explicitly registered in the RTF configuration file.The RTF configuration file is located in the.rtf directory under the root directory.
```python
#.rtf/native-ut-lynx.template
targets({
    #....
    "hello_unittest_exec": {
        "cwd": ".",
        "owners":["YourName"],
        "coverage": True,
        "enable_parallel":True,
    },
    #....
}
)
```
2. Build and run.
```bash
cd your_project_path
tools/rtf/rtf native-ut run --names lynx --target hello_unittest_exec
```

## Android Unit Test
Unit tests for all Java and Kotlin files within the Lynx project are implemented using [instrumentation testing](https://developer.android.com/training/testing/instrumented-tests).

### Write test cases
The Android unit test framework is based on [JUnit4](https://github.com/junit-team/junit4/wiki/Getting-started). You can learn how to write test cases on its official website.


### Run test cases by IDE
IDE tools like Android Studio support running instrumentation tests with just one click. For specific solutions, you can refer to the introductions on the [official website](https://developer.android.com/studio/test/test-in-android-studio). 

### Run test cases by RTF
The RTF tool also supports the management and running of Android unit tests, and its configuration file is located in the `project_root_path/.rtf/android-ut-lynx.template` file.

#### On the emulator
```bash
tools/rtf/rtf android-ut run --name lynx
```
#### On a real device
```bash
tools/rtf/rtf android-ut run --name lynx --rmd
```

## iOS Unit Test
The iOS unit tests of Lynx are built based on the native XCTest unit test and the Xcode compilation system of iOS, targeting all *.m files in the Lynx project. 

### Write test cases
You can check the [official website](https://developer.apple.com/documentation/xctest) and use XCTest to write test cases.

### Run test cases by Xcode
The running of Lynx iOS unit tests is implemented based on the Xcode system. Based on this, we have integrated all Lynx-related iOS unit tests in the form of source code dependencies in the LynxExplorer App. You only need to use the Xcode project of LynxExplorer to run all the tests. 

1. Initialize the environment and sync dependencies.
```bash
source tools/envsetup.sh
tools/hab sync .
```

2. Gen Xcode workspace
```bash
cd explorer/darwin/ios/LynxExplorer
./bundle_install.sh
open LynxExplorer.xcworkspace
```

3. Select LynxExplorerUnitTest schema and run
