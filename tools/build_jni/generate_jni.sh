# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

CURRENT_PATH=$(cd `dirname $0`; pwd)
ANDROID_NAME=("lynx_android/src/main/")
LIBRARY_NAME=("core")
ANDROID_NAME_LENGTH=${#LIBRARY_NAME[@]}
LIBRARY_NAME_LENGTH=${#ANDROID_NAME[@]}
if [ ${ANDROID_NAME_LENGTH} -ne ${LIBRARY_NAME_LENGTH} ]; then
    echo "Error: ANDROID_NAME and LIBRARY_NAME arrays are not of the same length!"
    exit 1
fi
python3 -c "import yaml"
if [ $? -eq 0 ]; then
    echo "PyYAML installed"
else
    echo "PyYAML is not installed"
    pip3 install PyYAML
    python3 -c "import yaml"
    if [ $? -eq 0 ]; then
        echo "PyYAML installed successfully"
    else
        echo "PyYAML installation failed"
    fi
fi
for index in $(seq 0 $(($ANDROID_NAME_LENGTH - 1)))
do
    GEN_FILE=$CURRENT_PATH"/yaml_to_jni_configs.py"
    LIBRARY_ROOT_PATH=$CURRENT_PATH"/../.."
    JAVA_ROOT_PATH=$CURRENT_PATH"/../../platform/android/"${ANDROID_NAME[index]}"java/"
    OUTPUT_DIR=$CURRENT_PATH"/../../"${LIBRARY_NAME[index]}"/build/jni/gen"
    INCLUDE_ROOT_DIR=${LIBRARY_NAME[index]}"/build/jni/gen"
    python3 $GEN_FILE $LIBRARY_ROOT_PATH $JAVA_ROOT_PATH $OUTPUT_DIR $INCLUDE_ROOT_DIR
done