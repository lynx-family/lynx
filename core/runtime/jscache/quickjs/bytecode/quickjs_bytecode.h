// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_QUICKJS_BYTECODE_QUICKJS_BYTECODE_H_
#define CORE_RUNTIME_JSCACHE_QUICKJS_BYTECODE_QUICKJS_BYTECODE_H_

#include <memory>
#include <string>
#include <utility>

#include "core/renderer/tasm/config.h"
#include "core/runtime/jsi/jsi.h"
#include "core/template_bundle/template_codec/version.h"

namespace lynx {
namespace piper {
namespace quickjs {
// structure of packed quickjs bytecode.
struct Bytecode {
  static constexpr uint32_t BYTECODE_MAGIC = 0xD8C54E17;
  static constexpr int32_t FIRST_HEADER_VERSION = 1;
  static constexpr int32_t LATEST_HEADER_VERSION = 1;

  // This struct defines the fields that all versions of the header must have at
  // the beginning and should never be modified for compatibility.
  struct BaseHeader {
    const uint32_t magic = BYTECODE_MAGIC;
    const uint32_t header_version;

    explicit BaseHeader(uint32_t header_version)
        : header_version(header_version) {}
  };

  struct HeaderV1 {
    static constexpr int32_t VERSION = 1;
    static constexpr int32_t MIN_SIZE =
        sizeof(BaseHeader) + sizeof(uint32_t) * 4;

    HeaderV1(uint32_t bytecode_size, const base::Version &target_sdk_version)
        : base_header(VERSION),
          bytecode_offset(sizeof(HeaderV1)),
          bytecode_size(bytecode_size),
          target_sdk_version_major(target_sdk_version.Major()),
          target_sdk_version_minor(target_sdk_version.Minor()) {}

    const BaseHeader base_header;

    // fields of header v1
    const uint32_t bytecode_offset;
    const uint32_t bytecode_size;
    // Quickjs engine will use target_sdk_version to determine optimization
    // methods for compatibility. Save it here to (or hold a chance to) do a
    // compatibility check before execution.
    const uint32_t target_sdk_version_major;
    const uint32_t target_sdk_version_minor;

    // New fixed-length fields are allowed to be appended here in header V1.
  };

  Bytecode(HeaderV1 header, std::shared_ptr<Buffer> raw_bytecode)
      : header(std::move(header)), raw_bytecode(std::move(raw_bytecode)) {}

  int32_t TotalSize() const {
    return header.bytecode_offset + header.bytecode_size;
  }

  HeaderV1 header;
  std::shared_ptr<Buffer> raw_bytecode;
};
}  // namespace quickjs
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_QUICKJS_BYTECODE_QUICKJS_BYTECODE_H_
