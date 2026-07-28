// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/template_binary_info.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/runtime/lepus/binary_input_stream.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/generator/base_struct.h"
#include "core/template_bundle/template_codec/template_binary.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace {

const char* BinarySectionName(lynx::tasm::BinarySection section) {
  switch (section) {
    case lynx::tasm::BinarySection::STRING:
      return "STRING";
    case lynx::tasm::BinarySection::CSS:
      return "CSS";
    case lynx::tasm::BinarySection::COMPONENT:
      return "COMPONENT";
    case lynx::tasm::BinarySection::PAGE:
      return "PAGE";
    case lynx::tasm::BinarySection::APP:
      return "APP";
    case lynx::tasm::BinarySection::JS:
      return "JS";
    case lynx::tasm::BinarySection::CONFIG:
      return "CONFIG";
    case lynx::tasm::BinarySection::DYNAMIC_COMPONENT:
      return "DYNAMIC_COMPONENT";
    case lynx::tasm::BinarySection::THEMED:
      return "THEMED";
    case lynx::tasm::BinarySection::USING_DYNAMIC_COMPONENT_INFO:
      return "USING_DYNAMIC_COMPONENT_INFO";
    case lynx::tasm::BinarySection::SECTION_ROUTE:
      return "SECTION_ROUTE";
    case lynx::tasm::BinarySection::ROOT_LEPUS:
      return "ROOT_LEPUS";
    case lynx::tasm::BinarySection::ELEMENT_TEMPLATE:
      return "ELEMENT_TEMPLATE";
    case lynx::tasm::BinarySection::PARSED_STYLES:
      return "PARSED_STYLES";
    case lynx::tasm::BinarySection::JS_BYTECODE:
      return "JS_BYTECODE";
    case lynx::tasm::BinarySection::LEPUS_CHUNK:
      return "LEPUS_CHUNK";
    case lynx::tasm::BinarySection::CUSTOM_SECTIONS:
      return "CUSTOM_SECTIONS";
    case lynx::tasm::BinarySection::NEW_ELEMENT_TEMPLATE:
      return "NEW_ELEMENT_TEMPLATE";
    case lynx::tasm::BinarySection::STYLE_OBJECT:
      return "STYLE_OBJECT";
    default:
      return "UNKNOWN";
  }
}

const char* CustomSectionEncodingName(int encoding) {
  switch (encoding) {
    case static_cast<int>(lynx::tasm::CustomSectionEncodingType::STRING):
      return "string";
    case static_cast<int>(lynx::tasm::CustomSectionEncodingType::JS_BYTECODE):
      return "js_bytecode";
    case static_cast<int>(lynx::tasm::CustomSectionEncodingType::CSS):
      return "css";
    default:
      return "unknown";
  }
}

struct Range {
  uint32_t start;
  uint32_t end;
};

lynx::tasm::DecodeResult MakeDecodeResult(int status, std::string result) {
  lynx::tasm::DecodeResult out;
  out.status = status;
  out.result = std::move(result);
  if (status != 0) {
    out.error_msg = out.result;
  }
  return out;
}

struct StringSectionStats {
  uint32_t string_count = 0;
  uint64_t total_string_bytes = 0;
  uint64_t total_length_field_bytes = 0;
  uint32_t max_len = 0;
  uint64_t empty_count = 0;
  std::array<uint64_t, 8> length_histogram{};
  std::array<uint64_t, 8> bytes_histogram{};
  std::vector<std::pair<uint32_t, uint32_t>> top_lengths;
};

struct JSFileIndexInfo {
  uint32_t path_index = 0;
  uint32_t path_len = 0;
  uint32_t content_index = 0;
  uint32_t content_len = 0;
};

struct ExtractedString {
  uint32_t index = 0;
  uint32_t len = 0;
  std::string text;
};

struct JSStringUsageStats {
  uint64_t total_path_bytes = 0;
  uint64_t total_content_bytes = 0;
  uint64_t unique_path_bytes = 0;
  uint64_t unique_content_bytes = 0;
  uint32_t unique_path_count = 0;
  uint32_t unique_content_count = 0;
  std::unordered_set<uint32_t> unique_path_indices;
  std::unordered_set<uint32_t> unique_content_indices;
};

class TemplateBinaryInfoReader final : public lynx::tasm::LynxBinaryReader {
 public:
  explicit TemplateBinaryInfoReader(
      std::unique_ptr<lynx::lepus::InputStream> stream)
      : LynxBinaryReader(std::move(stream)) {}

  static TemplateBinaryInfoReader Create(std::vector<uint8_t> binary) {
    auto input_stream =
        std::make_unique<lynx::lepus::ByteArrayInputStream>(std::move(binary));
    return TemplateBinaryInfoReader(std::move(input_stream));
  }

