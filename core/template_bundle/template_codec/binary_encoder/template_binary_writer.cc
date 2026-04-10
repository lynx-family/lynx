// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/template_bundle/template_codec/binary_encoder/template_binary_writer.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>

#include "base/include/sorted_for_each.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/js/bytecode/quickjs/bytecode/quickjs_bytecode_provider.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/template_bundle/template_codec/binary_encoder/style_object_encoder/style_object_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/template_section_recorder.h"
#include "core/template_bundle/template_codec/generator/source_generator.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {
size_t TemplateBinaryWriter::Encode() {
  // Encode header
  EncodeHeader();

  // Write app type
  WriteStringDirectly(app_type_.c_str());

  // Write snapshot
  WriteU8(false);

  // common encode logic for flexible and node flexible template
  auto common_encode_func = [this]() {
    // Encode css section
    EncodeCSSDescriptor();

    if (compile_options_.enable_simple_styling_) {
      // Encode simple styling objects.
      EncodeSimpleStyleObjects();
    }

    // Encode JS section
    if (compile_options_.encode_quickjs_bytecode_) {
      EncodeJsBytecode();
    } else {
      SerializeJSSource();
    }

    if (compile_options_.enable_fiber_arch_) {
      // Encode page config
      EncodeConfig();

      // Encode Lepus Code
      EncodeLepusSection();

      // Encode Lepus Chunk
      EncodeLepusChunkSection();

      // Encode Element Template
      EncodeElementTemplateSection();

      // Encode Parsed Styles, for Style Extraction
      EncodeParsedStylesSection();
    }

    // Encode Custom Sections Section
    EncodeCustomSection();
  };

  if (compile_options_.enable_flexible_template_) {
    return EncodeFlexibleTemplateBody(common_encode_func);
  }
  return EncodeNonFlexibleTemplateBody(common_encode_func);
}

size_t TemplateBinaryWriter::EncodeNonFlexibleTemplateBody(
    std::function<void()> encode_func) {
  // Write section count
  EncodeSectionCount(app_type_);

  // Record size after write section count, and record this size as header size
  header_size_ = stream_->size();

  encode_func();

  binary_info_.total_size_ = stream_->size();
  return stream_->size();
}

const std::vector<uint8_t> TemplateBinaryWriter::EncodeCSSFragmentToVector(
    encoder::SharedCSSFragment* fragment) {
  auto original_stream = std::move(stream_);
  stream_ = std::make_unique<lepus::ByteArrayOutputStream>();

  EncodeCSSFragment(fragment);

  auto buffer = stream_->byte_array();
  stream_ = std::move(original_stream);
  return buffer;
}

void TemplateBinaryWriter::EncodeCustomSection() {
  // currently only flexible templates are supported
  if (!compile_options_.enable_flexible_template_) {
    return;
  }

  if (custom_sections_ == nullptr || !custom_sections_->IsObject() ||
      custom_sections_->GetObject().MemberCount() == 0) {
    return;
  }

  TemplateSectionRecorder recorder(
      BinarySection::CUSTOM_SECTIONS, BinaryOffsetType::TYPE_CUSTOM_SECTIONS,
      this, stream_.get(), binary_info_, offset_map_, section_size_info_);
  uint32_t descriptor_offset = stream()->size();
  uint32_t start = 0;
  uint32_t end = 0;
  CustomSectionHeaders route;

  base::SortedForEach(
      custom_sections_->GetObject(),
      [this, &descriptor_offset, &route, &start, &end](const auto& it) {
        auto custom_section = lynx::lepus::jsonValueTolepusValue(it.value);
        auto section_table = custom_section.Table();

        constexpr char kCustomSectionContent[] = "content";
        auto content_iter = section_table->find(kCustomSectionContent);
        if (content_iter == section_table->end()) {
          return;
        }

        constexpr char kCustomSectionEncoding[] = "encoding";
        auto encoding_iter = section_table->find(kCustomSectionEncoding);
        CustomSectionEncodingType encoding_type =
            CustomSectionEncodingType::STRING;
        if (encoding_iter != section_table->end()) {
          if (encoding_iter->second.IsString() &&
              encoding_iter->second.StdString() == "JsBytecode") {
            encoding_type = CustomSectionEncodingType::JS_BYTECODE;
            // currently only support lepusng
            if (!IsLepusNGContext()) {
              throw lepus::CompileException(
                  "CustomSections's encoding:JS_BYTECODE only support "
                  "LepusNG!");
            }
          } else if (encoding_iter->second.IsString() &&
                     encoding_iter->second.StdString() == "CSS") {
            encoding_type = CustomSectionEncodingType::CSS;
          }
        }

        switch (encoding_type) {
          case CustomSectionEncodingType::STRING:
            EncodeValue(&content_iter->second, false);
            break;
          case CustomSectionEncodingType::JS_BYTECODE: {
            auto error = lepus::BytecodeGenerator::GenerateBytecode(
                mts_context(), content_iter->second.StdString(),
                compile_options_.target_sdk_version_);
            if (!error.empty()) {
              throw lepus::CompileException(error.c_str());
            }
            if (IsLepusNGContext()) {
              auto debug_info = GetDebugInfo();
              lepus_debug_info_.AddDebugInfo(it.name.GetString(), debug_info,
                                             quick_context());
            }
            ContextBinaryWriter::encode();
            break;
          }
          case CustomSectionEncodingType::CSS: {
            const auto& json_content = it.value["content"];
            if (json_content.IsObject() && json_content.HasMember("ruleList") &&
                json_content["ruleList"].IsArray()) {
              auto fragment = css_parser_->ParseExternalFragment(
                  json_content["ruleList"], it.name.GetString());
              auto buffer = EncodeCSSFragmentToVector(fragment.get());
              const size_t length = buffer.size();
              std::unique_ptr<uint8_t[]> ptr;
              if (length > 0) {
                ptr.reset(new uint8_t[length]);
                std::memcpy(ptr.get(), buffer.data(), length);
              }
              auto byte_array =
                  lepus::ByteArray::Create(std::move(ptr), length);
              lepus::Value byte_array_value(byte_array);
              EncodeValue(&byte_array_value, false);
            }
            break;
          }
        }

        // Record end, update start, and write these info into route.
        end = stream()->size() - descriptor_offset;

        section_table->Erase(kCustomSectionContent);
        route.emplace_back(it.name.GetString(),
                           CustomSectionHeader{lepus::Value(custom_section),
                                               Range(start, end)});
        start = end;
      },
      [](const auto& left, const auto& right) {
        return left.name.GetString() < right.name.GetString();
      });

  start = stream()->size();
  // Encode custom sections route
  EncodeCustomSectionRoute(route);
  end = stream()->size();

  // Insert route before custom sections
  stream_->Move(descriptor_offset, start, end - start);
}

