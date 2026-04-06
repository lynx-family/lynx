// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <list>
#include <set>

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

namespace lynx {
namespace tasm {
void TemplateBinaryWriter::EncodeSimpleStyleObjects() {
  TemplateSectionRecorder recorder(
      BinarySection::STYLE_OBJECT, BinaryOffsetType::TYPE_STYLE_OBJECT, this,
      stream_.get(), binary_info_, offset_map_, section_size_info_);
  if (!style_object_parser_) {
    return;
  }
  uint32_t style_object_section_count =
      static_cast<uint32_t>(StyleObjectSectionType::SECTION_COUNT);
  WriteCompactU32(style_object_section_count);
  static_assert(static_cast<uint32_t>(StyleObjectSectionType::STYLE_OBJECT) ==
                0);
  auto& style_objects = style_object_parser_->StyleObjects();
  StyleObjectRoute route;
  uint32_t descriptor_offset = stream()->size();
  uint32_t start = 0;
  uint32_t end = 0;
  if (!style_objects.empty()) {
    std::for_each(style_objects.begin(), style_objects.end(),
                  [descriptor_offset, &route, &start, &end,
                   this](const style::StyleObject& style_obj) {
                    EncodeCSSAttributes(style_obj.Properties());
                    end = stream()->size() - descriptor_offset;
                    route.style_object_ranges.emplace_back(start, end);
                    start = end;
                  });
  }
  start = stream()->size();
  EncodeSimpleStyleObjectsRoute(route);
  end = stream()->size();
  stream_->Move(descriptor_offset, start, end - start);

  static_assert(static_cast<uint32_t>(
                    StyleObjectSectionType::STYLE_OBJECT_KEYFRAMES) == 1);
  auto& style_objects_keyframes = style_object_parser_->StyleObjectsKeyframes();
  descriptor_offset = stream()->size();
  start = 0;
  end = 0;
  StyleObjectRoute keyframes_route;
  if (!style_objects_keyframes.empty()) {
    base::sorted_for_each(
        style_objects_keyframes.begin(), style_objects_keyframes.end(),
        [descriptor_offset, &keyframes_route, &start, &end,
         this](const auto& it) {
          EncodeUtf8Str(it.first.c_str(), it.first.length());
          EncodeCSSKeyframesToken(it.second.get());
          end = stream()->size() - descriptor_offset;
          keyframes_route.style_object_ranges.emplace_back(start, end);
          start = end;
        });
  }
  start = stream()->size();
  EncodeSimpleStyleObjectsRoute(keyframes_route);
  end = stream()->size();
  stream_->Move(descriptor_offset, start, end - start);

  static_assert(static_cast<uint32_t>(
                    StyleObjectSectionType::STYLE_OBJECT_FONTFACES) == 2);
  auto& style_objects_fontfaces = style_object_parser_->StyleObjectsFontFaces();
  descriptor_offset = stream()->size();
  start = 0;
  end = 0;
  StyleObjectRoute fontfaces_route;
  if (!style_objects_fontfaces.empty()) {
    base::sorted_for_each(
        style_objects_fontfaces.begin(), style_objects_fontfaces.end(),
        [descriptor_offset, &fontfaces_route, &start, &end,
         this](const auto& it) {
          EncodeUtf8Str(it.first.c_str(), it.first.length());
          EncodeCSSFontFaceTokenList(it.second);
          end = stream()->size() - descriptor_offset;
          fontfaces_route.style_object_ranges.emplace_back(start, end);
          start = end;
        });
  }

  start = stream()->size();
  EncodeSimpleStyleObjectsRoute(fontfaces_route);
  end = stream()->size();
  stream_->Move(descriptor_offset, start, end - start);
}

void TemplateBinaryWriter::EncodeSimpleStyleObjectsRoute(
    const StyleObjectRoute& route) {
  WriteCompactU32(route.style_object_ranges.size());
  std::for_each(route.style_object_ranges.begin(),
                route.style_object_ranges.end(), [this](const CSSRange& it) {
                  WriteCompactU32(it.start);
                  WriteCompactU32(it.end);
                });
}

size_t TemplateBinaryWriter::EncodeFlexibleTemplateBody(
    std::function<void()> encode_func) {
  header_size_ = stream_->size();

  encode_func();

  EncodeSectionRoute();
  MoveLastSectionToFirst(BinarySection::SECTION_ROUTE);

  binary_info_.total_size_ = stream_->size();
  return stream_->size();
}

void TemplateBinaryWriter::EncodeSectionRoute() {
  TemplateSectionRecorder recorder(
      BinarySection::SECTION_ROUTE, BinaryOffsetType::TYPE_SECTION_ROUTE, this,
      stream_.get(), binary_info_, offset_map_, section_size_info_);
  WriteCompactU32(binary_info_.section_ary_.size());
  uint32_t start_pos = binary_info_.section_ary_[0].start_offset_;
  for (const auto& info : binary_info_.section_ary_) {
    WriteU8(info.type_);
    WriteCompactU32(info.start_offset_ - start_pos);
    WriteCompactU32(info.end_offset_ - start_pos);
  }
}

void TemplateBinaryWriter::MoveLastSectionToFirst(
    const BinarySection& section) {
  DCHECK(binary_info_.section_ary_.size() > 0);
  TemplateBinary::SectionInfo info =
      binary_info_.section_ary_[binary_info_.section_ary_.size() - 1];
  DCHECK(info.type_ == section);

  uint32_t insert_pos = binary_info_.section_ary_[0].start_offset_;
  uint32_t cur_size = stream_->size();
  stream_->Move(insert_pos, info.start_offset_, cur_size - info.start_offset_);

  for (auto& kv : offset_map_) {
    kv.second.start += cur_size - info.start_offset_;
    kv.second.end += cur_size - info.start_offset_;
  }
  offset_map_[section] =
      Range(insert_pos + 1, insert_pos + cur_size - info.start_offset_);
}

bool TemplateBinaryWriter::EncodeHeaderInfo(
    const CompileOptions& compile_options) {
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

  uint32_t header_ext_info_total_size = sizeof(HeaderExtInfo);
  for (const auto& field : header_info_fields) {
    header_ext_info_total_size +=
        sizeof(field) - sizeof(void*) + field.payload_size_;
  }
  header_ext_info_ = {header_ext_info_total_size, HEADER_EXT_INFO_MAGIC,
                      (uint32_t)header_info_fields.size()};
  stream_->WriteData((uint8_t*)&header_ext_info_, sizeof(header_ext_info_));

  for (const auto& field : header_info_fields) {
    EncodeHeaderInfoField(field);
  }

  return true;
}

bool TemplateBinaryWriter::EncodeHeaderInfoField(
    const HeaderExtInfo::HeaderExtInfoField& header_info_field) {
  stream_->WriteData((const uint8_t*)(&header_info_field),
                     sizeof(header_info_field) - sizeof(void*));
  stream_->WriteData(static_cast<const uint8_t*>(header_info_field.payload_),
                     header_info_field.payload_size_);
  return true;
}

void TemplateBinaryWriter::EncodeHeader() {
  const char* ios_version = compile_options_.target_sdk_version_.c_str();
  const char* android_version = ios_version;

  uint32_t magic = ResolveHeaderMagic();
  WriteU32(magic);
  WriteStringDirectly(binary_info_.lepus_version_);
  WriteStringDirectly(binary_info_.cli_version_.c_str());
  WriteStringDirectly(ios_version);
  WriteStringDirectly(android_version);
  binary_info_.magic_word_ = magic;

  if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_HEADER_EXT_INFO_VERSION)) {
    EncodeHeaderInfo(compile_options_);
  }

