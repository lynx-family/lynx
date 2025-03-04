# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import hashlib
import os

from core.env.context import RTFContext


class Options:
    class OptionsListener:
        def __init__(self):
            pass

        # The reason for not using random strings as labels is that each rerun
        # generates a different "out" directory, requiring a full recompilation.
        # def generate_random_string(length):
        #     letters = string.ascii_letters
        #     return "".join(random.choice(letters) for _ in range(length))

        @staticmethod
        def generate_label(raw_data):
            hash_object = hashlib.md5(raw_data.encode())
            hash_value = hash_object.hexdigest()
            return hash_value

        def did_include(self, options, builders, targets, coverage):
            pass

    def __init__(self, context, args=None):
        self.context = context
        self.listeners: [Options.OptionsListener] = []
        if args is not None:
            k_v_list = [k_v.strip() for k_v in args.split(",")]
            for k_v in k_v_list:
                key_value = k_v.split("=")
                if len(key_value) != 2:
                    continue
                key = key_value[0]
                value = key_value[1]
                self.context.insert_variable(key, value)
        self.workspace = self.context.find_variable("workspace")

    def get_bool(self, key: str, default: bool):
        value = self.context.find_variable(key)
        if value is not None:
            if value.lower() == "false":
                return False
            return bool(value)
        else:
            return default

    def register_listener(self, listener: OptionsListener):
        self.listeners.append(listener)

    def get_str(self, key: str, default: str):
        value = self.context.find_variable(key)
        if value is not None:
            return str(value)
        else:
            return default

    def get_dir_list(self, path: str, excludes: [str]):
        search_path = os.path.join(self.workspace, path)
        contents = os.listdir(search_path)
        result = []
        for content in contents:
            if content not in excludes:
                result_path = os.path.join(path, content)
                if os.path.isdir(result_path):
                    result.append(f"{result_path}/*")
                else:
                    result.append(result_path)
        return result

    def include(self, template_path):
        template = os.path.join(self.workspace, template_path)

        # Identify the root directory of the repository where the template file is located,
        # because the options object within the included template needs to operate within
        # the corresponding repository directory.
        # eg:
        #   if: template_path = oss/.rtf/native-ut-lynx.template
        #   then: workspace = oss (not include .rtf)
        def find_workspace(path):
            target_workspace = os.path.dirname(path)
            folder_name = os.path.basename(target_workspace)
            if folder_name == ".rtf":
                return find_workspace(target_workspace)
            else:
                return target_workspace

        workspace = find_workspace(template)
        with open(template, "r") as template_file:
            context = RTFContext(workspace=workspace)
            context.options = Options(context)
            context.parent = self.context
            context.parent.children.append(context)
            context.options.listeners = self.context.options.listeners
            exec(template_file.read(), context.export())

            for listener in self.listeners:
                listener.did_include(
                    context.options,
                    context.find_variable("builder"),
                    context.find_variable("targets"),
                    context.find_variable("coverage"),
                )