  lynx::tasm::DecodeResult DecodeTemplateBinaryInfo() {
    if (!DecodeHeader()) {
      return MakeDecodeResult(-1, error_message_);
    }
    if (!DidDecodeHeader()) {
      return MakeDecodeResult(-1, error_message_);
    }
    if (!ReadStringDirectly(&app_type_)) {
      return MakeDecodeResult(-1, error_message_);
    }
    if (!DidDecodeAppType()) {
      return MakeDecodeResult(-1, error_message_);
    }
    uint8_t snapshot = 0;
    if (!stream_->ReadUx<uint8_t>(&snapshot)) {
      return MakeDecodeResult(-1, "Decode snapshot failed");
    }
    if (compile_options_.enable_flexible_template_) {
      if (!DecodeSectionRoute()) {
        return MakeDecodeResult(-1, error_message_);
      }
    } else {
      if (!DecodeSectionRouteLegacy()) {
        return MakeDecodeResult(-1, error_message_);
      }
    }

    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("total-size", total_size_, allocator);
    doc.AddMember("is-lepusng-binary", is_lepusng_binary_, allocator);
    doc.AddMember("engine-version",
                  rapidjson::Value(compile_options_.target_sdk_version_.c_str(),
                                   allocator),
                  allocator);
    doc.AddMember("app-type", rapidjson::Value(app_type_.c_str(), allocator),
                  allocator);

    rapidjson::Value sections(rapidjson::kObjectType);
    uint64_t sections_total = 0;

    std::vector<uint32_t> string_lengths;
    auto string_it = section_route_.find(lynx::tasm::BinarySection::STRING);
    if (string_it != section_route_.end()) {
      ParseStringSectionLengths(string_it->second.start_offset_,
                                string_it->second.end_offset_, string_lengths);
    }

    for (const auto& it : section_route_) {
      auto name = rapidjson::Value(BinarySectionName(it.first), allocator);
      const auto start = it.second.start_offset_;
      const auto end = it.second.end_offset_;
      const uint32_t size = end >= start ? (end - start) : 0;
      sections_total += size;

      rapidjson::Value section_obj(rapidjson::kObjectType);
      section_obj.AddMember("start", start, allocator);
      section_obj.AddMember("end", end, allocator);
      section_obj.AddMember("size", size, allocator);

      if (it.first == lynx::tasm::BinarySection::STRING) {
        StringSectionStats stats;
        if (ParseStringSectionStats(start, end, stats)) {
          section_obj.AddMember("string_count", stats.string_count, allocator);
          section_obj.AddMember("empty_count",
                                rapidjson::Value(stats.empty_count), allocator);
          section_obj.AddMember("total_string_bytes",
                                rapidjson::Value(stats.total_string_bytes),
                                allocator);
          section_obj.AddMember(
              "total_length_field_bytes",
              rapidjson::Value(stats.total_length_field_bytes), allocator);
          section_obj.AddMember("max_len", stats.max_len, allocator);

          rapidjson::Value histogram(rapidjson::kArrayType);
          const char* kBins[] = {"0",       "1-8",      "9-32",      "33-128",
                                 "129-512", "513-2048", "2049-8192", "8193+"};
          for (size_t i = 0; i < stats.length_histogram.size(); ++i) {
            rapidjson::Value bin(rapidjson::kObjectType);
            bin.AddMember("range", rapidjson::Value(kBins[i], allocator),
                          allocator);
            bin.AddMember("count", rapidjson::Value(stats.length_histogram[i]),
                          allocator);
            bin.AddMember("bytes", rapidjson::Value(stats.bytes_histogram[i]),
                          allocator);
            histogram.PushBack(bin, allocator);
          }
          section_obj.AddMember("histogram", histogram, allocator);

          rapidjson::Value top(rapidjson::kArrayType);
          for (const auto& it : stats.top_lengths) {
            rapidjson::Value row(rapidjson::kObjectType);
            row.AddMember("index", it.first, allocator);
            row.AddMember("len", it.second, allocator);
            top.PushBack(row, allocator);
          }
          section_obj.AddMember("top_lengths", top, allocator);
        }
      }
      sections.AddMember(name, section_obj, allocator);
    }
    doc.AddMember("sections", sections, allocator);

    rapidjson::Value chunks(rapidjson::kObjectType);

    auto js_it = section_route_.find(lynx::tasm::BinarySection::JS);
    if (js_it != section_route_.end() && !string_lengths.empty()) {
      std::vector<JSFileIndexInfo> files;
      uint32_t file_count = 0;
      if (ParseJSSourceSectionIndexInfo(js_it->second.start_offset_,
                                        js_it->second.end_offset_,
                                        string_lengths, file_count, files)) {
        std::unordered_set<uint32_t> needed_path_indices;
        needed_path_indices.reserve(files.size());
        for (const auto& f : files) {
          needed_path_indices.insert(f.path_index);
        }
        std::unordered_map<uint32_t, ExtractedString> extracted_paths;
        extracted_paths.reserve(needed_path_indices.size());
        if (string_it != section_route_.end()) {
          ParseStringSectionExtractStrings(
              string_it->second.start_offset_, string_it->second.end_offset_,
              needed_path_indices, 512, extracted_paths);
        }

        rapidjson::Value js_obj(rapidjson::kObjectType);
        js_obj.AddMember("file_count", file_count, allocator);
        rapidjson::Value file_list(rapidjson::kArrayType);
        JSStringUsageStats js_usage =
            ComputeJSStringUsage(files, string_lengths);
        for (const auto& f : files) {
          rapidjson::Value row(rapidjson::kObjectType);
          row.AddMember("path_index", f.path_index, allocator);
          row.AddMember("path_len", f.path_len, allocator);
          row.AddMember("content_index", f.content_index, allocator);
          row.AddMember("content_len", f.content_len, allocator);
          auto it = extracted_paths.find(f.path_index);
          if (it != extracted_paths.end()) {
            row.AddMember("path", rapidjson::Value(it->second.text, allocator),
                          allocator);
          }
          file_list.PushBack(row, allocator);
        }
        js_obj.AddMember("total_path_bytes",
                         rapidjson::Value(js_usage.total_path_bytes),
                         allocator);
        js_obj.AddMember("total_content_bytes",
                         rapidjson::Value(js_usage.total_content_bytes),
                         allocator);
        js_obj.AddMember("unique_path_bytes",
                         rapidjson::Value(js_usage.unique_path_bytes),
                         allocator);
        js_obj.AddMember("unique_content_bytes",
                         rapidjson::Value(js_usage.unique_content_bytes),
                         allocator);
        js_obj.AddMember("unique_path_count", js_usage.unique_path_count,
                         allocator);
        js_obj.AddMember("unique_content_count", js_usage.unique_content_count,
                         allocator);
        js_obj.AddMember("files", file_list, allocator);
        chunks.AddMember("js", js_obj, allocator);
      }
    }

    auto root_lepus_it =
        section_route_.find(lynx::tasm::BinarySection::ROOT_LEPUS);
    if (root_lepus_it != section_route_.end()) {
      rapidjson::Value root_obj(rapidjson::kObjectType);
      uint64_t code_len = 0;
      uint32_t length_field_bytes = 0;
      if (ParseRootLepusSectionStats(root_lepus_it->second.start_offset_,
                                     root_lepus_it->second.end_offset_,
                                     code_len, length_field_bytes)) {
        root_obj.AddMember("lepusng_code_len", rapidjson::Value(code_len),
                           allocator);
        root_obj.AddMember("length_field_bytes", length_field_bytes, allocator);
        root_obj.AddMember(
            "payload_bytes",
            rapidjson::Value(root_lepus_it->second.end_offset_ -
                             root_lepus_it->second.start_offset_),
            allocator);
      } else {
        root_obj.AddMember("error", rapidjson::Value("parse_failed", allocator),
                           allocator);
      }
      chunks.AddMember("root-lepus", root_obj, allocator);
    }

    if (!string_lengths.empty()) {
      rapidjson::Value string_usage(rapidjson::kObjectType);
      uint64_t total = 0;
      for (auto len : string_lengths) total += len;
      string_usage.AddMember("total_string_bytes", rapidjson::Value(total),
                             allocator);
      if (chunks.HasMember("js") && chunks["js"].IsObject()) {
        const auto& js_obj = chunks["js"];
        if (js_obj.HasMember("unique_path_bytes") &&
            js_obj.HasMember("unique_content_bytes")) {
          uint64_t used = js_obj["unique_path_bytes"].GetUint64() +
                          js_obj["unique_content_bytes"].GetUint64();
          string_usage.AddMember("js_unique_bytes", rapidjson::Value(used),
                                 allocator);
          if (total >= used) {
            string_usage.AddMember("other_bytes",
                                   rapidjson::Value(total - used), allocator);
          }
        }
      }

      if (js_it != section_route_.end() && !string_lengths.empty()) {
        std::vector<JSFileIndexInfo> files;
        uint32_t file_count = 0;
        if (ParseJSSourceSectionIndexInfo(js_it->second.start_offset_,
                                          js_it->second.end_offset_,
                                          string_lengths, file_count, files)) {
          JSStringUsageStats js_usage =
              ComputeJSStringUsage(files, string_lengths);

          std::unordered_set<uint32_t> excluded_indices =
              js_usage.unique_path_indices;
          excluded_indices.insert(js_usage.unique_content_indices.begin(),
                                  js_usage.unique_content_indices.end());
          OtherStringBreakdown breakdown;
          if (string_it != section_route_.end() &&
              ComputeOtherStringBreakdown(string_it->second.start_offset_,
                                          string_it->second.end_offset_,
                                          excluded_indices, breakdown)) {
            rapidjson::Value breakdown_obj(rapidjson::kObjectType);
            for (size_t i = 0; i < breakdown.stats.size(); ++i) {
              rapidjson::Value stat_obj(rapidjson::kObjectType);
              stat_obj.AddMember("count",
                                 rapidjson::Value(breakdown.stats[i].count),
                                 allocator);
              stat_obj.AddMember("bytes",
                                 rapidjson::Value(breakdown.stats[i].bytes),
                                 allocator);
              breakdown_obj.AddMember(
                  rapidjson::Value(OtherStringBreakdown::kTypeNames[i],
                                   allocator),
                  stat_obj, allocator);
            }
            string_usage.AddMember("other_count",
                                   rapidjson::Value(breakdown.total_count),
                                   allocator);
            string_usage.AddMember("other_bytes_sum",
                                   rapidjson::Value(breakdown.total_bytes),
                                   allocator);
            string_usage.AddMember("other_breakdown", breakdown_obj, allocator);

            rapidjson::Value other_top_by_type(rapidjson::kObjectType);
            for (size_t i = 0; i < breakdown.top.size(); ++i) {
              rapidjson::Value arr(rapidjson::kArrayType);
              for (const auto& s : breakdown.top[i]) {
                rapidjson::Value row(rapidjson::kObjectType);
                row.AddMember("name", rapidjson::Value(s.text, allocator),
                              allocator);
                row.AddMember("len", s.len, allocator);
                arr.PushBack(row, allocator);
              }
              other_top_by_type.AddMember(
                  rapidjson::Value(OtherStringBreakdown::kTypeNames[i],
                                   allocator),
                  arr, allocator);
            }
            string_usage.AddMember("other_top_by_type", other_top_by_type,
                                   allocator);

            rapidjson::Value other_all_by_type(rapidjson::kObjectType);
            for (size_t i = 0; i < breakdown.all.size(); ++i) {
              rapidjson::Value arr(rapidjson::kArrayType);
              for (const auto& s : breakdown.all[i]) {
                rapidjson::Value row(rapidjson::kObjectType);
                row.AddMember("name", rapidjson::Value(s.text, allocator),
                              allocator);
                row.AddMember("len", s.len, allocator);
                arr.PushBack(row, allocator);
              }
              other_all_by_type.AddMember(
                  rapidjson::Value(OtherStringBreakdown::kTypeNames[i],
                                   allocator),
                  arr, allocator);
            }
            string_usage.AddMember("other_all_by_type", other_all_by_type,
                                   allocator);
          }

          std::vector<std::pair<uint32_t, uint32_t>> other_top;
          other_top.reserve(20);
          for (uint32_t idx = 0; idx < string_lengths.size(); ++idx) {
            if (js_usage.unique_path_indices.count(idx) ||
                js_usage.unique_content_indices.count(idx)) {
              continue;
            }
            const auto len = string_lengths[idx];
            if (len == 0) continue;
            if (other_top.size() < 20) {
              other_top.emplace_back(idx, len);
              std::sort(other_top.begin(), other_top.end(),
                        [](const auto& a, const auto& b) {
                          return a.second > b.second;
                        });
            } else if (len > other_top.back().second) {
              other_top.back() = {idx, len};
              std::sort(other_top.begin(), other_top.end(),
                        [](const auto& a, const auto& b) {
                          return a.second > b.second;
                        });
            }
          }
          rapidjson::Value other_top_arr(rapidjson::kArrayType);
          for (const auto& p : other_top) {
            rapidjson::Value row(rapidjson::kObjectType);
            row.AddMember("index", p.first, allocator);
            row.AddMember("len", p.second, allocator);
            other_top_arr.PushBack(row, allocator);
          }
          string_usage.AddMember("other_top_lengths", other_top_arr, allocator);

          std::unordered_set<uint32_t> other_indices;
          other_indices.reserve(other_top.size());
          for (const auto& p : other_top) {
            other_indices.insert(p.first);
          }
          std::unordered_map<uint32_t, ExtractedString> extracted_other;
          extracted_other.reserve(other_indices.size());
          if (string_it != section_route_.end()) {
            ParseStringSectionExtractStrings(
                string_it->second.start_offset_, string_it->second.end_offset_,
                other_indices, 160, extracted_other);
          }

          rapidjson::Value other_named_arr(rapidjson::kArrayType);
          for (const auto& p : other_top) {
            rapidjson::Value row(rapidjson::kObjectType);
            row.AddMember("index", p.first, allocator);
            row.AddMember("len", p.second, allocator);
            auto it = extracted_other.find(p.first);
            if (it != extracted_other.end()) {
              row.AddMember("name",
                            rapidjson::Value(it->second.text, allocator),
                            allocator);
            }
            other_named_arr.PushBack(row, allocator);
          }
          string_usage.AddMember("other_top_named", other_named_arr, allocator);
        }
      }
      chunks.AddMember("string-usage", string_usage, allocator);
    }

    auto lepus_it = section_route_.find(lynx::tasm::BinarySection::LEPUS_CHUNK);
    if (lepus_it != section_route_.end()) {
      std::unordered_map<std::string, Range> lepus_chunks;
      if (ParseLepusChunkRanges(lepus_it->second.start_offset_, lepus_chunks)) {
        rapidjson::Value lepus_obj(rapidjson::kObjectType);
        uint64_t lepus_total = 0;
        for (const auto& kv : lepus_chunks) {
          const auto size = kv.second.end >= kv.second.start
                                ? (kv.second.end - kv.second.start)
                                : 0;
          lepus_total += size;
          rapidjson::Value range_obj(rapidjson::kObjectType);
          range_obj.AddMember("start", kv.second.start, allocator);
          range_obj.AddMember("end", kv.second.end, allocator);
          range_obj.AddMember("size", size, allocator);
          lepus_obj.AddMember(rapidjson::Value(kv.first.c_str(), allocator),
                              range_obj, allocator);
        }
        chunks.AddMember("lepus", lepus_obj, allocator);
        doc.AddMember("lepus-chunks-total", rapidjson::Value(lepus_total),
                      allocator);
      }
    }

    auto custom_it =
        section_route_.find(lynx::tasm::BinarySection::CUSTOM_SECTIONS);
    if (custom_it != section_route_.end()) {
      std::unordered_map<std::string, std::pair<Range, int>> custom_sections;
      if (ParseCustomSectionRanges(custom_it->second.start_offset_,
                                   custom_sections)) {
        rapidjson::Value custom_obj(rapidjson::kObjectType);
        uint64_t custom_total = 0;
        for (const auto& kv : custom_sections) {
          const auto start = kv.second.first.start;
          const auto end = kv.second.first.end;
          const auto size = end >= start ? (end - start) : 0;
          custom_total += size;
          rapidjson::Value range_obj(rapidjson::kObjectType);
          range_obj.AddMember("start", start, allocator);
          range_obj.AddMember("end", end, allocator);
          range_obj.AddMember("size", size, allocator);
          range_obj.AddMember(
              "encoding",
              rapidjson::Value(CustomSectionEncodingName(kv.second.second),
                               allocator),
              allocator);
          custom_obj.AddMember(rapidjson::Value(kv.first.c_str(), allocator),
                               range_obj, allocator);
        }
        chunks.AddMember("custom-sections", custom_obj, allocator);
        doc.AddMember("custom-sections-total", rapidjson::Value(custom_total),
                      allocator);
      }
    }

    doc.AddMember("chunks", chunks, allocator);
    doc.AddMember("sections-total", rapidjson::Value(sections_total),
                  allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return MakeDecodeResult(0, buffer.GetString());
  }

 private:
  bool ParseLepusChunkRanges(
      uint32_t section_start,
      std::unordered_map<std::string, Range>& out_ranges) {
    stream_->Seek(section_start);
    uint8_t type = 0;
    if (!stream_->ReadUx<uint8_t>(&type)) {
      return false;
    }
    if (static_cast<lynx::tasm::BinarySection>(type) !=
        lynx::tasm::BinarySection::LEPUS_CHUNK) {
      return false;
    }

    uint32_t count = 0;
    if (!stream_->ReadCompactU32(&count)) {
      return false;
    }

    struct RelRange {
      std::string key;
      uint32_t start;
      uint32_t end;
    };
    std::vector<RelRange> rels;
    rels.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
      std::string path;
      if (!ReadStringDirectly(&path)) {
        return false;
      }
      uint32_t start = 0;
      uint32_t end = 0;
      if (!stream_->ReadCompactU32(&start) || !stream_->ReadCompactU32(&end)) {
        return false;
      }
      rels.push_back({std::move(path), start, end});
    }

    const auto descriptor_offset = static_cast<uint32_t>(stream_->offset());
    for (auto& rr : rels) {
      out_ranges.emplace(std::move(rr.key), Range{descriptor_offset + rr.start,
                                                  descriptor_offset + rr.end});
    }
    return true;
  }

