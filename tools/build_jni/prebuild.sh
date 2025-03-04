#!/bin/bash
CURRENT_PATH=$(cd `dirname $0`; pwd)
JNI_PREBUILD=$CURRENT_PATH"/prebuild_jni.sh"
$JNI_PREBUILD
$CURRENT_PATH"/generate_jni.sh"