  if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_TEMPLATE_INFO)) {
    EncodeValue(&template_info_, true);
  }

  if (compile_options_.enable_trial_options_) {
    EncodeValue(&trial_options_, true);
  }
}

void TemplateBinaryWriter::EncodeConfig() {
  TemplateSectionRecorder recorder(
      BinarySection::CONFIG, BinaryOffsetType::TYPE_CONFIG, this, stream_.get(),
      binary_info_, offset_map_, section_size_info_);
  EncodeUtf8Str(config_.c_str());
}

bool TemplateBinaryWriter::WriteToFile(const char* file_name) {
  stream_->WriteToFile(file_name);
  return true;
}

const std::vector<uint8_t> TemplateBinaryWriter::WriteToVector() {
  auto buffer = stream_->byte_array();
  return buffer;
}

void TemplateBinaryWriter::EncodeCSSDescriptor() {
  TemplateSectionRecorder recorder(
      BinarySection::CSS, BinaryOffsetType::TYPE_CSS, this, stream_.get(),
      binary_info_, offset_map_, section_size_info_);

  auto& fragments = css_parser_->fragments();
  CSSRoute route;
  uint32_t descriptor_offset = stream()->size();
  uint32_t start = 0;
  uint32_t end = 0;
  base::sorted_for_each(
      fragments.begin(), fragments.end(),
      [descriptor_offset, &route, &start, &end, this](const auto& it) {
        auto& fragment = it.second;
        EncodeCSSFragment(fragment);
        end = stream()->size() - descriptor_offset;
        route.fragment_ranges.insert({fragment->id(), CSSRange(start, end)});
        start = end;
      });

  start = stream()->size();
  EncodeCSSRoute(route);
  end = stream()->size();

  if (!fragments.empty()) {
    stream_->Move(descriptor_offset, start, end - start);
  }
}

