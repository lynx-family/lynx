// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_base_template_reader.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

#if ENABLE_AIR
#include "core/renderer/dom/air/lynx_air_parsed_style_store.h"
#endif

namespace lynx {
namespace tasm {

bool LynxBinaryBaseTemplateReader::Decode() {
  decode_start_timestamp_ = base::CurrentSystemTimeMicroseconds();
  // Decode header
  ERROR_UNLESS(DecodeHeader());

  // Perform some check or set method after decode header.
  ERROR_UNLESS(DidDecodeHeader());

  // Decode app type.
  ERROR_UNLESS(ReadStringDirectly(&app_type_));

  // Perform some check or set method after decode app type.
  ERROR_UNLESS(DidDecodeAppType());

  // Decode snapshot, useless now.
  DECODE_BOOL(snapshot);

  // Decode template's all sections.
  ERROR_UNLESS(DecodeTemplateBody());

  // Perform some check or set method after decode template.
  ERROR_UNLESS(DidDecodeTemplate());

  decode_end_timestamp_ = base::CurrentSystemTimeMicroseconds();
  // If all above functions do not return false, then return true.
  return true;
}

// Decode Header Section
bool LynxBinaryBaseTemplateReader::DecodeHeader() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "DecodeHeader");
  DECODE_U32(total_size);
  if (total_size != stream_->size()) {
    std::ostringstream msg;
    msg << "template file has broken. Expected size is " << total_size
        << ". Actual size is " << stream_->size();
    error_message_ = msg.str();
    return false;
  }
  total_size_ = total_size;

  DECODE_U32(magic_word);
  if (magic_word == template_codec::kQuickBinaryMagic) {
    is_lepusng_binary_ = true;
  } else if (magic_word == template_codec::kLepusBinaryMagic) {
    is_lepusng_binary_ = false;
#if ENABLE_JUST_LEPUSNG
    error_message_ =
        "Support lepusNG only. Template file uses lepus. Please add "
        "`useLepusNG: true` in encode section.";
    return false;
#endif
  } else {
    return false;
  }

  // lepus_version is deprecated
  // now it is just used to be compatible with prev version
  std::string lepus_version;  // deprecated
  std::string error;
  ERROR_UNLESS(ReadStringDirectly(&lepus_version));
  ERROR_UNLESS_CODE(SupportedLepusVersion(lepus_version, error), error_message_,
                    error);

  std::string target_sdk_version;
  if (lepus_version > MIN_SUPPORTED_VERSION) {
    // cli_version is deprecated
    // now ios_version == android_version == target_cli_version
    std::string cli_version;  // deprecated
    std::string ios_version;
    std::string android_version;

    ERROR_UNLESS(ReadStringDirectly(&cli_version));  // deprecated
    ERROR_UNLESS(ReadStringDirectly(&ios_version));  // deprecated
    ERROR_UNLESS(ReadStringDirectly(&android_version));
    // currently android_version == ios_version, is the engien version in
    // project config
    if (ios_version != "unknown") {
      if (!CheckLynxVersion(ios_version)) {
        error_message_ =
            "version check miss, should "
            "(lynx sdk version >= App Bundle's engine version >= min supported "
            "version), but "
            "engine version: " +
            ios_version +
            ", lynx sdk version: " + Config::GetCurrentLynxVersion() +
            ", min supported lynx version: " +
            Config::GetMinSupportLynxVersion() + "; ";
        return false;
      } else {
        LOGI("App Bundle's engine version: "
             << ios_version
             << ", lynx sdk version:" << Config::GetCurrentLynxVersion()
             << ", min supported lynx version: "
             << Config::GetMinSupportLynxVersion());
      }
    } else {
      LOGI("engine version is unknown! jump LynxVersion check\n");
    }
    target_sdk_version = ios_version;
  }
  // Decode compiler options
  if (Config::IsHigherOrEqual(target_sdk_version,
                              FEATURE_HEADER_EXT_INFO_VERSION)) {
    ERROR_UNLESS(DecodeHeaderInfo(compile_options_));
  } else {
    compile_options_.target_sdk_version_ = target_sdk_version;
  }

