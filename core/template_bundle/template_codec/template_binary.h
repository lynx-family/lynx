// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_TEMPLATE_BINARY_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_TEMPLATE_BINARY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/linked_hash_map.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/template_bundle/template_codec/magic_number.h"

namespace lynx {
namespace tasm {

#define LEPUS_VERSION_COUNT 4

enum BinaryOffsetType {
  TYPE_STRING,
  TYPE_CSS,
  TYPE_COMPONENT,
  TYPE_PAGE_ROUTE,
  TYPE_PAGE_DATA,
  TYPE_APP,
  TYPE_JS,
  TYPE_CONFIG,
  TYPE_DYNAMIC_COMPONENT_ROUTE,
  TYPE_DYNAMIC_COMPONENT_DATA,
  TYPE_THEMED,
  TYPE_USING_DYNAMIC_COMPONENT_INFO,
  TYPE_PAGE,
  TYPE_DYNAMIC_COMPONENT,
  TYPE_SECTION_ROUTE,
  TYPE_ROOT_LEPUS,
  TYPE_ELEMENT_TEMPLATE,
  TYPE_PARSED_STYLES,
  TYPE_JS_BYTECODE,
  TYPE_LEPUS_CHUNK,
  TYPE_CUSTOM_SECTIONS,
  TYPE_NEW_ELEMENT_TEMPLATE,
  TYPE_STYLE_OBJECT,
};

enum BinarySection {
  STRING,
  CSS,
  COMPONENT,
  PAGE,
  APP,
  JS,
  CONFIG,
  DYNAMIC_COMPONENT,
  THEMED,
  USING_DYNAMIC_COMPONENT_INFO,
  SECTION_ROUTE,
  ROOT_LEPUS,
  ELEMENT_TEMPLATE,
  PARSED_STYLES,
  JS_BYTECODE,
  LEPUS_CHUNK,
  CUSTOM_SECTIONS,
  NEW_ELEMENT_TEMPLATE,
  STYLE_OBJECT,
};

enum PageSection {
  MOULD,
  CONTEXT,
  VIRTUAL_NODE_TREE,
  RADON_NODE_TREE,
};

enum DynamicComponentSection { DYNAMIC_MOULD, DYNAMIC_CONTEXT, DYNAMIC_CONFIG };

enum class CustomSectionEncodingType { STRING, JS_BYTECODE };

enum class StyleObjectSectionType {
  STYLE_OBJECT,
  STYLE_OBJECT_KEYFRAMES,
  SECTION_COUNT
};

struct Range {
  uint32_t start;
  uint32_t end;

  Range() : start(0), end(0) {}
  Range(uint32_t s, uint32_t e) : start(s), end(e) {}

  uint32_t size() const { return end - start; }

  bool operator<(const Range& rhs) const {
    return this->start < rhs.start ||
           (!(rhs.start < this->start) && this->end < rhs.end);
  }

  bool operator==(const Range& rhs) const {
    return this->start == rhs.start && this->end == rhs.end;
  }
};

using StringListVec = std::shared_ptr<std::vector<base::String>>;
typedef Range PageRange;
struct PageRoute {
  // Use linear map for reader to read as array of best performance.
  base::LinearFlatMap<int, PageRange> page_ranges;
};

typedef Range ComponentRange;
struct ComponentRoute {
  // Use linear map for reader to read as array of best performance.
  base::LinearFlatMap<int, ComponentRange> component_ranges;
};

typedef Range DynamicComponentRange;
struct DynamicComponentRoute {
  // Use linear map for reader to read as array of best performance.
  base::LinearFlatMap<int, DynamicComponentRange> dynamic_component_ranges;
};

typedef Range CSSRange;
struct CSSRoute {
  base::OrderedFlatMap<int, CSSRange> fragment_ranges;
};

struct StyleObjectRoute {
  std::vector<CSSRange> style_object_ranges;
};

typedef Range LepusChunkRange;
struct LepusChunkRoute {
  std::unordered_map<std::string, LepusChunkRange> lepus_chunk_ranges;
};

struct StringKeyRouter {
  uint32_t descriptor_offset_ = 0;
  std::unordered_map<std::string, uint32_t> start_offsets_;
};

struct OrderedStringKeyRouter {
  uint32_t descriptor_offset_ = 0;
  base::LinkedHashMap<std::string, uint32_t> start_offsets_;
};

typedef Range AirParsedStylesRange;
struct AirParsedStylesRoute {
  uint32_t descriptor_offset_;
  std::unordered_map<std::string,
                     std::unordered_map<std::string, AirParsedStylesRange>>
      parsed_styles_ranges_;
};

struct CustomSectionHeader {
 public:
  CustomSectionHeader() = default;
  CustomSectionHeader(lepus::Value header, Range range)
      : header(std::move(header)), range(std::move(range)) {}

  lepus::Value header{};
  Range range{0, 0};
};

struct CustomSectionRoute {
  uint32_t descriptor_offset;
  std::unordered_map<std::string, CustomSectionHeader> custom_section_headers;
};

class TemplateBinary {
 public:
  struct SectionInfo {
    BinarySection type_;
    uint32_t start_offset_;
    uint32_t end_offset_;
  };

  TemplateBinary(const char* lepus_version, const std::string& cli_version)
      : lepus_version_(lepus_version), cli_version_(cli_version) {}

  void AddSection(BinarySection sec, uint32_t start_offset,
                  uint32_t end_offset) {
    SectionInfo info = {sec, start_offset, end_offset};
    section_ary_.push_back(info);
  }

 public:
  typedef std::vector<SectionInfo> SectionList;

  uint32_t magic_word_;
  const char* lepus_version_;
  uint8_t section_count_;
  SectionList section_ary_;

  uint32_t total_size_;

  const std::string cli_version_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_TEMPLATE_BINARY_H_