void TemplateBinaryWriter::EncodeCSSRoute(const CSSRoute& css_route) {
  WriteCompactU32(css_route.fragment_ranges.size());
  base::sorted_for_each(css_route.fragment_ranges.begin(),
                        css_route.fragment_ranges.end(),
                        [this](const auto& it) {
                          WriteCompactS32(it.first);
                          WriteCompactU32(it.second.start);
                          WriteCompactU32(it.second.end);
                        });
}

void TemplateBinaryWriter::EncodeCSSFragment(
    encoder::SharedCSSFragment* fragment) {
  WriteCompactU32(fragment->id());
  WriteCompactU32(fragment->dependent_ids().size());
  for (auto id : fragment->dependent_ids()) {
    WriteCompactS32(id);
  }
  if (compile_options_.enable_css_selector_) {
    size_t selector_size = fragment->selector_tuple().size();
    WriteCompactU32(selector_size);
    for (const auto& it : fragment->selector_tuple()) {
      EncodeLynxCSSSelectorTuple(it);
    }
  }

  size_t size = fragment->css().size();
  size_t keyframes_size = fragment->GetKeyframesRuleMapForEncode().size() << 16;
  size += keyframes_size;
  WriteCompactU32(size);
  base::sorted_for_each(fragment->css().begin(), fragment->css().end(),
                        [this](const auto& it) {
                          EncodeUtf8Str(it.first.c_str(), it.first.length());
                          EncodeCSSParseToken(it.second.get());
                        });

  base::sorted_for_each(fragment->GetKeyframesRuleMapForEncode().begin(),
                        fragment->GetKeyframesRuleMapForEncode().end(),
                        [this](const auto& it) {
                          EncodeUtf8Str(it.first.c_str(), it.first.length());
                          EncodeCSSKeyframesToken(it.second.get());
                        });

  const size_t fontface_size = fragment->GetFontFaceTokenMapForEncode().size();
  if (fontface_size > 0) {
    WriteU8(CSS_BINARY_FONT_FACE_TYPE);
    WriteCompactU32(fontface_size);
    if (lynx::tasm::Config::IsHigherOrEqual(
            compile_options_.target_sdk_version_,
            FEATURE_CSS_FONT_FACE_EXTENSION)) {
      base::sorted_for_each(
          fragment->GetFontFaceTokenMapForEncode().begin(),
          fragment->GetFontFaceTokenMapForEncode().end(),
          [this](const auto& it) { EncodeCSSFontFaceTokenList(it.second); });
    } else {
      base::sorted_for_each(fragment->GetFontFaceTokenMapForEncode().begin(),
                            fragment->GetFontFaceTokenMapForEncode().end(),
                            [this](const auto& it) {
                              EncodeCSSFontFaceToken(it.second[0].get());
                            });
    }
  }
}

bool TemplateBinaryWriter::EncodeCSSParseToken(CSSParseToken* token) {
  DCHECK(token != nullptr);
  EncodeCSSAttributes(token->GetAttributes());
  if (lynx::tasm::Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                          FEATURE_CSS_STYLE_VARIABLES) &&
      compile_options_.enable_css_variable_) {
    EncodeCSSStyleVariables(token->GetStyleVariables());
  }
  if (compile_options_.enable_css_selector_) {
    return true;
  }
  const auto& sheets = token->sheets();
  size_t size = sheets.size();
  WriteCompactU32(size);

  if (sheets.size() == 0) {
    return true;
  }
  for (size_t i = 0; i < sheets.size(); i++) {
    CSSSheet* sheet = sheets[i].get();
    DCHECK(sheet != nullptr);
    EncodeCSSSheet(sheet);
  }

  return true;
}