  bool ParseCustomSectionRanges(
      uint32_t section_start,
      std::unordered_map<std::string, std::pair<Range, int>>& out_ranges) {
    stream_->Seek(section_start);
    uint8_t type = 0;
    if (!stream_->ReadUx<uint8_t>(&type)) {
      return false;
    }
    if (static_cast<lynx::tasm::BinarySection>(type) !=
        lynx::tasm::BinarySection::CUSTOM_SECTIONS) {
      return false;
    }

    uint32_t size = 0;
    if (!stream_->ReadUx<uint32_t>(&size)) {
      return false;
    }

    struct RelSection {
      std::string key;
      lynx::lepus::Value header;
      uint32_t start;
      uint32_t end;
    };
    std::vector<RelSection> rels;
    rels.reserve(size);
    for (uint32_t i = 0; i < size; ++i) {
      std::string key;
      if (!ReadStringDirectly(&key)) {
        return false;
      }
      lynx::lepus::Value header;
      if (!DecodeValue(&header, false)) {
        return false;
      }
      uint32_t start = 0;
      uint32_t end = 0;
      if (!stream_->ReadUx<uint32_t>(&start) ||
          !stream_->ReadUx<uint32_t>(&end)) {
        return false;
      }
      rels.push_back({std::move(key), std::move(header), start, end});
    }

    const auto descriptor_offset = static_cast<uint32_t>(stream_->offset());
    for (auto& rs : rels) {
      int encoding =
          static_cast<int>(lynx::tasm::CustomSectionEncodingType::STRING);
      if (rs.header.IsTable()) {
        BASE_STATIC_STRING_DECL(kEncodingType, "encoding");
        auto maybe_encoding_type = rs.header.GetProperty(kEncodingType);
        if (maybe_encoding_type.IsNumber()) {
          encoding = static_cast<int>(maybe_encoding_type.Number());
        }
      }
      out_ranges.emplace(std::move(rs.key),
                         std::make_pair(Range{descriptor_offset + rs.start,
                                              descriptor_offset + rs.end},
                                        encoding));
    }
    return true;
  }