  // Decode template info
  if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_TEMPLATE_INFO)) {
    ERROR_UNLESS(DecodeValue(&template_info_, true));
  }

  // Decode trial options
  if (compile_options_.enable_trial_options_) {
    // To keep compatible with old version, we should decode trial options but
    // never use it
    lepus::Value trial_options;
    ERROR_UNLESS(DecodeValue(&trial_options, true));
  }
  if (compile_options_.enable_css_class_merge_) {
    if (compile_options_.enable_css_class_merge_) {
      report::FeatureCounter::Instance()->Count(
          report::LynxFeature::CPP_ENABLE_CLASS_MERGE);
    }
  }
  enable_css_font_face_extension_ = Config::IsHigherOrEqual(
      compile_options_.target_sdk_version_, FEATURE_CSS_FONT_FACE_EXTENSION);
  enable_css_variable_ = EnableCssVariable(compile_options_);
  enable_css_variable_multi_default_value_ =
      EnableCssVariableMultiDefaultValue(compile_options_);
  enable_css_parser_ = EnableCssParser(compile_options_);
  return true;
}

bool LynxBinaryBaseTemplateReader::SupportedLepusVersion(
    const std::string &binary_version, std::string &error) {
  static std::string client_version = Config::GetVersion();
  static std::string min_supported_version = Config::GetMinSupportedVersion();
  static std::string max_need_console_version = Config::GetNeedConsoleVersion();

  auto vec_binary = VersionStrToNumber(binary_version);
  static auto vec_client = VersionStrToNumber(client_version);
  static auto vec_min_supported_version =
      VersionStrToNumber(min_supported_version);

  LOGI("client version:" << client_version
                         << "  ;binary_version:" << binary_version);

  // Store lepus version
  lepus_version_ = vec_binary;

  bool has_error = false;

  if (vec_client.size() < LEPUS_VERSION_COUNT ||
      vec_binary.size() < LEPUS_VERSION_COUNT) {
    has_error = true;
  }

  if (!has_error) {
    // check is binary version > client version
    for (size_t i = 0; i < std::min(vec_client.size(), vec_binary.size());
         i++) {
      if (vec_client[i] > vec_binary[i]) {
        break;
      }

      if (vec_client[i] < vec_binary[i]) {
        has_error = true;
        break;
      }
    }
  }

  if (!has_error) {
    // check is binary version > min supported version
    for (size_t i = 0;
         i < std::min(vec_min_supported_version.size(), vec_binary.size());
         i++) {
      if (vec_binary[i] > vec_min_supported_version[i]) {
        break;
      }

      if (vec_binary[i] < vec_min_supported_version[i]) {
        has_error = true;
        break;
      }
    }
  }

  if (has_error) {
    error = "unsupported binary version:";
    error += binary_version;
    error += " ; client version:";
    error += client_version;
    error += " ; min supported version:";
    error += min_supported_version;
  }

  // check if this binary need 'console' in js runtime global
  auto vec_max_need_console_version =
      VersionStrToNumber(max_need_console_version);
  for (size_t i = 0;
       i < std::min(vec_max_need_console_version.size(), vec_binary.size());
       i++) {
    if (vec_binary[i] > vec_max_need_console_version[i]) {
      support_component_js_ = true;
      break;
    }
  }
  return !has_error;
}

bool LynxBinaryBaseTemplateReader::CheckLynxVersion(
    const std::string &binary_version) {
  base::Version client_version(Config::GetCurrentLynxVersion());
  base::Version min_supported_lynx_version(Config::GetMinSupportLynxVersion());
  base::Version binary_lynx_version(binary_version);

  // binary_lynx_version should in this range:
  // min_supported_lynx_version  <= binary_lynx_version <= client_version
  if (binary_lynx_version < min_supported_lynx_version ||
      client_version < binary_lynx_version) {
    return false;
  }

  return true;
}

