// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/repack_binary_writer.h"

#include <list>
#include <string>

#include "core/runtime/vm/lepus/context.h"

namespace lynx {
namespace tasm {

void RepackBinaryWriter::EncodeString() {
  // Just change the string count and append the encoded string of mixin data to
  // the end of original string section.
  stream_.reset(new lepus::ByteArrayOutputStream());
  WriteCompactU32(context_->string_table()->string_list.size());
  string_count_vec_ = stream_->byte_array();

  stream_.reset(new lepus::ByteArrayOutputStream());
  base::StringTable* string_table = context_->string_table();
  size_t start_pos =
      string_table->string_list.size() - string_table->string_map_.size();
  for (size_t i = start_pos; i < string_table->string_list.size(); i++) {
    std::string str = string_table->string_list[i].str();
    size_t length = str.length();
    WriteCompactU32(length);

    if (length != 0) {
      stream_->WriteData(reinterpret_cast<const uint8_t*>(str.c_str()), length);
    }
  }
  string_vec_ = stream_->byte_array();
}

void RepackBinaryWriter::EncodePageRoute(const PageRoute& route) {
  stream_.reset(new lepus::ByteArrayOutputStream());
  WriteCompactU32(route.page_ranges.size());
  for (auto it = route.page_ranges.begin(); it != route.page_ranges.end();
       ++it) {
    WriteCompactS32(it->first);
    WriteCompactS32(it->second.start);
    WriteCompactS32(it->second.end);
  }
  route_vec_ = stream_->byte_array();
}

void RepackBinaryWriter::EncodeDynamicComponentRoute(
    const DynamicComponentRoute& route) {
  stream_.reset(new lepus::ByteArrayOutputStream());
  WriteCompactU32(route.dynamic_component_ranges.size());
  for (auto it = route.dynamic_component_ranges.begin();
       it != route.dynamic_component_ranges.end(); ++it) {
    WriteCompactS32(it->first);
    WriteCompactU32(it->second.start);
    WriteCompactU32(it->second.end);
  }
  route_vec_ = stream_->byte_array();
}

void RepackBinaryWriter::EncodeValue(const lepus::Value* value) {
  stream_.reset(new lepus::ByteArrayOutputStream());
  ContextBinaryWriter::EncodeValue(value);
  data_vec_ = stream_->byte_array();
}

void RepackBinaryWriter::EncodeHeaderInfo(
    const CompileOptions& compile_options) {
  stream_.reset(new lepus::ByteArrayOutputStream());
  std::list<HeaderExtInfo::HeaderExtInfoField> header_info_fields;

#define REGISTER_FIXED_LENGTH_FIELD(type, field, id)              \
  header_info_fields.push_back(HeaderExtInfo::HeaderExtInfoField{ \
      HeaderExtInfo::TYPE_##type, id, HeaderExtInfo::SIZE_##type, \
      (void*)(&compile_options.field)})

  FOREACH_FIXED_LENGTH_FIELD(REGISTER_FIXED_LENGTH_FIELD)
#undef REGISTER_FIXED_LENGTH_FIELD

#define REGISTER_STRING_FIELD(field, id)                                      \
  header_info_fields.push_back(HeaderExtInfo::HeaderExtInfoField{             \
      HeaderExtInfo::TYPE_STRING, id, (uint16_t)compile_options.field.size(), \
      (void*)(compile_options.field.c_str())})

  FOREACH_STRING_FIELD(REGISTER_STRING_FIELD)
#undef REGISTER_STRING_FIELD
  // encode section header
  uint32_t header_ext_info_total_size = 0;
  header_ext_info_total_size += sizeof(HeaderExtInfo);
  for (const auto& field : header_info_fields) {
    header_ext_info_total_size +=
        sizeof(field) - sizeof(void*) + field.payload_size_;
  }
  HeaderExtInfo header_ext_info = {header_ext_info_total_size,
                                   HEADER_EXT_INFO_MAGIC,
                                   (uint32_t)header_info_fields.size()};
  stream_->WriteData((uint8_t*)&header_ext_info, sizeof(header_ext_info));

  // encode fields
  for (const auto& field : header_info_fields) {
    EncodeHeaderInfoField(field);
  }

  header_ext_info_vec_ = stream_->byte_array();
}

void RepackBinaryWriter::AssembleNewTemplate(
    const uint8_t* ptr, size_t buf_len, size_t suffix_size,
    size_t string_offset, std::map<uint8_t, Range>& map, bool is_card,
    std::vector<uint8_t>& new_template) {
  Range string_range = map[BinaryOffsetType::TYPE_STRING];
  Range route_range = is_card
                          ? map[BinaryOffsetType::TYPE_PAGE_ROUTE]
                          : map[BinaryOffsetType::TYPE_DYNAMIC_COMPONENT_ROUTE];
  Range data_range = is_card
                         ? map[BinaryOffsetType::TYPE_PAGE_DATA]
                         : map[BinaryOffsetType::TYPE_DYNAMIC_COMPONENT_DATA];

  // TODO(liuli) more efficient way
  new_template.clear();
  new_template.insert(new_template.end(), ptr, ptr + string_range.start);
  new_template.insert(new_template.end(), string_count_vec_.begin(),
                      string_count_vec_.end());
  new_template.insert(new_template.end(), ptr + string_offset,
                      ptr + string_range.end);
  new_template.insert(new_template.end(), string_vec_.begin(),
                      string_vec_.end());
  new_template.insert(new_template.end(), ptr + string_range.end,
                      ptr + route_range.start);
  new_template.insert(new_template.end(), route_vec_.begin(), route_vec_.end());
  new_template.insert(new_template.end(), ptr + route_range.end,
                      ptr + data_range.start);
  new_template.insert(new_template.end(), data_vec_.begin(), data_vec_.end());
  new_template.insert(new_template.end(), ptr + data_range.end,
                      ptr + buf_len - suffix_size);
}

void RepackBinaryWriter::AssembleTemplateWithNewHeaderInfo(
    const uint8_t* ptr, size_t buf_len, size_t header_ext_info_offset,
    size_t header_ext_info_size, std::vector<uint8_t>& new_template) {
  new_template.clear();
  new_template.insert(new_template.end(), ptr, ptr + header_ext_info_offset);
  new_template.insert(new_template.end(), header_ext_info_vec_.begin(),
                      header_ext_info_vec_.end());
  new_template.insert(new_template.end(),
                      ptr + header_ext_info_offset + header_ext_info_size,
                      ptr + buf_len);
}

void RepackBinaryWriter::EncodeHeaderInfoField(
    const HeaderExtInfo::HeaderExtInfoField& header_info_field) {
  stream_->WriteData((const uint8_t*)(&header_info_field),
                     sizeof(header_info_field) - sizeof(void*));
  stream_->WriteData(static_cast<const uint8_t*>(header_info_field.payload_),
                     header_info_field.payload_size_);
}

}  // namespace tasm
}  // namespace lynx