uint32_t TemplateBinaryWriter::ResolveHeaderMagic() const {
  if (ctx_type_ == runtime::ContextType::LepusNGContextType) {
    return template_codec::kQuickBinaryMagic;
  }
  return template_codec::kLepusBinaryMagic;
}

void TemplateBinaryWriter::EncodeSectionCount(const std::string& app_type) {
  // count of sections, TODO:....
  uint8_t count = 7;
  if (app_type == "DynamicComponent") {
    count -= 2;
  }

  WriteU8(count);
  binary_info_.section_count_ = count;
}

void TemplateBinaryWriter::EncodeLepusSection() {
  {
    TemplateSectionRecorder recorder(
        BinarySection::ROOT_LEPUS, BinaryOffsetType::TYPE_ROOT_LEPUS, this,
        stream_.get(), binary_info_, offset_map_, section_size_info_);

    auto error = lepus::BytecodeGenerator::GenerateBytecode(
        mts_context(), lepus_code_, compile_options_.target_sdk_version_,
        lepus_code_filename_);

    if (IsLepusNGContext()) {
      auto debug_info = GetDebugInfo();
      // "lepusNG_debug_info" is hardcoded in various places in lynx sdk, tasm &
      // oliver. Use this name for now.
      lepus_debug_info_.AddDebugInfo("lepusNG_debug_info", debug_info,
                                     quick_context());
    }

    // if error occurred in Compile, terminate the encode process.
    if (!error.empty()) {
      throw lepus::CompileException(error.c_str());
    }
    ContextBinaryWriter::encode();
  }
}

// The format of "parsed_styles_" is as follows:
//  {
//    // parsedStyle_0
//    "parsedStyle_0_key": [
//    { // StyleRule_0
//        "type": "StyleRule",
//        "style": [
//         {
//              "name": "border",
//              "value": "1px solid red",
//              "keyLoc": {
//                  "line": 1,
//                  "column": 18
//              },
//              "valLoc": {
//                  "column": 33,
//                  "line": 1
//              }
//         },
//         {
//              "name": "width",
//              "value": "750rpx",
//              "keyLoc": {
//                  "line": 1,
//                  "column": 38
//              },
//              "valLoc": {
//                  "column": 46,
//                  "line": 1
//              }
//         },],
//         "variables": {
//              "--main-bg-color": "blue",
//         },
//         "selectorText": {
//              "value": ".container",
//              "loc": {
//                  "column": 11,
//                  "line": 1
//              }
//         }
//    },
//    { // StyleRule_1
//    },
//    { // StyleRule_2
//    },],
//    // parsedStyle_1
//    "parsedStyle_1_key": [],
//    // parsedStyle_1
//    "parsedStyle_1_key": []
// }
// When executing EncodeParsedStyles, each parsedStyles corresponding to a key
// will be traversed, and all the selectors and CSS variables of the
// parsedStyles will be aggregated and written into the template.
}  // namespace tasm
}  // namespace lynx