VersionComponentArray LynxBinaryBaseTemplateReader::VersionStrToNumber(
    const std::string &version_str) {
  VersionComponentArray version_vec;
  size_t pre_pos = 0;
  for (int i = 0; i < LEPUS_VERSION_COUNT - 1; i++) {
    size_t pos = version_str.find('.', pre_pos);
    if (pos == std::string::npos) {
      break;
    }
    std::string section = version_str.substr(pre_pos, pos);
    version_vec.push_back(atoi(section.c_str()));
    pre_pos = pos + 1;
  }
  size_t pos = version_str.find('-');
  if (pos != std::string::npos) {
    std::string section = version_str.substr(pre_pos, pos);
    version_vec.push_back(atoi(section.c_str()));
  } else {
    std::string section = version_str.substr(pre_pos);
    version_vec.push_back(atoi(section.c_str()));
  }
  return version_vec;
}

template <typename T>
void LynxBinaryBaseTemplateReader::ReinterpretValue(
    T &tgt, const HeaderExtInfoByteArray &src) {
  if (src.size() == sizeof(T)) {
    tgt = *reinterpret_cast<const T *>(src.data());
  }
}

template <>
void LynxBinaryBaseTemplateReader::ReinterpretValue(
    std::string &tgt, const HeaderExtInfoByteArray &src) {
  tgt = std::string(reinterpret_cast<const char *>(src.data()), src.size());
}

bool LynxBinaryBaseTemplateReader::DecodeHeaderInfo(
    CompileOptions &compile_options) {
  auto curr_offset = stream_->offset();
  memset(&header_ext_info_, 0, sizeof(header_ext_info_));
  ERROR_UNLESS(stream_->ReadData(reinterpret_cast<uint8_t *>(&header_ext_info_),
                                 sizeof(header_ext_info_)));

  DCHECK(header_ext_info_.header_ext_info_magic_ == HEADER_EXT_INFO_MAGIC);
  for (uint32_t i = 0; i < header_ext_info_.header_ext_info_field_numbers_;
       i++) {
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

  // space for forward compatible

  stream_->Seek(curr_offset + header_ext_info_.header_ext_info_size);

  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeHeaderInfoField() {
  HeaderExtInfo::HeaderExtInfoField header_info_field;
  auto size_to_read = sizeof(header_info_field) - sizeof(void *);
  ERROR_UNLESS(stream_->ReadData(
      reinterpret_cast<uint8_t *>(&header_info_field), (int)size_to_read));

  DCHECK(header_info_map_.find(header_info_field.key_id_) ==
         header_info_map_.end());
  auto &payload = header_info_map_[header_info_field.key_id_];
  payload.resize<false>(header_info_field.payload_size_);
  ERROR_UNLESS(stream_->ReadData(reinterpret_cast<uint8_t *>(payload.data()),
                                 header_info_field.payload_size_));
  return true;
}

bool LynxBinaryBaseTemplateReader::DidDecodeAppType() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DidDecodeAppType");
  if (!app_type_check_) {
    // if app_type_ is not set, skip
    return true;
  }

  AppType excepted_type = *app_type_check_;
  AppType actual_type = app_type_ != APP_TYPE_DYNAMIC_COMPONENT
                            ? AppType::kCard
                            : AppType::kDynamicComponent;

  if (excepted_type != actual_type) {
    error_message_ = "App type mismatch, excepted type:";
    error_message_.append(excepted_type == AppType::kCard
                              ? APP_TYPE_CARD
                              : APP_TYPE_DYNAMIC_COMPONENT);
    error_message_.append(", actual type:");
    error_message_.append(app_type_);
    error_message_.append(", please check the dsl of your project.");
    return false;
  }
  return true;
}

// Decode Template body
bool LynxBinaryBaseTemplateReader::DecodeTemplateBody() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeTemplateBody");

  if (compile_options_.enable_flexible_template_) {
    ERROR_UNLESS(DecodeFlexibleTemplateBody());
    return true;
  }
  ERROR_UNLESS(DeserializeSection());
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeFlexibleTemplateBody() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeFlexibleTemplateBody");

  ERROR_UNLESS(DecodeSectionRoute());

  const static std::vector<BinarySection> kFiberSectionOrder{
      BinarySection::STRING,
      BinarySection::PARSED_STYLES,
      BinarySection::ELEMENT_TEMPLATE,
      BinarySection::CSS,
      BinarySection::JS,
      BinarySection::JS_BYTECODE,
      BinarySection::CONFIG,
      BinarySection::ROOT_LEPUS,
      BinarySection::LEPUS_CHUNK,
      BinarySection::CUSTOM_SECTIONS,
      BinarySection::NEW_ELEMENT_TEMPLATE,
  };

  const static std::vector<BinarySection> kSectionOrder{
      BinarySection::STRING,
      BinarySection::PARSED_STYLES,
      BinarySection::CSS,
      BinarySection::JS,
      BinarySection::JS_BYTECODE,
      BinarySection::COMPONENT,
      BinarySection::APP,
      BinarySection::PAGE,
      BinarySection::CONFIG,
      BinarySection::DYNAMIC_COMPONENT,
      BinarySection::USING_DYNAMIC_COMPONENT_INFO,
      BinarySection::THEMED,
      BinarySection::CUSTOM_SECTIONS,
  };

  const auto &order =
      compile_options_.enable_fiber_arch_ ? kFiberSectionOrder : kSectionOrder;

  for (const auto &s : order) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FindSpecificSection");

    auto iter = section_route_.find(s);
    if (iter == section_route_.end()) {
      continue;
    }

    const auto &route = iter->second;
    stream_->Seek(route.start_offset_);

    DECODE_U8(type);
    ERROR_UNLESS(DecodeSpecificSection(static_cast<BinarySection>(type)));
  }
  return true;
}

