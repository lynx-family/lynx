// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/repack_binary_reader.h"

#include "core/runtime/vm/lepus/base_binary_reader.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/template_bundle/template_codec/header_ext_info.h"

namespace lynx {
namespace tasm {

bool RepackBinaryReader::DecodeHeader() {
  stream_->Seek(0);

  // Almost same with TemplateBinaryHeaderReader::DecodeHeader, so just copy the
  // codes. :)
  // TODO(liuli): remove duplicate code
  DECODE_U32(total_size);
  if (total_size != stream_->size()) {
    error_message_ = "template file has broken";
    return false;
  }

  bool is_lepusng = false;
  DECODE_U32(magic_word);
  if (magic_word == template_codec::kQuickBinaryMagic) {
    is_lepusng = true;
  } else if (magic_word == template_codec::kLepusBinaryMagic) {
    is_lepusng = false;
  } else {
    return false;
  }

  // lepus_version is deprecated
  // now it is just used to be compatible with prev version
  std::string lepus_version;  // deprecated
  std::string error;
  ERROR_UNLESS(ReadStringDirectly(&lepus_version));

  std::string target_sdk_version;
  if (lepus_version > MIN_SUPPORTED_VERSION) {
    // cli_version is deprecated
    // now ios_version == android_version == target_cli_version
    std::string cli_version;  // deprecated
    std::string ios_version;
    std::string android_version;
    ERROR_UNLESS(ReadStringDirectly(&cli_version));  // deprecated
    ERROR_UNLESS(ReadStringDirectly(&ios_version));
    ERROR_UNLESS(ReadStringDirectly(&android_version));
    if (ios_version != "unknown") {
      if (!CheckLynxVersion(ios_version)) {
        error_message_ =
            "Decode error, version check miss, should "
            "(current lynx version >= (ios == android) >= min supported), but: "
            "ios version:" +
            ios_version + ", android_version: " + android_version +
            ", min supported lynx version: " +
            MIN_SUPPORTED_LYNX_VERSION.ToString() +
            ", current lynx version: " + LYNX_VERSION.ToString();
        return false;
      }
    } else {
      LOGI("engine version is unknown! jump LynxVersion check\n");
    }
    target_sdk_version = ios_version;
  }
  compile_options_.target_sdk_version_ = target_sdk_version;

  if (is_lepusng) {
    std::shared_ptr<lepus::Context> ctx = lepus::Context::CreateContext(true);
    context_ = ctx.get();
    if (!context_) {
      error_message_ = "Cannot create correct lepus context";
      return false;
    }
  }
  return true;
}

template <typename T>
void RepackBinaryReader::ReinterpretValue(T &tgt, std::vector<uint8_t> src) {
  if (src.size() == sizeof(T)) {
    tgt = *reinterpret_cast<T *>(src.data());
  }
}

template <>
void RepackBinaryReader::ReinterpretValue(std::string &tgt,
                                          std::vector<uint8_t> src) {
  tgt = std::string((const char *)src.data(), src.size());
}

bool RepackBinaryReader::DecodeHeaderInfo() {
  // needs to be called immediately after DecodeHeader
  auto curr_offset = stream_->offset();
  header_ext_info_offset_ = curr_offset;
  HeaderExtInfo header_ext_info;
  memset(&header_ext_info, 0, sizeof(header_ext_info));
  ERROR_UNLESS(stream_->ReadData(reinterpret_cast<uint8_t *>(&header_ext_info),
                                 sizeof(header_ext_info)));
  DCHECK(header_ext_info.header_ext_info_magic_ == HEADER_EXT_INFO_MAGIC);
  for (uint32_t i = 0; i < header_ext_info.header_ext_info_field_numbers_;
       ++i) {
    ERROR_UNLESS(DecodeHeaderInfoField());
  }

#define CONVERT_FIXED_LENGTH_FIELD(type, field, id) \
  ReinterpretValue(compile_options_.field, header_info_map_[id])
  FOREACH_FIXED_LENGTH_FIELD(CONVERT_FIXED_LENGTH_FIELD)
#undef CONVERT_FIXED_LENGTH_FIELD

#define CONVERT_STRING_FIELD(field, id) \
  ReinterpretValue(compile_options_.field, header_info_map_[id])
  FOREACH_STRING_FIELD(CONVERT_STRING_FIELD)
#undef CONVERT_STRING_FIELD

  header_info_map_.clear();
  header_ext_info_size_ = header_ext_info.header_ext_info_size;
  stream_->Seek(curr_offset + header_ext_info_size_);
  return true;
}

bool RepackBinaryReader::DecodeHeaderInfoField() {
  HeaderExtInfo::HeaderExtInfoField header_info_field;
  auto size_to_read = sizeof(header_info_field) - sizeof(void *);
  ERROR_UNLESS(stream_->ReadData(
      reinterpret_cast<uint8_t *>(&header_info_field), (int)size_to_read));

  auto pay_load = std::vector<uint8_t>(header_info_field.payload_size_);
  ERROR_UNLESS(stream_->ReadData(reinterpret_cast<uint8_t *>(pay_load.data()),
                                 header_info_field.payload_size_));

  DCHECK(header_info_map_.find(header_info_field.key_id_) ==
         header_info_map_.end());
  header_info_map_[header_info_field.key_id_] = pay_load;
  return true;
}

bool RepackBinaryReader::DecodeString() {
  // We DO NOT decode too much in the process of mixing data. We just append
  // the encoded string of mixin data to the end of original string section.
  // This may cause the increase of the size of template, but we assume that
  // it is controllable.
  stream_->Seek(type_offset_map_[BinaryOffsetType::TYPE_STRING].start);
  DECODE_COMPACT_U32(count);
  string_offset_ = stream_->offset();
  base::StringTable *string_table = context_->string_table();
  string_table->string_list.resize(count);
  return true;
}

bool RepackBinaryReader::DecodeSuffix() {
  stream_->Seek(size() - sizeof(uint32_t));
  DECODE_U32(magic_word);
  if (magic_word != template_codec::kTasmSsrSuffixMagic) {
    error_code_ = ERR_NOT_SSR;
    error_message_ = "Not Valid SSR Template";
    return false;
  }
  stream_->Seek(size() - 2 * sizeof(uint32_t));
  DECODE_U32(suffix_size);
  suffix_size_ = suffix_size;
  stream_->Seek(size() - suffix_size);
  DECODE_U8(offset_count);
  for (int i = 0; i < offset_count; ++i) {
    DECODE_U8(type);
    if (type == BinaryOffsetType::TYPE_DYNAMIC_COMPONENT_ROUTE) {
      is_card_ = false;
    }
    DECODE_U32(start);
    DECODE_U32(end);
    type_offset_map_.insert(std::make_pair(type, Range(start, end)));
  }
  return true;
}

bool RepackBinaryReader::DecodePageRoute(PageRoute &route) {
  stream_->Seek(type_offset_map_[BinaryOffsetType::TYPE_PAGE_ROUTE].start);
  DECODE_COMPACT_U32(size);
  for (int i = 0; i < size; ++i) {
    DECODE_COMPACT_S32(id);
    DECODE_COMPACT_U32(start);
    DECODE_COMPACT_U32(end);
    route.page_ranges.insert({id, PageRange(start, end)});
  }
  return true;
}

bool RepackBinaryReader::DecodeDynamicComponentRoute(
    DynamicComponentRoute &route) {
  stream_->Seek(
      type_offset_map_[BinaryOffsetType::TYPE_DYNAMIC_COMPONENT_ROUTE].start);
  DECODE_COMPACT_U32(size);
  for (int i = 0; i < size; ++i) {
    DECODE_COMPACT_S32(id);
    DECODE_COMPACT_U32(start);
    DECODE_COMPACT_U32(end);
    route.dynamic_component_ranges.insert(
        {id, DynamicComponentRange(start, end)});
  }
  return true;
}

bool RepackBinaryReader::CheckLynxVersion(const std::string &binary_version) {
  base::Version client_version(LYNX_VERSION);
  base::Version min_supported_lynx_version(MIN_SUPPORTED_LYNX_VERSION);
  base::Version binary_lynx_version(binary_version);

  // binary_lynx_version should in this range:
  // min_supported_lynx_version  <= binary_lynx_version <= client_version
  if (binary_lynx_version < min_supported_lynx_version ||
      client_version < binary_lynx_version) {
    return false;
  }

  return true;
}

}  // namespace tasm
}  // namespace lynx
