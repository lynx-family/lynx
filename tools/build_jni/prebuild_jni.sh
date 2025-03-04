# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
CURRENT_PATH=$(cd `dirname $0`; pwd)
ANDROID_NAME=("lynx_android/src/main/" "lynx_devtool/src/main/" "../../devtool/base_devtool/android/base_devtool/src/main/")
LIBRARY_NAME=("core" "devtool/lynx_devtool" "devtool/base_devtool")
ANDROID_NAME_LENGTH=${#LIBRARY_NAME[@]}
LIBRARY_NAME_LENGTH=${#ANDROID_NAME[@]}
if [ ${ANDROID_NAME_LENGTH} -ne ${LIBRARY_NAME_LENGTH} ]; then
    echo "Error: ANDROID_NAME and LIBRARY_NAME arrays are not of the same length!"
    exit 1
fi
for index in $(seq 0 $(($ANDROID_NAME_LENGTH - 1)))
do
    ROOT_LYNX_JAVA_PATH=$CURRENT_PATH"/../../platform/android/"${ANDROID_NAME[index]}"java/"
    LYNX_OUTPUT_DIR=$CURRENT_PATH"/../../"${LIBRARY_NAME[index]}"/build/gen/"
    LYNX_GEN_FILE=$CURRENT_PATH"/jni_generator.py"
    STAMP_FILE=$LYNX_OUTPUT_DIR"stamp"
    JNI_FILES_LIST=$CURRENT_PATH"/../../"${LIBRARY_NAME[index]}"/build/jni_files"

    # timestamps of files: current script / jni_generator.py / jni_files / jni headers / java files
    new_stamp_content=""
    outputs_timestamp=""

    current_script_timestamp=$(stat -f "%m" "$0")
    new_stamp_content+="$0:$current_script_timestamp"$

    jni_generator_timestamp=$(stat -f "%m" "$LYNX_GEN_FILE")
    new_stamp_content+="$LYNX_GEN_FILE:$jni_generator_timestamp"$

    jni_files_timestamp=$(stat -f "%m" "$JNI_FILES_LIST")
    new_stamp_content+="$JNI_FILES_LIST:$jni_files_timestamp"$

    while read line || [ -n "$line" ]
    do
        input_file=$ROOT_LYNX_JAVA_PATH$line
        current_timestamp=$(stat -f "%m" "$input_file")
        new_stamp_content+="$input_file:$current_timestamp"$

        file_name=${line##*/}
        jni_file_name=${file_name%.*}"_jni.h"
        output_file=$LYNX_OUTPUT_DIR$jni_file_name
        if [ -f "$output_file" ]; then
            output_timestamp=$(stat -f "%m" "$output_file")
            outputs_timestamp+="$output_file:$output_timestamp"$
        fi
    done < "$JNI_FILES_LIST"

    if [ "$new_stamp_content$outputs_timestamp" != "$(cat "$STAMP_FILE" 2>/dev/null)" ]; then
        while read line || [ -n "$line" ]
        do
            file_name=${line##*/}
            jni_file_name=${file_name%.*}"_jni.h"
            input_file=$ROOT_LYNX_JAVA_PATH$line
            output_file=$LYNX_OUTPUT_DIR$jni_file_name
            python3 $LYNX_GEN_FILE $input_file $output_file
            echo "python3 $LYNX_GEN_FILE $input_file $output_file"
            output_timestamp=$(stat -f "%m" "$output_file")
            new_stamp_content+="$output_file:$output_timestamp"$
        done < "$JNI_FILES_LIST"

        echo "$new_stamp_content" > "$STAMP_FILE"
    fi
done