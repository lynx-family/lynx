#!/bin/bash
set -e

version="$2"
enableLite="$4"
jssdkMainDestPath="$6"
jssdkDebugDestPath="$8"
lynxCoreBuildToolsPath="${10}"

build() {
    cd "$lynxCoreBuildToolsPath"
    "./build.sh" android "$jssdkMainDestPath/lynx_core.js" "$jssdkDebugDestPath/lynx_core_dev.js" "$version"
}

clear() {
    if [ -f "${jssdkMainDestPath}/lynx_core.js" ]; then
        rm "${jssdkMainDestPath}/lynx_core.js"
    fi
    if [ -f "${jssdkDebugDestPath}/lynx_core_dev.js" ]; then
        rm "${jssdkDebugDestPath}/lynx_core_dev.js"
    fi
}

if [[ "$1" = "--build" ]]; 
then
    echo "build"
    build
elif [[ "$1" = "--clear" ]]; 
then
    echo "clear"
    clear
else
    echo "no action"
fi