  bool ParseStringSectionStats(uint32_t section_start, uint32_t section_end,
                               StringSectionStats& out) {
    stream_->Seek(section_start);
    uint8_t type = 0;
    if (!stream_->ReadUx<uint8_t>(&type)) {
      return false;
    }
    if (static_cast<lynx::tasm::BinarySection>(type) !=
        lynx::tasm::BinarySection::STRING) {
      return false;
    }

    uint32_t count = 0;
    if (!stream_->ReadCompactU32(&count)) {
      return false;
    }
    out.string_count = count;

    const uint32_t kTopN = 20;
    out.top_lengths.clear();
    out.top_lengths.reserve(kTopN);

    for (uint32_t i = 0; i < count; ++i) {
      uint32_t len = 0;
      auto before = stream_->offset();
      if (!stream_->ReadCompactU32(&len)) {
        return false;
      }
      auto after = stream_->offset();
      if (after > section_end || after < before) {
        return false;
      }
      out.total_length_field_bytes += (after - before);

      if (len == 0) {
        out.empty_count += 1;
      }
      out.total_string_bytes += len;
      if (len > out.max_len) {
        out.max_len = len;
      }

      size_t bin = 0;
      if (len == 0) {
        bin = 0;
      } else if (len <= 8) {
        bin = 1;
      } else if (len <= 32) {
        bin = 2;
      } else if (len <= 128) {
        bin = 3;
      } else if (len <= 512) {
        bin = 4;
      } else if (len <= 2048) {
        bin = 5;
      } else if (len <= 8192) {
        bin = 6;
      } else {
        bin = 7;
      }
      out.length_histogram[bin] += 1;
      out.bytes_histogram[bin] += len;

      if (len > 0) {
        if (static_cast<uint64_t>(stream_->offset()) + len > section_end) {
          return false;
        }
        Skip(len);
      }

      if (out.top_lengths.size() < kTopN) {
        out.top_lengths.emplace_back(i, len);
        std::sort(
            out.top_lengths.begin(), out.top_lengths.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
      } else if (len > out.top_lengths.back().second) {
        out.top_lengths.back() = {i, len};
        std::sort(
            out.top_lengths.begin(), out.top_lengths.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
      }
    }

    return stream_->offset() <= section_end;
  }

  bool ParseStringSectionLengths(uint32_t section_start, uint32_t section_end,
                                 std::vector<uint32_t>& out_lengths) {
    out_lengths.clear();
    stream_->Seek(section_start);
    uint8_t type = 0;
    if (!stream_->ReadUx<uint8_t>(&type)) {
      return false;
    }
    if (static_cast<lynx::tasm::BinarySection>(type) !=
        lynx::tasm::BinarySection::STRING) {
      return false;
    }
    uint32_t count = 0;
    if (!stream_->ReadCompactU32(&count)) {
      return false;
    }
    out_lengths.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
      uint32_t len = 0;
      if (!stream_->ReadCompactU32(&len)) {
        return false;
      }
      if (stream_->offset() > section_end) {
        return false;
      }
      out_lengths[i] = len;
      if (len > 0) {
        if (static_cast<uint64_t>(stream_->offset()) + len > section_end) {
          return false;
        }
        Skip(len);
      }
    }
    return stream_->offset() <= section_end;
  }

  bool ParseStringSectionExtractStrings(
      uint32_t section_start, uint32_t section_end,
      const std::unordered_set<uint32_t>& indices, uint32_t max_bytes,
      std::unordered_map<uint32_t, ExtractedString>& out) {
    out.clear();
    stream_->Seek(section_start);
    uint8_t type = 0;
    if (!stream_->ReadUx<uint8_t>(&type)) {
      return false;
    }
    if (static_cast<lynx::tasm::BinarySection>(type) !=
        lynx::tasm::BinarySection::STRING) {
      return false;
    }
    uint32_t count = 0;
    if (!stream_->ReadCompactU32(&count)) {
      return false;
    }
    for (uint32_t idx = 0; idx < count; ++idx) {
      uint32_t len = 0;
      if (!stream_->ReadCompactU32(&len)) {
        return false;
      }
      if (stream_->offset() > section_end) {
        return false;
      }
      if (len > 0) {
        if (static_cast<uint64_t>(stream_->offset()) + len > section_end) {
          return false;
        }
        if (indices.count(idx)) {
          const uint32_t take = len;
          ExtractedString s;
          s.index = idx;
          s.len = len;
          s.text.assign(reinterpret_cast<const char*>(stream_->cursor()), take);
          out.emplace(idx, std::move(s));
        }
        Skip(len);
      } else {
        if (indices.count(idx)) {
          ExtractedString s;
          s.index = idx;
          s.len = 0;
          s.text = "";
          out.emplace(idx, std::move(s));
        }
      }
      if (out.size() >= indices.size()) {
        return true;
      }
    }
    return true;
  }

 private:
  bool DecodeSectionRouteLegacy() {
    section_route_.clear();
    uint8_t section_count = 0;
    if (!stream_->ReadUx<uint8_t>(&section_count)) {
      error_message_ = "Decode section_count failed";
      return false;
    }
    for (uint8_t i = 0; i < section_count; ++i) {
      const uint32_t start = static_cast<uint32_t>(stream_->offset());
      uint8_t type = 0;
      if (!stream_->ReadUx<uint8_t>(&type)) {
        error_message_ = "Decode section type failed";
        return false;
      }
      const auto section = static_cast<lynx::tasm::BinarySection>(type);
      if (!DecodeSpecificSection(section)) {
        return false;
      }
      const uint32_t end = static_cast<uint32_t>(stream_->offset());
      auto it = section_route_.find(section);
      if (it == section_route_.end()) {
        section_route_.emplace(section, lynx::tasm::TemplateBinary::SectionInfo{
                                            section, start, end});
      } else {
        it->second.end_offset_ = end;
        if (start < it->second.start_offset_) {
          it->second.start_offset_ = start;
        }
      }
    }
    return true;
  }

  struct OtherStringBreakdown {
    struct Stat {
      uint64_t count = 0;
      uint64_t bytes = 0;
    };

    static constexpr uint32_t kTopN = 20;

    enum Type : size_t {
      kEmpty = 0,
      kJson,
      kUrl,
      kCssSelector,
      kPath,
      kIdentifier,
      kOther,
      kCount
    };

    static constexpr const char* kTypeNames[kCount] = {
        "empty", "json", "url", "css_selector", "path", "identifier", "other"};

    std::array<Stat, kCount> stats{};
    std::array<std::vector<ExtractedString>, kCount> top{};
    std::array<std::vector<ExtractedString>, kCount> all{};
    uint64_t total_count = 0;
    uint64_t total_bytes = 0;
  };

  static void AppendAllStrings(std::vector<ExtractedString>& all,
                               uint32_t index, uint32_t len,
                               const uint8_t* data, uint32_t take) {
    ExtractedString s;
    s.index = index;
    s.len = len;
    if (data && take > 0) {
      s.text.assign(reinterpret_cast<const char*>(data), take);
    }
    all.emplace_back(std::move(s));
  }

  static void UpdateTopStrings(std::vector<ExtractedString>& top,
                               uint32_t index, uint32_t len,
                               const uint8_t* data, uint32_t take) {
    if (len == 0) return;
    if (take > len) take = len;
    ExtractedString s;
    s.index = index;
    s.len = len;
    if (data && take > 0) {
      s.text.assign(reinterpret_cast<const char*>(data), take);
    }
    if (top.size() < OtherStringBreakdown::kTopN) {
      top.emplace_back(std::move(s));
      std::sort(top.begin(), top.end(),
                [](const auto& a, const auto& b) { return a.len > b.len; });
      return;
    }
    if (!top.empty() && len > top.back().len) {
      top.back() = std::move(s);
      std::sort(top.begin(), top.end(),
                [](const auto& a, const auto& b) { return a.len > b.len; });
    }
  }

  static bool ContainsBytes(const uint8_t* data, size_t data_len,
                            const char* needle) {
    if (!data || !needle) return false;
    const size_t needle_len = std::strlen(needle);
    if (needle_len == 0 || needle_len > data_len) return false;
    for (size_t i = 0; i + needle_len <= data_len; ++i) {
      if (std::memcmp(data + i, needle, needle_len) == 0) {
        return true;
      }
    }
    return false;
  }

  static bool IsIdentifierLike(const uint8_t* data, size_t data_len) {
    if (!data || data_len == 0 || data_len > 128) return false;
    for (size_t i = 0; i < data_len; ++i) {
      const uint8_t c = data[i];
      const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') || c == '_' || c == '-' ||
                      c == '.' || c == ':';
      if (!ok) return false;
    }
    return true;
  }

  static OtherStringBreakdown::Type ClassifyString(const uint8_t* data,
                                                   uint32_t len,
                                                   uint32_t take) {
    if (len == 0) return OtherStringBreakdown::kEmpty;
    if (!data || take == 0) return OtherStringBreakdown::kOther;

    size_t start = 0;
    while (start < take) {
      const uint8_t c = data[start];
      if (c != ' ' && c != '\n' && c != '\r' && c != '\t') break;
      ++start;
    }
    if (start >= take) return OtherStringBreakdown::kOther;
    const uint8_t first = data[start];

    if (first == '{' || first == '[') {
      return OtherStringBreakdown::kJson;
    }
    if (ContainsBytes(data, take, "http://") ||
        ContainsBytes(data, take, "https://") ||
        ContainsBytes(data, take, "url(") ||
        ContainsBytes(data, take, "data:") ||
        ContainsBytes(data, take, "lynx://")) {
      return OtherStringBreakdown::kUrl;
    }
    if (first == '.' || first == '#' || first == '[' || first == ':' ||
        first == '*') {
      return OtherStringBreakdown::kCssSelector;
    }
    if (first == '/' || ContainsBytes(data, take, "/")) {
      if (ContainsBytes(data, take, ".ttf") ||
          ContainsBytes(data, take, ".otf") ||
          ContainsBytes(data, take, ".woff") ||
          ContainsBytes(data, take, ".woff2") ||
          ContainsBytes(data, take, ".png") ||
          ContainsBytes(data, take, ".jpg") ||
          ContainsBytes(data, take, ".jpeg") ||
          ContainsBytes(data, take, ".webp") ||
          ContainsBytes(data, take, ".gif") ||
          ContainsBytes(data, take, ".svg") ||
          ContainsBytes(data, take, ".json") ||
          ContainsBytes(data, take, ".css") ||
          ContainsBytes(data, take, ".js")) {
        return OtherStringBreakdown::kPath;
      }
    }
    if (IsIdentifierLike(data + start, take - static_cast<uint32_t>(start))) {
      return OtherStringBreakdown::kIdentifier;
    }
    return OtherStringBreakdown::kOther;
  }

  bool ComputeOtherStringBreakdown(uint32_t section_start, uint32_t section_end,
                                   const std::unordered_set<uint32_t>& excluded,
                                   OtherStringBreakdown& out) {
    out = OtherStringBreakdown{};
    stream_->Seek(section_start);
    uint8_t type = 0;
    if (!stream_->ReadUx<uint8_t>(&type)) return false;
    if (static_cast<lynx::tasm::BinarySection>(type) !=
        lynx::tasm::BinarySection::STRING) {
      return false;
    }
    uint32_t count = 0;
    if (!stream_->ReadCompactU32(&count)) return false;
    for (uint32_t idx = 0; idx < count; ++idx) {
      uint32_t len = 0;
      if (!stream_->ReadCompactU32(&len)) return false;
      if (stream_->offset() > section_end) return false;
      if (len > 0) {
        if (static_cast<uint64_t>(stream_->offset()) + len > section_end) {
          return false;
        }
      }
      if (!excluded.count(idx)) {
        const uint32_t take = std::min<uint32_t>(len, 256);
        const auto t = ClassifyString(stream_->cursor(), len, take);
        out.stats[t].count += 1;
        out.stats[t].bytes += len;
        out.total_count += 1;
        out.total_bytes += len;
        UpdateTopStrings(out.top[t], idx, len, stream_->cursor(), len);
        AppendAllStrings(out.all[t], idx, len, stream_->cursor(), len);
      }
      if (len > 0) Skip(len);
    }
    return true;
  }

  bool ParseJSSourceSectionIndexInfo(
      uint32_t section_start, uint32_t section_end,
      const std::vector<uint32_t>& string_lengths, uint32_t& out_count,
      std::vector<JSFileIndexInfo>& out_files) {
    out_files.clear();
    out_count = 0;
    stream_->Seek(section_start);
    uint8_t type = 0;
    if (!stream_->ReadUx<uint8_t>(&type)) {
      return false;
    }
    if (static_cast<lynx::tasm::BinarySection>(type) !=
        lynx::tasm::BinarySection::JS) {
      return false;
    }
    uint32_t count = 0;
    if (!ReadU32(&count)) {
      return false;
    }
    out_count = count;
    out_files.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
      uint32_t path_index = 0;
      uint32_t content_index = 0;
      if (!ReadCompactU32(&path_index) || !ReadCompactU32(&content_index)) {
        return false;
      }
      if (stream_->offset() > section_end) {
        return false;
      }
      JSFileIndexInfo info;
      info.path_index = path_index;
      info.content_index = content_index;
      if (path_index < string_lengths.size()) {
        info.path_len = string_lengths[path_index];
      }
      if (content_index < string_lengths.size()) {
        info.content_len = string_lengths[content_index];
      }
      out_files.push_back(info);
    }
    return true;
  }