bool TemplateBinaryWriter::EncodeLynxCSSSelectorTuple(
    const encoder::LynxCSSSelectorTuple& selector_tuple) {
  size_t flattened_size = selector_tuple.flattened_size;
  WriteCompactU32(flattened_size);
  if (flattened_size == 0 || !selector_tuple.selector_arr) {
    return true;
  }
  EncodeCSSSelector(selector_tuple.selector_arr.get());
  EncodeCSSParseToken(selector_tuple.parse_token.get());
  return true;
}

bool TemplateBinaryWriter::EncodeCSSSelector(
    const css::LynxCSSSelector* selector) {
  DCHECK(selector != nullptr);
  auto current = selector;
  while (current) {
    auto value = current->ToLepus();
    EncodeValue(&value);
    if (current->IsLastInTagHistory() && current->IsLastInSelectorList()) {
      break;
    }
    current++;
  }
  return true;
}

bool TemplateBinaryWriter::EncodeCSSKeyframesToken(
    encoder::CSSKeyframesToken* token) {
  DCHECK(token != nullptr);
  EncodeCSSKeyframesMap(token->GetKeyframes());
  return true;
}

bool TemplateBinaryWriter::EncodeCSSFontFaceTokenList(
    const std::vector<std::shared_ptr<CSSFontFaceToken>>& tokenList) {
  uint32_t size = tokenList.size();
  WriteCompactU32(size);
  if (size == 0) {
    return true;
  }
  for (size_t i = 0; i < size; i++) {
    CSSFontFaceToken* token = tokenList[i].get();
    EncodeCSSFontFaceToken(token);
  }
  return true;
}

bool TemplateBinaryWriter::EncodeCSSFontFaceToken(CSSFontFaceToken* token) {
  DCHECK(token != nullptr);
  auto attrMap = token->GetAttrMap();

  uint32_t size = attrMap.size();
  WriteCompactU32(size);

  base::sorted_for_each(attrMap.begin(), attrMap.end(),
                        [this](const auto& itr) {
                          EncodeUtf8Str(itr.first.c_str());
                          EncodeUtf8Str(itr.second.c_str());
                        });
  return true;
}

bool TemplateBinaryWriter::EncodeCSSSheet(CSSSheet* sheet) {
  DCHECK(sheet != nullptr);

  WriteCompactU32(sheet->GetType());
  EncodeUtf8Str(sheet->GetName().c_str());
  EncodeUtf8Str(sheet->GetSelector().c_str());

  return true;
}

bool TemplateBinaryWriter::EncodeCSSAttributes(const StyleMap& attrs) {
  uint32_t size = attrs.size();
  WriteCompactU32(size);

  for (auto it = attrs.begin(); it != attrs.end(); ++it) {
    WriteCompactU32(it->first);
    EncodeCSSValue(it->second);
  }
  return true;
}

bool TemplateBinaryWriter::EncodeCSSStyleVariables(
    const CSSVariableMap& style_variables) {
  uint32_t size = style_variables.size();
  WriteCompactU32(size);
  for (auto it = style_variables.begin(); it != style_variables.end(); ++it) {
    WriteStringDirectly(it->first.c_str());
    WriteStringDirectly(it->second.c_str());
  }
  return true;
}

bool TemplateBinaryWriter::EncodeCSSKeyframesMap(
    const CSSKeyframesMap& keyframes) {
  uint32_t size = keyframes.size();
  WriteCompactU32(size);

  base::sorted_for_each(
      keyframes.begin(), keyframes.end(), [this](const auto& itr) {
        if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                    FEATURE_CSS_VALUE_VERSION) &&
            compile_options_.enable_css_parser_) {
          WriteCompactD64(CSSKeyframesToken::ParseKeyStr(
              itr.first, compile_options_.enable_css_strict_mode_));
        } else {
          EncodeUtf8Str(itr.first.c_str());
        }
        EncodeCSSAttributes(*itr.second);
      });
  return true;
}

LepusDebugInfo TemplateBinaryWriter::GetDebugInfo() const {
  LepusDebugInfo info;

  if (IsLepusNGContext()) {
    info.debug_info_.source_code = quick_context()->GetDebugSourceCode();
    info.debug_info_.top_level_function =
        quick_context()->GetTopLevelFunction();
  } else {
    info.lepus_funcs_ = GetContextFunc();
  }

  return info;
}

