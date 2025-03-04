# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import datetime
import sys


def get_format_time_str():
    current_time = datetime.datetime.now()
    formatted_time = current_time.strftime("%Y/%m/%d %H:%M:%S")
    return formatted_time


class BaseLog:
    def info(self, msg):
        pass

    def success(self, msg):
        pass

    def error(self, msg):
        pass

    def fatal(self, msg):
        pass

    def warning(self, msg):
        pass


class Log:
    green_color_code = "\033[92m"
    red_color_code = "\033[91m"
    yellow_color_code = "\033[93m"
    blue_color_code = "\033[94m"
    reset_color_code = "\033[0m"

    @staticmethod
    def print(msg):
        print(msg, flush=True)

    @staticmethod
    def info(msg):
        print_msg = f"[RTF][INFO] {msg}"
        print(
            f"{Log.blue_color_code}[{get_format_time_str()}]{print_msg}{Log.reset_color_code}",
            flush=True,
        )

    @staticmethod
    def success(msg):
        print_msg = f"[RTF][SUCCESS] {msg}"
        print(
            f"{Log.green_color_code}[{get_format_time_str()}]{print_msg}{Log.reset_color_code}",
            flush=True,
        )

    @staticmethod
    def error(msg):
        print_msg = f"[RTF][ERROR] {msg}"
        print(
            f"{Log.red_color_code}[{get_format_time_str()}]{print_msg}{Log.reset_color_code}",
            flush=True,
        )

    @staticmethod
    def fatal(msg):
        print_msg = f"[RTF][FATAL] {msg}"
        print(
            f"{Log.red_color_code}[{get_format_time_str()}]{print_msg}{Log.reset_color_code}",
            flush=True,
        )
        sys.exit(1)

    @staticmethod
    def warning(msg):
        print_msg = f"[RTF][WARNING] {msg}"
        print(
            f"{Log.yellow_color_code}[{get_format_time_str()}]{print_msg}{Log.reset_color_code}",
            flush=True,
        )


class LocalFileLog(BaseLog):
    def __init__(self):
        self.contents = []

    def info(self, msg):
        self.contents.append(msg)
        Log.info(msg)

    def success(self, msg):
        self.contents.append(msg)
        Log.success(msg)

    def error(self, msg):
        self.contents.append(msg)
        Log.error(msg)

    def fatal(self, msg):
        self.contents.append(msg)
        self.save()
        Log.fatal(msg)

    def warning(self, msg):
        self.contents.append(msg)
        Log.warning(msg)

    def save(self):
        with open("rtf_result.log", "w") as f:
            f.write("\n".join(self.contents))


class RobotMessageLog(BaseLog):
    def __init__(self):
        super().__init__()
        self.template = """{
          "config": {
            "wide_screen_mode": true
          },
          "elements": [
            {
              "tag": "hr"
            },
            {
                "content": "Messages:\\n%s",
                "tag": "markdown"
            }
          ],
          "header": {
            "template": "%s",
            "title": {
              "content": "Coverage Check",
              "tag": "plain_text"
            }
          }
        }
        """
        self.result = "green"  # success(green), warning(yellow), failed(red)
        self.contents = []

    def warning(self, msg):
        if self.result == "green":
            self.result = "yellow"
        self.contents.append(f"🟠{msg}")
        Log.warning(msg)

    def error(self, msg):
        self.result = "red"
        self.contents.append(f"❌{msg}")
        Log.error(msg)

    def fatal(self, msg):
        self.result = "red"
        self.contents.append(f"❌{msg}")
        self.save()
        Log.fatal(msg)

    def success(self, msg):
        self.contents.append(f"📢{msg}")
        Log.success(msg)

    def info(self, msg):
        self.contents.append(f"📢{msg}")
        Log.info(msg)

    def save(self):
        message = self.template % ("\\n\\n".join(self.contents), self.result)
        with open("check_unittest_coverage.msg", "w") as f:
            f.write(message)
