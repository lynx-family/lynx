# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import subprocess

# Get the current script's directory
current_path = os.path.dirname(os.path.realpath(__file__))

# Define arrays for Android names and library names
android_names = ["lynx_android/src/main/", "lynx_devtool/src/main/", "../../devtool/base_devtool/android/base_devtool/src/main/"]
library_names = ["core", "devtool/lynx_devtool", "devtool/base_devtool"]

# Check if the lengths of the two arrays are the same
if len(android_names) != len(library_names):
    print("Error: ANDROID_NAME and LIBRARY_NAME arrays are not of the same length!")
    exit(1)

# Iterate through the arrays
for index in range(len(library_names)):
    root_lynx_java_path = os.path.join(current_path, "../../platform/android/", android_names[index], "java/")
    lynx_output_dir = os.path.join(current_path, "../../", library_names[index], "/build/gen/")
    lynx_gen_file = os.path.join(current_path, "jni_generator.py")
    stamp_file = os.path.join(lynx_output_dir, "stamp")
    jni_files_list = os.path.join(current_path, "../../", library_names[index], "/build/jni_files")
    print(f'jni_files_list: {jni_files_list}')

    # Initialize variables for timestamps
    new_stamp_content = ""
    outputs_timestamp = ""

    # Get the timestamp of the current script
    current_script_timestamp = os.path.getmtime(__file__)
    new_stamp_content += f"{__file__}:{current_script_timestamp}"

    # Get the timestamp of jni_generator.py
    jni_generator_timestamp = os.path.getmtime(lynx_gen_file)
    new_stamp_content += f"{lynx_gen_file}:{jni_generator_timestamp}"

    # Get the timestamp of jni_files
    jni_files_timestamp = os.path.getmtime(jni_files_list)
    new_stamp_content += f"{jni_files_list}:{jni_files_timestamp}"

    # Read the jni_files list and get timestamps of input and output files
    with open(jni_files_list, 'r') as f:
        for line in f:
            line = line.strip()
            input_file = os.path.join(root_lynx_java_path, line)
            current_timestamp = os.path.getmtime(input_file)
            new_stamp_content += f"{input_file}:{current_timestamp}"

            file_name = os.path.basename(line)
            jni_file_name = os.path.splitext(file_name)[0] + "_jni.h"
            output_file = os.path.join(lynx_output_dir, jni_file_name)
            if os.path.exists(output_file):
                output_timestamp = os.path.getmtime(output_file)
                outputs_timestamp += f"{output_file}:{output_timestamp}"

    # Read the old stamp content
    old_stamp_content = ""
    if os.path.exists(stamp_file):
        with open(stamp_file, 'r') as f:
            old_stamp_content = f.read()

    # Check if the timestamps have changed
    if new_stamp_content + outputs_timestamp != old_stamp_content:
        with open(jni_files_list, 'r') as f:
            for line in f:
                line = line.strip()
                file_name = os.path.basename(line)
                jni_file_name = os.path.splitext(file_name)[0] + "_jni.h"
                input_file = os.path.join(root_lynx_java_path, line)
                output_file = os.path.join(lynx_output_dir, jni_file_name)
                # Call the jni_generator.py script
                subprocess.run(["python3", lynx_gen_file, input_file, output_file])
                print(f"python3 {lynx_gen_file} {input_file} {output_file}")
                output_timestamp = os.path.getmtime(output_file)
                new_stamp_content += f"{output_file}:{output_timestamp}"

        # Write the new stamp content to the stamp file
        with open(stamp_file, 'w') as f:
            f.write(new_stamp_content)