const std::vector<lynx::fml::RefPtr<lynx::lepus::Function>>&
TemplateBinaryWriter::GetContextFunc() const {
  return func_vec;
}

void TemplateBinaryWriter::EncodeCustomSectionRoute(
    const CustomSectionHeaders& route) {
  WriteU32(route.size());
  std::for_each(route.begin(), route.end(), [this](const auto& header) {
    WriteStringDirectly(header.first.c_str());
    EncodeValue(&header.second.header, false);
    WriteU32(header.second.range.start);
    WriteU32(header.second.range.end);
  });
}

bool TemplateBinaryWriter::IsDir(const char* path) {
  struct stat buf;
  if (lstat(path, &buf) < 0) {
    return false;
  }
  if (S_ISDIR(buf.st_mode)) {
    return true;
  }
  return false;
}

void TemplateBinaryWriter::EncodeLepusChunkSection() {
  if (lepus_chunk_code_.empty()) {
    return;
  }
  TemplateSectionRecorder recorder(
      BinarySection::LEPUS_CHUNK, BinaryOffsetType::TYPE_LEPUS_CHUNK, this,
      stream_.get(), binary_info_, offset_map_, section_size_info_);
  LepusChunkRoute route;
  uint32_t descriptor_offset = stream()->size();
  uint32_t start = 0;
  uint32_t end = 0;
  base::sorted_for_each(
      lepus_chunk_code_.begin(), lepus_chunk_code_.end(),
      [descriptor_offset, &route, &start, &end, this](const auto& it) {
        std::string path = it.first;
        auto& chunk = it.second;

        auto error = lepus::BytecodeGenerator::GenerateBytecode(
            mts_context(), chunk, compile_options_.target_sdk_version_, path);

        if (!error.empty()) {
          throw lepus::CompileException(error.c_str());
        }

        if (IsLepusNGContext()) {
          auto debug_info = GetDebugInfo();
          lepus_debug_info_.AddDebugInfo(path, debug_info, quick_context());
        }

        ContextBinaryWriter::encode();
        end = stream()->size() - descriptor_offset;
        route.lepus_chunk_ranges.insert({path, LepusChunkRange(start, end)});
        start = end;
      });
  start = stream()->size();
  EncodeLepusChunkRoute(route);
  end = stream()->size();
  if (!lepus_chunk_code_.empty()) {
    stream_->Move(descriptor_offset, start, end - start);
  }
}

void TemplateBinaryWriter::EncodeLepusChunkRoute(
    const LepusChunkRoute& lepus_chunk_route) {
  WriteCompactU32(lepus_chunk_route.lepus_chunk_ranges.size());
  base::sorted_for_each(lepus_chunk_route.lepus_chunk_ranges.begin(),
                        lepus_chunk_route.lepus_chunk_ranges.end(),
                        [this](const auto& it) {
                          WriteStringDirectly(it.first.c_str());
                          WriteCompactU32(it.second.start);
                          WriteCompactU32(it.second.end);
                        });
}

void TemplateBinaryWriter::EncodeElementTemplateSection() {
  if (element_template_ == nullptr || !element_template_->IsObject() ||
      element_template_->GetObject().MemberCount() == 0) {
    return;
  }

  TemplateSectionRecorder recorder(BinarySection::NEW_ELEMENT_TEMPLATE,
                                   BinaryOffsetType::TYPE_NEW_ELEMENT_TEMPLATE,
                                   this, stream_.get(), binary_info_,
                                   offset_map_, section_size_info_);

  EncodeTemplatesToBinary(element_template_);
}

int TemplateBinaryWriter::FindJSFileInDirectory(
    const char* path, const char* relationPath,
    std::unordered_map<std::string, std::string>& js_map) {
  const int MAX_PATH_LEN = 4097;
  DIR* db;
  char filename[MAX_PATH_LEN];
  struct dirent* p;
  db = opendir(path);
  if (db == NULL) return 0;
  memset(filename, 0, sizeof(filename));
  while ((p = readdir(db))) {
    if ((strcmp(p->d_name, ".") == 0) || (strcmp(p->d_name, "..") == 0))
      continue;
    else {
      snprintf(filename, sizeof(filename), "%s/%s", path, p->d_name);
      if (IsDir(filename)) {
        char newRelation[MAX_PATH_LEN];
        snprintf(newRelation, sizeof(newRelation), "%s%s/", relationPath,
                 p->d_name);
        FindJSFileInDirectory(filename, newRelation, js_map);
      } else {
        char newRelation[MAX_PATH_LEN];
        snprintf(newRelation, sizeof(newRelation), "%s%s", relationPath,
                 p->d_name);
        char newFilePath[MAX_PATH_LEN];
        snprintf(newFilePath, sizeof(newRelation), "%s/%s", path, p->d_name);

        std::string name(p->d_name);
        if (name.substr(name.length() - 2) == "js") {
          js_map[newRelation] = newFilePath;
        }
      }
    }
    memset(filename, 0, sizeof(filename));
  }
  closedir(db);
  return 0;
}

