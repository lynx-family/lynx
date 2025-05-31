# Building Lynx Explorer for Android

This document provides instructions for building the Lynx Explorer Android app from source. If you just want to try out Lynx, you can download the pre-built APK from the releases page instead.

## System Requirements

- 100GB or more of disk space
- Git/Python3(>=3.9) installed

## Install Dependencies

The following dependencies are needed:

- JDK 11
- Android development environment
- Python library

### JDK

#### Install JDK11

- MacOS

    We recommend using Homebrew to install the OpenJDK distribution called Zulu, which is provided by Azul.

    ```
    brew install --cask zulu@11
    ```

    You can use the following command to confirm whether the installation is successful.

    ```
    javac --version
    ```

    If the installation is successful, the terminal will output javac version number 11.

- Linux

    On Ubuntu or Debian:

    ```
    sudo apt install openjdk-11-jdk 
    ```

    On RHEL or CentOS:

    ```
    sudo yum install openjdk-11-jdk 
    ```

- Windows

    We recommend using winget to install the OpenJDK distribution.

    ```
    winget install -e --id ojdkbuild.openjdk.11.jdk
    ```

    You can use the following command to confirm whether the installation is successful.

    ```
    javac --version
    ```

    If the installation is successful, the terminal will output javac version number 11.x.xx.

#### Update JAVA_HOME

Confirm your JDK installation directory. If you follow the above steps, the JDK path is likely to be 

- `/Library/Java/JavaVirtualMachines/zulu-11.jdk/Contents/Home` on MacOS
- `/usr/lib/jvm/java-11-openjdk-amd64` on Linux
- `C:\Program Files\ojdkbuild\java-11-openjdk-11.0.15-1` on Windows.

Add the following statement to your environment configuration file (it may be ~/.zshrc or ~/.bash_profile or ~/.bashrc, depending on your terminal environment):

- MacOS

    ```
    export JAVA_HOME=/Library/Java/JavaVirtualMachines/zulu-11.jdk/Contents/Home
    export PATH=$JAVA_HOME/bin:$PATH
    ```

- Linux 

    ```
    export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
    export PATH=$JAVA_HOME/bin:$PATH
    ```

- Windows

    ```
    $JDK_PATH='C:\Program Files\ojdkbuild\java-11-openjdk-11.0.15-1'
    [Environment]::SetEnvironmentVariable('JAVA_HOME', $JDK_PATH, 'User')
    $EXISTING_PATH = [Environment]::GetEnvironmentVariable('PATH', 'User')
    [Environment]::SetEnvironmentVariable('PATH', "$JDK_PATH;$EXISTING_PATH", 'User')
    ```

### Android Development Environment

Configuring the Android development environment required by Lynx includes the following step:

#### Configure ANDROID_HOME

Add the ANDROID_HOME variable to your environment configuration file(maybe ~/.zshrc or ~/.bash_profile or ~/.bashrc, depending on your terminal environment).

If you have installed the Android SDK before, please set ANDROID_HOME to the installation directory of the Android SDK.(If you have previously installed Android SDK by Android Studio, the installation path of the Android SDK is usually located at /Users/your-username/Library/Android/sdk on MacOS and Linux or C:\Users\Admin\AppData\Local\Android\Sdk on Windows)

If you have NOT installed the Android SDK before, you can set ANDROID_HOME to the path where you want the Android SDK to be installed. We have tools to help you install the Android SDK to ANDROID_HOME.

- MacOS and Linux

    ```
    export ANDROID_HOME=<path-to-android-sdk>
    ```

- Windows

    ```
    [Environment]::SetEnvironmentVariable('ANDROID_HOME', $path-to-android-sdk, 'User')
    ```

### Python Library

We recommend using pyenv to manage python environment.
To install pyenv: 

- [`https://github.com/pyenv/pyenv`](https://github.com/pyenv/pyenv) on MacOS and Linux.
- [`https://github.com/pyenv-win/pyenv-win`](https://github.com/pyenv-win/pyenv-win) on Windows.

Install python with version higher or equal to 3.9 using pyenv:  

```
pyenv install 3.9 # or higher
pyenv global 3.9 # or higher
```

## Get the Code

### Pull the Repository

Pull the code from the Github repository and specify the path(`src/lynx`) to avoid contaminating the directory when installing dependencies.

```
git clone https://github.com/lynx-family/lynx.git src/lynx
```

### Get the Dependent Files

After getting the project repository, execute the following commands in the root directory of the project to get the project dependent files.

- MacOS and Linux

    ```
    cd src/lynx
    source tools/envsetup.sh
    tools/hab sync .
    ```

- Windows

    ```
    cd src\lynx
    tools\envsetup.ps1
    tools\hab.ps1 sync .
    ```

### Install the Android Components

Execute the following commands, which will install the Android components required by Lynx, including the Android SDK/NDK. During the execution process, your authorization might be required.

```
python3 tools/android_tools/prepare_android_build.py
```

## Build and Run

You can compile LynxExplorer through the command line terminal or Android Studio. The following two methods are introduced respectively.

### Method 1: Build and Run using Android Studio

#### Open the Project

1. Use Android Studio to open the `/explorer/android` directory of the project.

2. Make sure that the JDK used by your Android Studio points to the JDK 11 installed in the above steps: 

    1. Open Settings > Build,Execution,Deployment > Build Tools > Gradle, modify the Default Gradle JDK.
    2. Fill in the path of your JAVA_HOME. If you follow the JDK configuration steps above, it is likely to be
        - `/Library/Java/JavaVirtualMachines/zulu-11.jdk/Contents/Home` on MacOS.
        - `/usr/lib/jvm/java-11-openjdk-amd64` on Linux.
        - `C:\Program Files\ojdkbuild\java-11-openjdk-11.0.15-1` on Windows.

3. Trigger Gradle sync.

#### Build and Run

Select the `LynxExplorer` module and click the `Run` button to experience LynxExplorer on your device.

### Method 2: Build and Run using the Command Line

#### Build

Enter the `explorer/android` directory from the project root directory and execute the following command.

```
cd explorer/android
./gradlew :LynxExplorer:assembleNoAsanDebug --no-daemon
```

This command will generate LynxExplorer-noasan-debug.apk in the `lynx_explorer/build/outputs/apk/noasan/debug/` folder.

#### Install

You can install the above .apk file on your device using the adb command.

```
adb install lynx_explorer/build/outputs/apk/noasan/debug/LynxExplorer-noasan-debug.apk
```

If the adb command is not found, you can add the path to the adb command in the environment configuration file(~/.zshrc or ~/.bash_profile or ~/.bashrc):

- MacOS and Linux

    ```
    export PATH=${PATH}:${ANDROID_HOME}/platform-tools
    ```

- Windows

    After executing the following command, restart PowerShell to use the adb command.

    ```
    [Environment]::SetEnvironmentVariable('PATH', "$ANDROID_HOME/platform-tools;$PATH", 'User')
    ```
