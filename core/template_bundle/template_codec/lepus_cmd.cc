// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef __EMSCRIPTEN__
#include "core/template_bundle/template_codec/lepus_cmd.h"

#include <dirent.h>

#include <fstream>
#include <memory>
#include <system_error>
#include <vector>

#include "core/template_bundle/lynx_template_bundle_converter.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "core/template_bundle/template_codec/lepus_cmd_ipc.h"

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

bool ReadTextFile(const std::string& file_path, std::string& out) {
  std::ifstream input(file_path, std::ios::in);
  if (!input.is_open()) {
    return false;
  }
  out.assign(std::istreambuf_iterator<char>(input),
             std::istreambuf_iterator<char>());
  return true;
}

bool ReadBinaryFile(const std::string& file_path, std::vector<uint8_t>& out) {
  std::ifstream input(file_path, std::ios::in | std::ios::binary);
  if (!input.is_open()) {
    return false;
  }
  input.seekg(0, std::ios::end);
  const auto size = input.tellg();
  if (size < 0) {
    return false;
  }
  out.resize(static_cast<size_t>(size));
  input.seekg(0, std::ios::beg);
  if (!out.empty()) {
    input.read(reinterpret_cast<char*>(out.data()),
               static_cast<std::streamsize>(out.size()));
  }
  return input.good() || input.eof();
}

bool WriteTextFile(const std::string& file_path, const std::string& content) {
  std::ofstream output(file_path, std::ios::out | std::ios::trunc);
  if (!output.is_open()) {
    return false;
  }
  output << content;
  return output.good();
}

bool WriteBinaryFile(const std::string& file_path,
                     const std::vector<uint8_t>& content) {
  std::ofstream output(file_path,
                       std::ios::out | std::ios::binary | std::ios::trunc);
  if (!output.is_open()) {
    return false;
  }
  if (!content.empty()) {
    output.write(reinterpret_cast<const char*>(content.data()),
                 static_cast<std::streamsize>(content.size()));
  }
  return output.good();
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
  auto has_option = [argc, argv](const std::string& option) {
    auto first = argv;
    auto last = argv + argc;
    return std::find(first, last, option) != last;
  };
  auto parse_option = [argc, argv](const std::string& option) {
    auto first = argv;
    auto last = argv + argc;
    auto it = std::find(first, last, option);
    if (it != last && ++it != last) {
      return std::string{*it};
    }
    return std::string{};
  };

  const auto mode = parse_option("--mode");
  if (mode == "ipc") {
    // Run in IPC mode using stdin/stdout with CBOR protocol
    lynx::lepus_cmd::RunIpcMode();
    return 0;
  }
  if (mode == "decode") {
    const auto input_path = parse_option("--input");
    const auto output_path = parse_option("--output");
    if (input_path.empty() || output_path.empty()) {
      std::cerr << "decode mode requires --input <binary> and --output <json>"
                << std::endl;
      return 1;
    }

    std::vector<uint8_t> binary;
    if (!ReadBinaryFile(input_path, binary)) {
      std::cerr << "cannot read input file: " << input_path << std::endl;
      return 2;
    }

    auto reader = lynx::tasm::LynxBinaryReader::CreateLynxBinaryReader(binary);
    if (!reader.Decode()) {
      std::cerr << "decode failed: " << reader.error_message_ << std::endl;
      return 3;
    }
    auto bundle = reader.GetTemplateBundle();
    const auto result = lynx::tasm::LynxTemplateBundleConverter::
        ConvertTemplateBundleToSerializedString(bundle);
    if (!WriteTextFile(output_path, result)) {
      std::cerr << "cannot write output file: " << output_path << std::endl;
      return 4;
    }
    std::cout << "decode success: " << output_path << std::endl;
    return 0;
  }

  std::string encode_options;
  if (!parse_option("--input").empty()) {
    const auto input_path = parse_option("--input");
    if (!ReadTextFile(input_path, encode_options)) {
      std::cerr << "cannot read input file: " << input_path << std::endl;
      return 1;
    }
  } else {
    // Backward compatible mode:
    // use --path/--ttml/--config/--snapshot/... to generate encode options.
    encode_options = lynx::lepus_cmd::MakeEncodeOptionsFromArgs(argc, argv);
  }

  auto encode_result = lynx::tasm::encode(encode_options);
  if (encode_result.status != 0) {
    std::cerr << "encode failed, status=" << encode_result.status
              << ", error=" << encode_result.error_msg << std::endl;
    return 2;
  }

  const auto output_path = parse_option("--output");
  if (!output_path.empty()) {
    if (!WriteBinaryFile(output_path, encode_result.buffer)) {
      std::cerr << "cannot write output file: " << output_path << std::endl;
      return 3;
    }
    std::cout << "encode success: " << output_path
              << ", bytes=" << encode_result.buffer.size() << std::endl;
  } else if (has_option("--mode")) {
    std::cerr << "encode mode requires --output <binary>" << std::endl;
    return 1;
  } else {
    std::cout << "encode success, bytes=" << encode_result.buffer.size()
              << " (missing --output, nothing written)" << std::endl;
  }
  return 0;
}

#endif  // __EMSCRIPTEN__