  JSStringUsageStats ComputeJSStringUsage(
      const std::vector<JSFileIndexInfo>& files,
      const std::vector<uint32_t>& string_lengths) {
    JSStringUsageStats out;
    out.unique_path_indices.reserve(files.size());
    out.unique_content_indices.reserve(files.size());
    for (const auto& f : files) {
      out.total_path_bytes += f.path_len;
      out.total_content_bytes += f.content_len;
      out.unique_path_indices.insert(f.path_index);
      out.unique_content_indices.insert(f.content_index);
    }
    for (const auto idx : out.unique_path_indices) {
      if (idx < string_lengths.size()) {
        out.unique_path_bytes += string_lengths[idx];
      }
    }
    for (const auto idx : out.unique_content_indices) {
      if (idx < string_lengths.size()) {
        out.unique_content_bytes += string_lengths[idx];
      }
    }
    out.unique_path_count =
        static_cast<uint32_t>(out.unique_path_indices.size());
    out.unique_content_count =
        static_cast<uint32_t>(out.unique_content_indices.size());
    return out;
  }

  bool ParseRootLepusSectionStats(uint32_t section_start, uint32_t section_end,
                                  uint64_t& out_code_len,
                                  uint32_t& out_length_field_bytes) {
    out_code_len = 0;
    out_length_field_bytes = 0;
    stream_->Seek(section_start);
    uint8_t type = 0;
    if (!stream_->ReadUx<uint8_t>(&type)) {
      return false;
    }
    if (static_cast<lynx::tasm::BinarySection>(type) !=
        lynx::tasm::BinarySection::ROOT_LEPUS) {
      return false;
    }
    if (!is_lepusng_binary_) {
      return false;
    }
    auto before = stream_->offset();
    if (!ReadCompactU64(&out_code_len)) {
      return false;
    }
    auto after = stream_->offset();
    if (after > section_end || after < before) {
      return false;
    }
    out_length_field_bytes = static_cast<uint32_t>(after - before);
    if (static_cast<uint64_t>(stream_->offset()) + out_code_len > section_end) {
      return false;
    }
    return true;
  }
};

lynx::tasm::DecodeResult DecodeTemplateBinaryInfoInternal(const uint8_t* data,
                                                          size_t len) {
  lynx::tasm::DecodeResult out;
  out.status = -1;
  if (data == nullptr || len == 0) {
    out.error_msg = "Invalid Buffer!";
    return out;
  }

  auto reader =
      TemplateBinaryInfoReader::Create(std::vector<uint8_t>(data, data + len));
  out = reader.DecodeTemplateBinaryInfo();
  if (out.status != 0 && out.error_msg.empty()) {
    out.error_msg = out.result;
  }
  return out;
}

}  // namespace

namespace lynx {
namespace tasm {
namespace codec {

DecodeResult DecodeTemplateBinaryInfoImpl(const uint8_t* data, size_t len) {
  return DecodeTemplateBinaryInfoInternal(data, len);
}

}  // namespace codec
}  // namespace tasm
}  // namespace lynx