// For Section Route
bool LynxBinaryBaseTemplateReader::DecodeSectionRoute() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeSectionRoute");

  // section route type
  DECODE_U8(section_route_type);
  DECODE_COMPACT_U32(section_count);

  for (uint32_t i = 0; i < section_count; ++i) {
    DECODE_U8(section);
    DECODE_COMPACT_U32(start);
    DECODE_COMPACT_U32(end);
    section_route_.insert({static_cast<BinarySection>(section),
                           {static_cast<BinarySection>(section), start, end}});
  }

  uint32_t start = static_cast<uint32_t>(stream_->offset());
  for (auto &pair : section_route_) {
    pair.second.start_offset_ += start;
    pair.second.end_offset_ += start;
  }
  return true;
}

bool LynxBinaryBaseTemplateReader::DeserializeSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DeserializeSection");

  DECODE_U8(section_count);
  for (size_t i = 0; i < section_count; i++) {
    DECODE_U8(type);
    ERROR_UNLESS(DecodeSpecificSection(static_cast<BinarySection>(type)));
  }  // end for

  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeSpecificSection(
    const BinarySection &section) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeSpecificSection");
  switch (section) {
    case BinarySection::CSS: {
      ERROR_UNLESS(DecodeCSSDescriptor());
      break;
    }
    case BinarySection::APP: {
      ERROR_UNLESS(DecodeAppDescriptor());
      break;
    }
    case BinarySection::PAGE: {
      ERROR_UNLESS(DecodePageDescriptor());
      break;
    }
    case BinarySection::STRING: {
      ERROR_UNLESS(DeserializeStringSection());
      break;
    }
    case BinarySection::COMPONENT: {
      ERROR_UNLESS(DecodeComponentDescriptor());
      break;
    }
    case BinarySection::JS: {
      ERROR_UNLESS(DeserializeJSSourceSection());
      break;
    }
    case BinarySection::JS_BYTECODE: {
      ERROR_UNLESS(DeserializeJSBytecodeSection());
      break;
    }
    case BinarySection::CONFIG: {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodePageConfig");
      page_config_offset_ = stream_->offset();
      DECODE_STDSTR(config_str);
      EnsurePageConfig();
      ERROR_UNLESS(
          config_decoder_->DecodePageConfig(config_str, page_configs_));
      break;
    }
    case BinarySection::DYNAMIC_COMPONENT: {
      ERROR_UNLESS(DecodeDynamicComponentDescriptor());
      break;
    }
    case BinarySection::THEMED: {
      ERROR_UNLESS(DecodeThemedSection());
      break;
    }
    case BinarySection::USING_DYNAMIC_COMPONENT_INFO: {
      ERROR_UNLESS(DecodeDynamicComponentDeclarations());
      break;
    }
    case BinarySection::ROOT_LEPUS: {
      ERROR_UNLESS(DecodeContext());
      break;
    }
    case BinarySection::LEPUS_CHUNK: {
      ERROR_UNLESS(DecodeLepusChunk());
      break;
    }
    case BinarySection::ELEMENT_TEMPLATE: {
      error_message_ =
          "The legacy element template is no longer supported. Please upgrade "
          "to the latest version of speedy to use the new element template.";
      LOGE(error_message_);
      return false;
    }
    case BinarySection::PARSED_STYLES: {
      if (compile_options_.arch_option_ == ArchOption::FIBER_ARCH) {
        ERROR_UNLESS(DecodeParsedStylesSection());
      } else if (compile_options_.arch_option_ == ArchOption::AIR_ARCH) {
        ERROR_UNLESS(DecodeAirParsedStylesSection());
      }
      break;
    }
    case BinarySection::CUSTOM_SECTIONS: {
      ERROR_UNLESS(DecodeCustomSectionsSection());
      break;
    }
    case BinarySection::NEW_ELEMENT_TEMPLATE: {
      ERROR_UNLESS(DecodeElementTemplateSection());
      break;
    }
    default:
      LOGE("unkown - section:");
      return false;
  }
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeAppDescriptor() { return true; }

