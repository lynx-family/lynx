// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/fixture_replay_harmony.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/base/json/json_util.h"
#include "core/services/replay/fixture_evaluator.h"
#include "third_party/modp_b64/modp_b64.h"
#include "third_party/rapidjson/document.h"
#include "third_party/zlib/contrib/minizip/unzip.h"

namespace lynx {
namespace devtool {
namespace {

constexpr size_t kMaxArchiveBytes = 128 * 1024 * 1024;
constexpr size_t kMaxEntries = 65535;

bool IsSafePath(const std::string& path) {
  if (path.empty() || path.find_first_of("\\:") != std::string::npos ||
      path.find('\0') != std::string::npos) {
    return false;
  }
  size_t start = 0;
  while (start <= path.size()) {
    auto end = path.find('/', start);
    auto part = path.substr(start, end - start);
    if (part.empty() || part == "." || part == "..") return false;
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return true;
}

bool EnsureDirectories(const std::string& root, const std::string& relative) {
  size_t end = 0;
  while ((end = relative.find('/', end)) != std::string::npos) {
    const auto path = root + "/" + relative.substr(0, end++);
    struct stat info {};
    if (mkdir(path.c_str(), 0700) != 0 &&
        (lstat(path.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))) {
      return false;
    }
  }
  return true;
}

std::string ExtractArchive(const std::string& archive,
                           const std::string& directory) {
  struct stat info {};
  if (stat(archive.c_str(), &info) != 0 || !S_ISREG(info.st_mode) ||
      info.st_size <= 0 ||
      static_cast<uint64_t>(info.st_size) > kMaxArchiveBytes) {
    return "Invalid fixture archive size";
  }
  std::unique_ptr<void, decltype(&unzClose)> zip(unzOpen64(archive.c_str()),
                                                 unzClose);
  if (!zip) return "Cannot open fixture archive";
  unz_global_info64 global{};
  if (unzGetGlobalInfo64(zip.get(), &global) != UNZ_OK ||
      global.number_entry == 0 || global.number_entry > kMaxEntries) {
    return "Invalid fixture entry count";
  }
  std::unordered_set<std::string> entries;
  size_t total = 0;
  int status = unzGoToFirstFile(zip.get());
  while (status == UNZ_OK) {
    unz_file_info64 entry{};
    char filename[4096];
    if (unzGetCurrentFileInfo64(zip.get(), &entry, filename, sizeof(filename),
                                nullptr, 0, nullptr, 0) != UNZ_OK ||
        entry.size_filename == 0 || entry.size_filename >= sizeof(filename)) {
      return "Invalid fixture entry name";
    }
    std::string path(filename, entry.size_filename);
    const bool is_directory = path.back() == '/';
    if (is_directory) path.pop_back();
    const auto type = (entry.external_fa >> 16) & S_IFMT;
    if (!IsSafePath(path) || !entries.insert(path).second ||
        entries.size() > kMaxEntries || (entry.flag & 1) != 0 ||
        (type != 0 && type != (is_directory ? S_IFDIR : S_IFREG)) ||
        entry.uncompressed_size > kMaxArchiveBytes - total) {
      return "Unsafe or oversized fixture entry";
    }
    if (!EnsureDirectories(directory, is_directory ? path + "/" : path)) {
      return "Cannot create fixture directory";
    }
    if (unzOpenCurrentFile(zip.get()) != UNZ_OK) {
      return "Cannot read fixture entry";
    }
    // Write regular files only, with exclusive creation. Archive attributes can
    // never introduce symlinks or overwrite a previously extracted asset.
    std::unique_ptr<FILE, decltype(&fclose)> output(nullptr, fclose);
    if (!is_directory) {
      int fd = open((directory + "/" + path).c_str(),
                    O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
      if (fd >= 0) {
        output.reset(fdopen(fd, "wb"));
        if (!output) close(fd);
      }
      if (!output) {
        unzCloseCurrentFile(zip.get());
        return "Cannot create fixture asset";
      }
    }
    char buffer[8192];
    size_t size = 0;
    int count;
    std::string error;
    while ((count = unzReadCurrentFile(zip.get(), buffer, sizeof(buffer))) >
           0) {
      if (is_directory ||
          static_cast<size_t>(count) > kMaxArchiveBytes - total) {
        error = "Fixture exceeds extraction limit";
        break;
      }
      total += count;
      size += count;
      if (fwrite(buffer, 1, count, output.get()) !=
          static_cast<size_t>(count)) {
        error = "Cannot write fixture asset";
        break;
      }
    }
    const int close_status = unzCloseCurrentFile(zip.get());
    if (output && fclose(output.release()) != 0) {
      error = "Cannot finish fixture asset";
    }
    if (!error.empty()) return error;
    if (count < 0 || close_status != UNZ_OK ||
        size != entry.uncompressed_size) {
      return "Corrupt fixture entry";
    }
    status = unzGoToNextFile(zip.get());
  }
  if (status != UNZ_END_OF_LIST_OF_FILE ||
      entries.size() != global.number_entry ||
      entries.count("fixture.js") == 0 ||
      lstat((directory + "/fixture.js").c_str(), &info) != 0 ||
      !S_ISREG(info.st_mode)) {
    return "Invalid fixture archive or missing fixture.js";
  }
  return "";
}

// Resolve binary assets on the same worker as extraction and evaluation. The
// UI adapter only translates action field names for the existing replay path.
std::string ResolveTemplates(const std::string& directory,
                             tasm::replay::FixtureEvaluationResult& result) {
  constexpr size_t kMaxTemplateBytes = 16 * 1024 * 1024;
  size_t total = 0;
  std::unordered_map<std::string, std::string> templates;
  for (auto& action : result.actions) {
    if (action.function_name != "loadTemplate") continue;
    rapidjson::Document params;
    params.Parse(action.params_json.data(), action.params_json.size());
    if (params.HasParseError() || !params.IsObject()) {
      return "Invalid fixture template parameters";
    }
    if (params.HasMember("templateAsset")) {
      const auto& asset = params["templateAsset"];
      if (!asset.IsString()) return "Invalid fixture template asset";
      std::string path(asset.GetString(), asset.GetStringLength());
      auto found = templates.find(path);
      if (found == templates.end()) {
        auto bytes = tasm::replay::ReadFixtureAsset(
            directory, path, (kMaxTemplateBytes - total) / 4 * 3);
        if (bytes.empty()) return "Invalid or oversized fixture template asset";
        std::string encoded(lynx_modp_b64_encode_len(bytes.size()), '\0');
        encoded.resize(
            lynx_modp_b64_encode(encoded.data(), bytes.data(), bytes.size()));
        found = templates.emplace(path, std::move(encoded)).first;
      }
      params.RemoveMember("source");
      params.AddMember("source",
                       rapidjson::Value(found->second.data(),
                                        static_cast<rapidjson::SizeType>(
                                            found->second.size()),
                                        params.GetAllocator()),
                       params.GetAllocator());
      action.params_json = base::ToJson(params);
    }
    if (!params.HasMember("source") || !params["source"].IsString() ||
        params["source"].GetStringLength() == 0) {
      return "Empty fixture template";
    }
    // Count every serialized occurrence, including repeated templates.
    auto size = params["source"].GetStringLength();
    if (size > kMaxTemplateBytes - total) {
      return "Fixture templates exceed size limit";
    }
    total += size;
  }
  return "";
}

}  // namespace

base::expected<std::string, std::string> LoadFixtureForReplay(
    const std::string& archive, const std::string& directory) {
  auto error = ExtractArchive(archive, directory);
  if (!error.empty()) return base::unexpected(error);
  auto result = tasm::replay::EvaluateFixture(directory);
  if (!result.has_value()) return base::unexpected(result.error());

  error = ResolveTemplates(directory, *result);
  if (!error.empty()) return base::unexpected(error);
  // Match Android's adapter format. Payloads are already valid JSON; preserve
  // them and use the existing RapidJSON serializer only for string escaping.
  auto quote = [](const std::string& value) {
    return base::ToJson(rapidjson::Value(
        value.data(), static_cast<rapidjson::SizeType>(value.size())));
  };
  std::string json = "{\"actions\":[";
  bool first = true;
  for (const auto& action : result->actions) {
    if (!first) json += ',';
    first = false;
    json += "{\"functionName\":" + quote(action.function_name) +
            ",\"delayMs\":" + std::to_string(action.delay_ms) +
            ",\"params\":" + action.params_json + '}';
  }
  json += "],\"sharedData\":{";
  first = true;
  for (const auto& item : result->shared_data) {
    if (!first) json += ',';
    first = false;
    json += quote(item.key) + ':' + item.value_json;
  }
  json += "}}";
  return base::ok(std::move(json));
}

}  // namespace devtool
}  // namespace lynx