void TemplateBinaryWriter::EncodeParsedStylesSection() {
  if (element_template_parsed_styles_ == nullptr ||
      !element_template_parsed_styles_->IsObject()) {
    return;
  }

  TemplateSectionRecorder recorder(
      BinarySection::PARSED_STYLES, BinaryOffsetType::TYPE_PARSED_STYLES, this,
      stream_.get(), binary_info_, offset_map_, section_size_info_);

  EncodeParsedStylesToBinary(element_template_parsed_styles_);
}

void TemplateBinaryWriter::EncodeAirParsedStylesRoute(
    const AirParsedStylesRoute& route) {
  WriteU32(route.parsed_styles_ranges_.size());
  base::sorted_for_each(
      route.parsed_styles_ranges_.begin(), route.parsed_styles_ranges_.end(),
      [this](const auto& it) {
        WriteStringDirectly(it.first.c_str());
        WriteU32(it.second.size());
        base::sorted_for_each(
            it.second.begin(), it.second.end(),
            [this](const auto& pair) {
              WriteStringDirectly(pair.first.c_str());
              WriteU32(pair.second.start);
              WriteU32(pair.second.end);
            },
            [](const auto& left, const auto& right) {
              return left.first < right.first;
            });
      },
      [](const auto& left, const auto& right) {
        return left.first < right.first;
      });
}

void TemplateBinaryWriter::EncodeAirParsedStyles() {
  if (air_styles_ == nullptr || !air_styles_->IsObject()) {
    return;
  }

  TemplateSectionRecorder recorder(
      BinarySection::PARSED_STYLES, BinaryOffsetType::TYPE_PARSED_STYLES, this,
      stream_.get(), binary_info_, offset_map_, section_size_info_);
  constexpr static const char* kRawCSS = "raw_css_ids";
  const auto& value = lepus::jsonValueTolepusValue(*air_styles_);
  const auto& style_table = value.Table();
  std::set<CSSPropertyID> css_raw_ids;
  if (style_table->Contains(kRawCSS)) {
    const auto& raw_css_ids = style_table->GetValue(kRawCSS);
    if (raw_css_ids.IsArray()) {
      const auto& array = raw_css_ids.Array();
      for (size_t i = 0; i < array->size(); ++i) {
        const auto& id = array->get(i);
        if (id.IsNumber()) {
          css_raw_ids.insert(static_cast<CSSPropertyID>(id.Number()));
        }
      }
    }
    style_table->Erase(kRawCSS);
  }

  AirParsedStylesRoute route;
  uint32_t descriptor_offset = stream()->size();
  uint32_t start = 0;
  uint32_t end = 0;

  base::sorted_for_each(
      style_table->begin(), style_table->end(),
      [this, &descriptor_offset, &route, &start, &end,
       &css_raw_ids](const auto& it) {
        const auto& v = it.second;
        if (!v.IsObject()) {
          return;
        }
        std::unordered_map<std::string, AirParsedStylesRange>
            single_comp_ranges;
        base::sorted_for_each(
            v.Table()->begin(), v.Table()->end(),
            [this, &descriptor_offset, &start, &end, &css_raw_ids,
             &single_comp_ranges](const auto& pair) {
              const auto& vv = pair.second;
              if (!vv.IsObject()) {
                return;
              }
              const auto& table = vv.Table();
              std::map<CSSPropertyID, lepus::Value> raw_map;
              StyleMap map;
              for (const auto& pair : *table) {
                CSSPropertyID property_id =
                    CSSProperty::GetPropertyID(pair.first.str());
                if (compile_options_.enable_air_raw_css_ ||
                    css_raw_ids.find(property_id) != css_raw_ids.end()) {
                  raw_map.insert({property_id, pair.second});
                } else {
                  StyleMap output;
                  UnitHandler::Process(
                      property_id, pair.second, output,
                      CSSParserConfigs::GetCSSParserConfigsByComplierOptions(
                          compile_options_));
                  for (const auto& p : output) {
                    map.insert_or_assign(p.first, p.second);
                  }
                }
              }
              WriteU32(raw_map.size());
              for (const auto& p : raw_map) {
                WriteU32(p.first);
                EncodeValue(&p.second);
              }
              WriteU32(map.size());
              for (const auto& p : map) {
                WriteU32(p.first);
                EncodeCSSValue(p.second, true, true);
              }
              end = stream()->size() - descriptor_offset;
              single_comp_ranges.insert(
                  {pair.first.str(), AirParsedStylesRange(start, end)});
              start = end;
            },
            [](const auto& left, const auto& right) {
              return left.first.str() < right.first.str();
            });
        route.parsed_styles_ranges_.insert(
            {it.first.str(), single_comp_ranges});
      },
      [](const auto& left, const auto& right) {
        return left.first.str() < right.first.str();
      });

  start = stream()->size();
  EncodeAirParsedStylesRoute(route);
  end = stream()->size();
  stream_->Move(descriptor_offset, start, end - start);
}

