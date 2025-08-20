
# Building Lynx Explorer for Harmony

This document provides instructions for building the Lynx Explorer Harmony app from source.

## System Requirements

- 100GB or more of disk space
- Git installed
- Git/Python3(>=3.9) installed
- DevEco Studio 5.0.13.200 or later, you can download the latest version from the official DevEco Studio download page

## Install Dependencies

The following dependencies are needed:

- HarmonyOS SDK

### Configure HarmonyOS SDK

Add the `HARMONY_HOME` variable to your environment configuration file (it may be `~/.zshrc` or `~/.bash_profile` or `~/.bashrc`, depending on your terminal environment).

```bash
export HARMONY_HOME=/Applications/DevEco-Studio.app/Contents/sdk/
```

Also, ensure the `hdc` and `ohpm` tools are in your `PATH`.
- `hdc` is located at `$HARMONY_HOME/default/openharmony/toolchains/hdc`.
- `ohpm` is located at `/Applications/DevEco-Studio.app/Contents/tools/ohpm/bin`.

Add the following lines to your shell configuration file:

```bash
export PATH=/Applications/DevEco-Studio.app/Contents/tools/ohpm/bin:$HARMONY_HOME/default/openharmony/toolchains:$PATH
```

## Get the Code

### Pull the Repository

Pull the code from the Github repository.

```bash
git clone https://github.com/lynx-family/lynx.git
```

### Get the Dependent Files

After getting the project repository, execute the following commands in the root directory of the project to get the project dependent files.

```bash
cd lynx
source tools/envsetup.sh
tools/hab sync .
```

## Build and Run

You can compile LynxExplorer through the command line terminal or DevEco Studio. The following two methods are introduced respectively.

### Method 1: Build and Run using DevEco Studio

#### Open the Project

Use DevEco Studio to open the `explorer/harmony` directory of the project.

#### Build and Run

Select the `lynx_explorer` module and click the `Run` button to experience LynxExplorer on your device.

If you want to skip building the Lynx core libraries and the bundles in subsequent builds, you can select `skipBundle` in BuildMode.

### Method 2: Build and Run using the Command Line

Use the following python script to build the Lynx Explorer for Harmony. This will build the Lynx core libraries, the bundles, and finally the HAP.

Add the `DEVECO_SDK_HOME` variable to your environment configuration file.

```bash
export DEVECO_SDK_HOME=/Applications/DevEco-Studio.app/Contents/sdk
```

Also, ensure the `hvigorw` tool is in your `PATH`.

```bash
export PATH=/Applications/DevEco-Studio.app/Contents/tools/hvigor/bin:$PATH
```

#### Install Harmony Dependencies

Execute the following commands to install Harmony-specific dependencies using `ohpm`.

```bash
pushd platform/harmony && ohpm install && popd
pushd explorer/harmony && ohpm install && popd
```

#### Build and Install

If you want to build a release version, you can remove the `--debug` option, if you don't want to build a development version, remove the `--dev` option.

```bash
python3 explorer/harmony/script/build.py --debug --dev --build_lynx_core --build_bundle --build_hap
```

After a successful build, you can use DevEco Studio or the `hdc` command-line tool to install it on a **harmony virtual device**.

```
hdc install explorer/harmony/lynx_explorer/build/default/outputs/default/lynx_explorer-default-unsigned.hap
```
