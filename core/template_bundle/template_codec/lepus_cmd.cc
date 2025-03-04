// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef __EMSCRIPTEN__
#include "core/template_bundle/template_codec/lepus_cmd.h"

#include <dirent.h>

#include <memory>
#include <system_error>

#include "core/template_bundle/template_codec/binary_encoder/encoder.h"

namespace {
auto CreateJsonDocumentForCompile(
    const std::string& ttml_buf, const std::string& js_buf,
    const lynx::lepus_cmd::PackageConfigs& package_configs) {
  auto options = rapidjson::Document{};
  options.SetObject();

  auto& options_allocator = options.GetAllocator();
  // deprecated old cli configs
  options.AddMember("sourceFile", std::string{}, options_allocator);
  // from .intermediates
  options.AddMember("manifest", rapidjson::Value(js_buf.data(), js_buf.size()),
                    options_allocator);
  options.AddMember("sourceContent",
                    rapidjson::Value(ttml_buf.data(), ttml_buf.size()),
                    options_allocator);
  // from package.config.json
  options.AddMember("silence", package_configs.silence_, options_allocator);
  options.AddMember("snapshot", package_configs.snapshot_, options_allocator);
  options.AddMember("targetSdkVersion", package_configs.target_sdk_version_,
                    options_allocator);
  // others
  options.AddMember("outputFile", "", options_allocator);
  return options;
}

auto ReadPackageConfigsFromFile(const std::string& package_config_buf,
                                const char* package_config_file_path) {
  auto options = rapidjson::Document{};
  if (options.Parse(package_config_buf.data()).HasParseError()) {
    throw std::system_error(std::make_error_code(std::errc::bad_message), [&] {
      std::string what{"invalid json file: "};
      what += package_config_file_path;
      what += ", parse error: ";
      what += options.GetParseErrorMsg();
      what += ", position: ";
      what += std::to_string(options.GetErrorOffset());
      return what;
    }());
  }

  auto package_config = lynx::lepus_cmd::PackageConfigs{
      .snapshot_ =
          options.HasMember("snapshot") && options["snapshot"].GetBool(),
      .silence_ = options.HasMember("silence") && options["silence"].GetBool(),
      .target_sdk_version_ = options.HasMember("targetSdkVersion")
                                 ? options["targetSdkVersion"].GetString()
                                 : ""};

  return package_config;
}

std::string RapidJsonDocumentToString(const rapidjson::Document& document) {
  auto buf = rapidjson::StringBuffer{};
  auto writer = rapidjson::Writer<decltype(buf)>{buf};
  document.Accept(writer);
  return buf.GetString();
}

bool StrEndWith(const char* str, const char* suffix) {
  if (str == nullptr || suffix == nullptr) return false;

  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  if (suffix_len > str_len) return false;

  return 0 == strncmp(str + str_len - suffix_len, suffix, suffix_len);
}

template <typename Func>
auto ForEachInDir(const char* dir_path, Func&& func) {
  const char* paths[2] = {dir_path, nullptr};
  auto fts = ::fts_open(const_cast<char**>(paths), 0, nullptr);
  ::fts_read(fts);
  auto child = ::fts_children(fts, 0);
  while (child != nullptr) {
    if (!S_ISDIR(child->fts_statp->st_mode)) {
      func(child->fts_name);
    }
    child = child->fts_link;
  }
  ::fts_close(fts);
}

std::string GetAbsPath(const std::string& path) {
  char abs_path[PATH_MAX];
  if (!::realpath(path.c_str(), abs_path)) {
    throw std::system_error{
        std::make_error_code(std::errc::no_such_file_or_directory),
        std::string{abs_path}};
  }
  return std::string{abs_path};
}

auto GenerateJSMap(const char* folder_path) {
  auto js_map = rapidjson::Document{};
  js_map.SetObject();
  auto& js_map_allocator = js_map.GetAllocator();

  ForEachInDir(folder_path, [&](const char* file_name) {
    if (StrEndWith(file_name, ".js")) {
      auto value = rapidjson::Value{};
      auto abs_path = std::string{folder_path};
      abs_path += file_name;
      // check the path of the js file.
      abs_path = GetAbsPath(abs_path);
      value.SetString(abs_path.c_str(), js_map_allocator);
      js_map.AddMember(rapidjson::Value{file_name, js_map_allocator}.Move(),
                       value.Move(), js_map_allocator);
    }
  });

  return js_map;
}

auto GetAbsFolderPathFromPath(const std::string& option_path) {
  auto res = GetAbsPath(option_path);
  auto on_folder_close = [](::DIR* dir_ptr) { ::closedir(dir_ptr); };
  auto abs_folder_dir = std::unique_ptr<::DIR, decltype(on_folder_close)>(
      ::opendir(res.c_str()), on_folder_close);
  // make sure the dir exists
  if (!abs_folder_dir) {
    throw std::system_error{
        std::make_error_code(std::errc::no_such_file_or_directory),
        res + "is not a FOLDER."};
  }
  res.push_back('/');
  return res;
}

}  // anonymous namespace