void TemplateBinaryWriter::SerializeJSSource() {
  TemplateSectionRecorder recorder(BinarySection::JS, BinaryOffsetType::TYPE_JS,
                                   this, stream_.get(), binary_info_,
                                   offset_map_, section_size_info_);

  WriteU32(js_code_.size());
  if (!silence_) {
    printf("start encode JSSource......\n");
  }

  base::SortedForEach(
      js_code_,
      [this](const auto& it) {
        EncodeUtf8Str(it.first.c_str());
        if (!silence_) {
          printf("         %s\n", m.name.GetString());
        }
        EncodeUtf8Str(it.second.c_str());
      },
      [](const auto& a, const auto& b) { return a.first < b.first; });
  if (!silence_) {
    printf("end encode JSSource......\n");
  }
}

void TemplateBinaryWriter::EncodeJsBytecode() {
  TemplateSectionRecorder recorder(
      BinarySection::JS_BYTECODE, BinaryOffsetType::TYPE_JS_BYTECODE, this,
      stream_.get(), binary_info_, offset_map_, section_size_info_);
  WriteU32(static_cast<unsigned>(runtime::js::JSRuntimeType::quickjs));
  WriteU32(js_code_.size());
  if (!silence_) {
    printf("start to encode JS Bytecode......\n");
  }

  base::SortedForEach(
      js_code_,
      [this](const std::pair<std::string, std::string>& it) {
        const std::string& file_name = it.first;
        const std::string& file_content = it.second;
        EncodeUtf8Str(file_name.c_str());
        if (!silence_) {
          printf("         %s\n", m.name.GetString());
        }

        auto src_buffer =
            std::make_shared<runtime::js::StringBuffer>(file_content);
        auto provider_src =
            runtime::js::quickjs::QuickjsBytecodeProvider::FromSource(
                file_name, src_buffer);
        bool is_debug_info_out = tasm::Config::IsHigherOrEqual(
            compile_options_.target_sdk_version_.c_str(), LYNX_VERSION_2_14);
        if (is_debug_info_out) {
          if (auto& info = provider_src.GenerateDebugInfo(); info.context_) {
            SetLynxTargetSdkVersion(
                info.context_, compile_options_.target_sdk_version_.c_str());
            SetDebugInfoOutside(info.context_, true);
            info.source_ = file_content;
          }
        }

        auto provider = provider_src.Compile(
            base::Version(compile_options_.target_sdk_version_),
            {.strip_debug_info = !is_debug_info_out});

        if (!provider) {
          throw lepus::CompileException(
              (file_name + " compilation error!").c_str());
        }
        auto bin_buffer = provider->GetPackedBytecodeBuffer();
        if (!bin_buffer) {
          throw lepus::CompileException(
              (file_name + " compilation error!").c_str());
        }

        WriteCompactU64(static_cast<uint64_t>(bin_buffer->size()));
        WriteData(bin_buffer->data(), bin_buffer->size(), "quick bytecode");
        if (is_debug_info_out) {
          js_debug_info_.insert(
              {file_name, provider_src.GetDebugInfoProvider()});
        }
      },
      [](const auto& a, const auto& b) { return a.first < b.first; });
  if (!silence_) {
    printf("end encode JS Bytecode......\n");
  }
}

}  // namespace tasm
}  // namespace lynx
