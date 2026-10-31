// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lynx_resource_handle.h"

#include <fcntl.h>
#include <sys/stat.h>

#include <cctype>
#include <cerrno>
#include <limits>
#include <string>
#include <utility>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "base/include/log/logging.h"

namespace lynx {
namespace pub {

namespace {

struct FileDescriptor {
  int value;
  ~FileDescriptor() {
    if (value < 0) return;
#ifdef _WIN32
    _close(value);
#else
    close(value);
#endif
  }
};

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
  int fd;
  do {
#ifdef _WIN32
    fd = _open(file_path.c_str(), _O_RDONLY | _O_BINARY);
#else
    fd = open(file_path.c_str(), O_RDONLY);
#endif
  } while (fd < 0 && errno == EINTR);
  const FileDescriptor input{fd};
  if (fd < 0) {
    LOGE(
        "[ResourceHandle] Materialize failure_stage=file_open, "
        "failure_reason=open_failed");
    return base::unexpected<std::string>("Failed to open resource file");
  }

#ifdef _WIN32
  struct _stat64 status;
#else
  struct stat status;
#endif
  int stat_result;
  do {
#ifdef _WIN32
    stat_result = _fstat64(fd, &status);
#else
    stat_result = fstat(fd, &status);
#endif
  } while (stat_result < 0 && errno == EINTR);
  if (stat_result < 0 || status.st_size < 0 ||
      static_cast<uint64_t>(status.st_size) >
          static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
    LOGE(
        "[ResourceHandle] Materialize failure_stage=size_validation, "
        "failure_reason=invalid_size");
    return base::unexpected<std::string>("Resource file has an invalid size");
  }

  const auto size = static_cast<size_t>(status.st_size);
  std::vector<uint8_t> bytes;
  bytes.reserve(size);
  uint8_t buffer[64 * 1024];
  while (bytes.size() < size) {
    const auto remaining = size - bytes.size();
    const auto requested =
        remaining < sizeof(buffer) ? remaining : sizeof(buffer);
#ifdef _WIN32
    const auto count = _read(fd, buffer, static_cast<unsigned int>(requested));
#else
    const auto count = read(fd, buffer, requested);
#endif
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) {
      LOGE(
          "[ResourceHandle] Materialize failure_stage=file_read, "
          "failure_reason=read_failed");
      return base::unexpected<std::string>("Failed to read resource file");
    }
    bytes.insert(bytes.end(), buffer, buffer + count);
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