namespace lynx {
namespace lepus_cmd {

std::string MakeEncodeOptions(const std::string& folder_path,
                              const std::string& ttml_path,
                              const PackageConfigs& package_configs) {
  const auto abs_folder_path = GetAbsFolderPathFromPath(folder_path);
  auto abs_ttml_path = abs_folder_path + "/ttml.json";
  if (!ttml_path.empty()) {
    try {
      abs_ttml_path = GetAbsPath(ttml_path);
    } catch (...) {
      std::cerr << "cannot find file: " << ttml_path
                << "use default file: " << abs_ttml_path << "instead"
                << std::endl;
    }
  }
  // read the ttml.json into memory
  const auto ttml_buf = lepus::readFile(abs_ttml_path.c_str());

  // collect all '.js' file in abs_folder_path to build a map: [JS_FILE_NAME :
  // JS_FILE_ABS_PATH]
  const auto js_map = GenerateJSMap(abs_folder_path.c_str());
  const auto js_map_str = RapidJsonDocumentToString(js_map);

  // create the json doc and convert it into a string and feed it into function
  // encode
  const auto document =
      CreateJsonDocumentForCompile(ttml_buf, js_map_str, package_configs);
  const auto document_str = RapidJsonDocumentToString(document);
  return document_str;
}
/*
 * usage:
 * lepus_cmd
 * --path   [path to dir containing all '.js' file: app-service.js,
 * entry-xxxxx.js, ... ]
 * --config [path to project.config.json]. if applied,
 * --targetSdkVersion, --silence do not work
 * --ttml   [path to ttml.json] if not applied, use --path + "ttml.json"
 * instead.
 *
 * --snapshot if applied, enable snapshot
 * --targetSdkVersion [sdk version]
 * --silence if applied no debug message outputs
 *
 */
std::string MakeEncodeOptionsFromArgs(int args, char** argv) {
  auto has_option = [args, argv](const std::string& option) {
    auto first = argv;
    auto last = argv + args;
    auto it = std::find(first, last, option);
    if (it != last) {
      return true;
    }
    return false;
  };

  auto parse_option = [args, argv](const std::string& option) {
    auto first = argv;
    auto last = argv + args;
    auto it = std::find(first, last, option);
    if (it != last && ++it != last) {
      return std::string{*it};
    }
    return std::string{};
  };

  const auto package_configs = [&]() {
    // if we assign a path for project.config.json, use the config in the json.
    if (has_option("--config")) {
      const auto package_config_file_path =
          GetAbsPath(parse_option("--config"));
      const auto package_config_buf =
          lepus::readFile(package_config_file_path.c_str());
      return ReadPackageConfigsFromFile(package_config_buf,
                                        package_config_file_path.c_str());
    }
    // otherwise, use default or args.
    return PackageConfigs{
        .snapshot_ = has_option("--snapshot"),
        .silence_ = has_option("--silence"),
        .target_sdk_version_ = has_option("--targetSdkVersion")
                                   ? parse_option("--targetSdkVersion")
                                   : std::string{}};
  }();

  auto option_path = parse_option("--path");
  // get the path for the dir containing all '.js' files : app-service.js,
  // entry-xxxx.js
  const auto abs_folder_path = GetAbsFolderPathFromPath(option_path);

  // get the path for the 'ttml.json'
  const auto ttml_file_path = [&]() {
    if (has_option("--ttml")) {
      return parse_option("--ttml");
    } else {
      return GetAbsPath(abs_folder_path + "/ttml.json");
    }
  }();

  return MakeEncodeOptions(abs_folder_path, ttml_file_path, package_configs);
}
}  // namespace lepus_cmd
}  // namespace lynx

int main(int argc, char** argv) {
  // put speedy's input here.
  const auto compile_options = "";
  lynx::tasm::EncodeResult res = lynx::tasm::encode(compile_options);
  LOGI("Encode status: " << res.status
                         << ". And error message is: " << res.error_msg);
}

#endif  // __EMSCRIPTEN__
