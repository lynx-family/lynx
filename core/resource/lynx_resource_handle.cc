// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/public/lynx_resource_handle.h"

#include <cctype>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/resource/trace/resource_trace_event_def.h"

namespace lynx {
namespace pub {

namespace {

bool IsAbsoluteLocalFilePath(const std::string& path) {
  if (path.empty() || path.find("://") != std::string::npos) {
    return false;
  }
#if defined(_WIN32) || defined(_WIN64)
  return (path.size() >= 3 &&
          std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':' &&
          (path[2] == '\\' || path[2] == '/')) ||
         (path.size() >= 2 && path[0] == '\\' && path[1] == '\\');
#else
  return path[0] == '/';
#endif
}

class FileResourceHandle final : public LynxResourceHandle {
 public:
  explicit FileResourceHandle(std::string file_path)
      : file_path_(std::move(file_path)) {}

  std::optional<std::string> FilePath() const override { return file_path_; }

  base::expected<std::vector<uint8_t>, std::string> ReadAllBytes()
      const override {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, RESOURCE_HANDLE_READ_ALL_BYTES);
    LOGI("[ResourceHandle] ReadAllBytes begin");
    std::ifstream input(file_path_, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
      LOGE(
          "[ResourceHandle] ReadAllBytes failure_stage=file_open, "
          "failure_reason=open_failed");
      TRACE_EVENT_INSTANT(
          LYNX_TRACE_CATEGORY, RESOURCE_HANDLE_READ_ALL_BYTES_RESULT,
          "failure_stage", "file_open", "failure_reason", "open_failed");
      return base::unexpected<std::string>("Failed to open resource file");
    }

    const std::streampos end = input.tellg();
    if (end < 0 ||
        static_cast<uint64_t>(end) >
            static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
      LOGE(
          "[ResourceHandle] ReadAllBytes failure_stage=size_validation, "
          "failure_reason=invalid_size");
      TRACE_EVENT_INSTANT(
          LYNX_TRACE_CATEGORY, RESOURCE_HANDLE_READ_ALL_BYTES_RESULT,
          "failure_stage", "size_validation", "failure_reason", "invalid_size");
      return base::unexpected<std::string>("Resource file has an invalid size");
    }

    std::vector<uint8_t> bytes(static_cast<size_t>(end));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
      input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
      if (!input) {
        LOGE(
            "[ResourceHandle] ReadAllBytes failure_stage=file_read, "
            "failure_reason=read_failed");
        TRACE_EVENT_INSTANT(
            LYNX_TRACE_CATEGORY, RESOURCE_HANDLE_READ_ALL_BYTES_RESULT,
            "failure_stage", "file_read", "failure_reason", "read_failed");
        return base::unexpected<std::string>("Failed to read resource file");
      }
    }
    LOGI("[ResourceHandle] ReadAllBytes success, byte_size=" << bytes.size());
    TRACE_EVENT_INSTANT(
        LYNX_TRACE_CATEGORY, RESOURCE_HANDLE_READ_ALL_BYTES_RESULT,
        [byte_size =
             std::to_string(bytes.size())](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_debug_annotations("result", "success");
          ctx.event()->add_debug_annotations("byte_size", byte_size);
        });
    return bytes;
  }

 private:
  const std::string file_path_;
};

}  // namespace

std::shared_ptr<LynxResourceHandle> LynxResourceHandle::CreateFile(
    std::string file_path) {
  if (!IsAbsoluteLocalFilePath(file_path)) {
    return nullptr;
  }
  return std::make_shared<FileResourceHandle>(std::move(file_path));
}

}  // namespace pub
}  // namespace lynx
