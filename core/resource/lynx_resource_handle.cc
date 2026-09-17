// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lynx_resource_handle.h"

#include <cctype>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace pub {

namespace {

bool IsAbsoluteFilePath(const std::string& path) {
#ifdef _WIN32
  return (path.size() >= 3 &&
          std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':' &&
          (path[2] == '\\' || path[2] == '/')) ||
         (path.size() >= 2 && path[0] == '\\' && path[1] == '\\');
#else
  return path[0] == '/';
#endif
}

LynxResourceHandle::DataResult ReadFile(const std::string& file_path) {
  std::ifstream input(file_path, std::ios::binary | std::ios::ate);
  if (!input.is_open()) {
    LOGE(
        "[ResourceHandle] Materialize failure_stage=file_open, "
        "failure_reason=open_failed");
    return base::unexpected<std::string>("Failed to open resource file");
  }

  const std::streampos end = input.tellg();
  if (end < 0 ||
      static_cast<uint64_t>(end) >
          static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
    LOGE(
        "[ResourceHandle] Materialize failure_stage=size_validation, "
        "failure_reason=invalid_size");
    return base::unexpected<std::string>("Resource file has an invalid size");
  }

  std::vector<uint8_t> bytes(static_cast<size_t>(end));
  input.seekg(0, std::ios::beg);
  if (!bytes.empty()) {
    input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!input) {
      LOGE(
          "[ResourceHandle] Materialize failure_stage=file_read, "
          "failure_reason=read_failed");
      return base::unexpected<std::string>("Failed to read resource file");
    }
  }
  return std::make_shared<const std::vector<uint8_t>>(std::move(bytes));
}

class FileResourceHandle final : public LynxResourceHandle {
 public:
  explicit FileResourceHandle(std::string file_path)
      : file_path_(std::move(file_path)), snapshot_(ReadFile(file_path_)) {}

  std::optional<std::string> FilePath() const override { return file_path_; }

  DataResult GetData() const override { return snapshot_; }

 protected:
  base::expected<std::vector<uint8_t>, std::string> ReadAllBytes()
      const override {
    auto result = GetData();
    if (!result.has_value()) {
      return base::unexpected<std::string>(result.error());
    }
    if (result.value() == nullptr) {
      return base::unexpected<std::string>("Resource snapshot is null");
    }
    return *result.value();
  }

 private:
  const std::string file_path_;
  const DataResult snapshot_;
};

}  // namespace

LynxResourceHandle::CreateResult LynxResourceHandle::CreateFile(
    std::string file_path) {
  if (file_path.empty()) {
    return base::unexpected<std::string>(
        "Resource file path must not be empty");
  }
  if (file_path.find("://") != std::string::npos) {
    return base::unexpected<std::string>(
        "Resource file path must not use a URI scheme");
  }
  if (!IsAbsoluteFilePath(file_path)) {
    return base::unexpected<std::string>("Resource file path must be absolute");
  }
  return std::make_shared<FileResourceHandle>(std::move(file_path));
}

LynxResourceHandle::DataResult LynxResourceHandle::GetData() const {
  auto result = ReadAllBytes();
  if (!result.has_value()) {
    return base::unexpected<std::string>(result.error());
  }
  return std::make_shared<const std::vector<uint8_t>>(
      std::move(result.value()));
}

int64_t LynxResourceHandle::Size() const {
  auto result = GetData();
  if (!result.has_value() || result.value() == nullptr) {
    return -1;
  }
  return static_cast<int64_t>(result.value()->size());
}

}  // namespace pub
}  // namespace lynx
