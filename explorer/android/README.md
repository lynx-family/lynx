# Build Lynx Explorer

This document will help you build LynxExplorer for Android on MacOS or Linux.

## System requirements

- 100GB or more of disk space
- Git/Python3(>=3.9) installed

## Install dependencies

The following dependencies are needed:

- JDK 11
- Android development environment
- Python library

### JDK

#### Install JDK11

#### MacOS
We recommend using Homebrew to install the OpenJDK distribution called Zulu, which is provided by Azul.

```
brew install --cask zulu@11
```

You can use the following command to confirm whether the installation is successful

```
javac --version
```

If the installation is successful, the terminal will output javac version number 11

#### Linux

On Ubuntu or Debian:
```
sudo apt install openjdk-11-jdk 
```

On RHEL or CentOS:
```
sudo yum install openjdk-11-jdk 
```


#### Update JAVA_HOME

Confirm your JDK installation directory. If you follow the above steps, the JDK path is likely to be 
- `/Library/Java/JavaVirtualMachines/zulu-11.jdk/Contents/Home` on MacOS
- `/usr/lib/jvm/java-11-openjdk-amd64` on Linux.  

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

### Android development environment

Configuring the Android development environment required by Lynx includes the following step:

#### Configure ANDROID_HOME

Add the ANDROID_HOME variable to your environment configuration file(maybe ~/.zshrc or ~/.bash_profile or ~/.bashrc,depending on your terminal environment).

If you have installed the Android SDK before, please set ANDROID_HOME to the installation directory of the Android SDK.(If you have previously installed Android SDK by Android Studio, the installation path of the Android SDK is usually located at /Users/your-username/Library/Android/sdk)

If you have NOT installed the Android SDK before, you can set ANDROID_HOME to the path where you want the Android SDK to be installed. We have tools to help you install the Android SDK to ANDROID_HOME.

```
export ANDROID_HOME=<path-to-android-sdk>
```


### Python library

#### MacOS
The yaml dependency needs to be installed to execute some auto-generation logic.

```
# use the virtual environment to manage python environment
python3 -m venv venv
source venv/bin/activate
# install PyYAML package
pip3 install pyyaml
```
#### Linux

We recommend using pyenv to manage python environment on Linux.

To install pyenv: [`https://github.com/pyenv/pyenv`](https://github.com/pyenv/pyenv)   
Install python with version larger or equal to 3.9 using pyenv:  
```
pyenv install 3.9 # Or a larger version
pyenv global 3.9 # Or a larger version
```
Install pyyaml after python is setup:
```
pip3 install pyyaml
```

## Get the code

### Pull the repository

Pull the code from the Github repository and specify the path(`src/lynx`) to avoid contaminating the directory when installing dependencies.

```
git clone git@github.com:lynx-family/lynx.git src/lynx
```

### Get the dependent files

After getting the project repository, execute the following commands in the root directory of the project to get the project dependent files

```
cd src/lynx
source tools/envsetup.sh
tools/hab sync .
```

### Install the Android components

Execute the following commands, which will install the Android components required by Lynx, including the Android SDK/NDK. During the execution process, your authorization might be required.

```
source tools/android_tools/prepare_android_build.sh
```

## Compile and run

You can compile LynxExplorer through the command line terminal or Android Studio. The following two methods are introduced respectively.

### Method 1: Compile and run using Android Studio

#### Open the project

1.Use Android Studio to open the `/explorer/android` directory of the project

2.Make sure that the JDK used by your Android Studio points to the JDK 11 installed in the above steps: 

1. Open Settings > Build,Execution,Deployment > Build Tools > Gradle, modify the Default Gradle JDK
2. Fill in the path of your JAVA_HOME. If you follow the JDK configuration steps above, it is likely to be
    - `/Library/Java/JavaVirtualMachines/zulu-11.jdk/Contents/Home` on MacOS.
    - `/usr/lib/jvm/java-11-openjdk-amd64` on Linux.
    
3.Trigger Gradle sync    

#### Compile and run

Select the `LynxExplorer` module and click the `Run` button to experience LynxExplorer on your device

### Method 2: Compile and run using the command line

#### Compile

Enter the `explorer/android` directory from the project root directory and execute the following command

```
cd explorer/android
./gradlew :LynxExplorer:assembleNoAsanDebug --no-daemon
```

This command will generate LynxExplorer-noasan-debug.apk in the `lynx_explorer/build/outputs/apk/noasan/debug/` folder

#### Install

You can install the above .apk file on your device using the adb command

````
adb install lynx_explorer/build/outputs/apk/noasan/debug/LynxExplorer-noasan-debug.apk
````

If the adb command is not found, you can add the path to the adb command in the environment configuration file(~/.zshrc or ~/.bash_profile or ~/.bashrc):

```
export PATH=${PATH}:${ANDROID_HOME}/platform-tools
```