bool LynxBinaryBaseTemplateReader::DecodePageDescriptor() { return true; }

bool LynxBinaryBaseTemplateReader::DecodePageMould(PageMould *mould) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodePageRoute(PageRoute &route) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DeserializeVirtualNodeSection() {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeComponentDescriptor() { return true; }

bool LynxBinaryBaseTemplateReader::DecodeComponentRoute(ComponentRoute &route) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeComponentMould(ComponentMould *mould,
                                                        int offset,
                                                        int length) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DeserializeJSSourceSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DeserializeJSSourceSection");
  DECODE_U32(count);
  for (size_t i = 0; i < count; i++) {
    DECODE_STDSTR(path);
    DECODE_STDSTR(content);
    js_bundle_.AddJsContent(
        path, {std::move(content), piper::JsContent::Type::SOURCE});
  }
  return true;
}

bool LynxBinaryBaseTemplateReader::DeserializeJSBytecodeSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DeserializeJSBytecodeSection");
  DECODE_U32(engine);
  ERROR_UNLESS(engine == static_cast<uint32_t>(piper::JSRuntimeType::quickjs));
  DECODE_U32(count);
  for (size_t i = 0; i < count; i++) {
    DECODE_STR(path);
    DECODE_COMPACT_U64(data_len);
    std::string content;
    content.resize(static_cast<std::size_t>(data_len));
    ERROR_UNLESS(ReadData(reinterpret_cast<unsigned char *>(content.data()),
                          static_cast<int>(data_len)));
    js_bundle_.AddJsContent(
        path.str(), {std::move(content), piper::JsContent::Type::BYTECODE});
  }
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeDynamicComponentDescriptor() {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeDynamicComponentDeclarations() {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeDynamicComponentRoute(
    DynamicComponentRoute &route) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeDynamicComponentMould(
    DynamicComponentMould *mould) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeThemedSection() { return true; }

bool LynxBinaryBaseTemplateReader::DecodeAirParsedStylesSection() {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeAirParsedStylesInner(
    StyleMap &style_map) {
  return true;
}

void LynxBinaryBaseTemplateReader::EnsurePageConfig() {
  if (page_configs_ == nullptr) {
    page_configs_ = std::make_shared<PageConfig>();
  }
}
}  // namespace tasm
}  // namespace lynx
