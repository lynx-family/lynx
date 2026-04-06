// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_TEMPLATE_SECTION_RECORDER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_TEMPLATE_SECTION_RECORDER_H_

#include <map>

#include "core/template_bundle/template_codec/binary_encoder/template_binary_writer.h"

namespace lynx {
namespace tasm {

class TemplateSectionRecorder {
 public:
  TemplateSectionRecorder(BinarySection binary_section,
                          BinaryOffsetType offset_type,
                          TemplateBinaryWriter* writer,
                          lepus::OutputStream* stream,
                          TemplateBinary& binary_info,
                          std::map<uint8_t, Range>& offset_map,
                          std::map<BinarySection, uint32_t>& section_size_info)
      : binary_section_(binary_section),
        offset_type_(offset_type),
        writer_(writer),
        stream_(stream),
        binary_info_(binary_info),
        offset_map_(offset_map),
        section_size_info_(section_size_info) {
    binary_info_start_ = stream_->size();
    writer_->WriteU8(binary_section_);
    section_start_ = stream_->size();
  }

  ~TemplateSectionRecorder() {
    binary_info_end_ = stream_->size();
    binary_info_.AddSection(binary_section_, binary_info_start_,
                            binary_info_end_);
    section_end_ = stream_->size();
    offset_map_[offset_type_] = Range(section_start_, section_end_);
    section_size_info_[binary_section_] = section_end_ - section_start_ + 1;
  }

 private:
  BinarySection binary_section_{};
  BinaryOffsetType offset_type_{};
  TemplateBinaryWriter* writer_{nullptr};
  lepus::OutputStream* stream_{nullptr};
  TemplateBinary& binary_info_;
  std::map<uint8_t, Range>& offset_map_;
  std::map<BinarySection, uint32_t>& section_size_info_;
  uint32_t section_start_{};
  uint32_t section_end_{};
  uint32_t binary_info_start_{};
  uint32_t binary_info_end_{};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_TEMPLATE_SECTION_RECORDER_H_
