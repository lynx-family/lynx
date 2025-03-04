// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_LEPUS_CMD_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_LEPUS_CMD_H_
#ifndef __EMSCRIPTEN__

#include <fts.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "core/runtime/vm/lepus/json_parser.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace lepus_cmd {
struct PackageConfigs {
  bool snapshot_;
  bool silence_;
  std::string target_sdk_version_;
};
std::string MakeEncodeOptions(const std::string& abs_folder_path,
                              const std::string& ttml_file_path,
                              const PackageConfigs& package_configs);
std::string MakeEncodeOptionsFromArgs(int argc, char** argv);
}  // namespace lepus_cmd
}  // namespace lynx

#endif  //__EMSCRIPTEN__

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_LEPUS_CMD_H_
