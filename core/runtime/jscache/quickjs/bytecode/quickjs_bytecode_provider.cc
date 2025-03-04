// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider.h"

#include <memory>
#include <utility>

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider_src.h"

namespace lynx {
namespace piper {
namespace quickjs {
bool QuickjsBytecodeProvider::IsBytecode(
    const std::shared_ptr<const Buffer> &buffer) {
  if (buffer == nullptr || buffer->size() < sizeof(Bytecode::BaseHeader)) {
    return false;
  }
  return Bytecode::BYTECODE_MAGIC ==
         reinterpret_cast<const Bytecode::BaseHeader *>(buffer->data())->magic;
}

std::pair<bool, std::string> QuickjsBytecodeProvider::ValidateBytecode(
    const std::shared_ptr<const Buffer> &buffer) {
  if (!IsBytecode(buffer)) {
    return {false, "Buffer is not bytecode"};
  }

  auto *header = reinterpret_cast<const Bytecode::BaseHeader *>(buffer->data());
  if (header->header_version < Bytecode::FIRST_HEADER_VERSION ||
      header->header_version > Bytecode::LATEST_HEADER_VERSION) {
    return {false, "Invalid header version: " +
                       std::to_string(header->header_version)};
  }

  if (buffer->size() < Bytecode::HeaderV1::MIN_SIZE) {
    return {false, "Min size of header v1 not reached"};
  }
  auto *header_v1 =
      reinterpret_cast<const Bytecode::HeaderV1 *>(buffer->data());
  if (header_v1->bytecode_offset + header_v1->bytecode_size != buffer->size()) {
    return {false, "Size mismatch"};
  }
  return {true, {}};
}

std::shared_ptr<const Buffer>
QuickjsBytecodeProvider::GetPackedBytecodeBuffer() {
  std::string str;
  str.reserve(bytecode_.TotalSize());
  // TODO(zhenziqi) this works only when using header version 1.
  DCHECK(bytecode_.header.base_header.header_version ==
         Bytecode::HeaderV1::VERSION);
  str.append(reinterpret_cast<char *>(&bytecode_.header),
             bytecode_.header.bytecode_offset);
  str.append(reinterpret_cast<const char *>(bytecode_.raw_bytecode->data()),
             bytecode_.raw_bytecode->size());
  return std::make_shared<StringBuffer>(std::move(str));
}

QuickjsBytecodeProviderSrc QuickjsBytecodeProvider::FromSource(
    std::string source_url, std::shared_ptr<const Buffer> src) {
  return {std::move(source_url), std::move(src)};
}

base::expected<QuickjsBytecodeProvider, std::string>
QuickjsBytecodeProvider::FromPackedBytecode(
    const std::shared_ptr<const Buffer> &bytecode) {
  if (auto [ok, err] = ValidateBytecode(bytecode); !ok) {
    return base::unexpected(err);
  }

  auto *header_from_buffer =
      reinterpret_cast<const Bytecode::HeaderV1 *>(bytecode->data());

  // parse header
  // TODO(zhenziqi) only works in header v1 right now
  DCHECK(header_from_buffer->base_header.header_version ==
         Bytecode::HeaderV1::VERSION);
  Bytecode::HeaderV1 header(
      header_from_buffer->bytecode_size,
      base::Version(header_from_buffer->target_sdk_version_major,
                    header_from_buffer->target_sdk_version_minor));

  std::string raw_bytecode;
  raw_bytecode.reserve(header.bytecode_size);
  raw_bytecode.append(
      std::next(reinterpret_cast<const char *>(bytecode->data()),
                header.bytecode_offset),
      header.bytecode_size);

  Bytecode bc(std::move(header),
              std::make_shared<StringBuffer>(std::move(raw_bytecode)));
  return QuickjsBytecodeProvider{std::move(bc)};
}

std::shared_ptr<const Buffer> QuickjsBytecodeProvider::GetRawBytecode() {
  return bytecode_.raw_bytecode;
}

base::Version QuickjsBytecodeProvider::GetTargetSdkVersion() {
  return base::Version(bytecode_.header.target_sdk_version_major,
                       bytecode_.header.target_sdk_version_minor);
}

QuickjsBytecodeProvider::QuickjsBytecodeProvider(Bytecode &&bytecode)
    : bytecode_(std::move(bytecode)) {}
}  // namespace quickjs
}  // namespace piper
}  // namespace lynx
