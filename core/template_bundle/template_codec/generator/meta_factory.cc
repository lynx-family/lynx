// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/generator/meta_factory.h"

#include "base/include/sorted_for_each.h"
#include "core/base/json/json_util.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/template_bundle/template_codec/generator/source_generator.h"
#include "third_party/aes/aes.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

namespace {
constexpr const char* kTargetSdkVersion = "targetSdkVersion";
constexpr const char* kSourceContext = "sourceContent";
constexpr const char* kEncodeBundleModuleMode = "bundleModuleMode";
constexpr const char* kManifest = "manifest";
constexpr const char* kSilence = "silence";
constexpr const char* kSkipEncode = "skipEncode";
constexpr const char* kEnableTTForFullVersion = "enableTTForFullVersion";
constexpr const char* kEnableRadon = "enableRadon";
constexpr const char* kEnableDataSetAttrs = "enableDataSetAttrs";
constexpr const char* kEnableCSSExternalClass = "enableCSSExternalClass";
constexpr const char* kEnableCSSClassMerge = "enableCSSClassMerge";
constexpr const char* kDisableMultipleCascadeCSS = "disableMultipleCascadeCSS";
constexpr const char* kEnableRemoveCSSScope = "enableRemoveCSSScope";
constexpr const char* kEnableCSSLazyDecode = "enableCSSLazyDecode";
constexpr const char* kEnableCSSAsyncDecode = "enableCSSAsyncDecode";
constexpr const char* kEnableCSSSelector = "enableCSSSelector";
constexpr const char* kEnableCSSInvalidation = "enableCSSInvalidation";
constexpr const char* kEncodeQuickjsBytecode =
    "experimental_encodeQuickjsBytecode";
constexpr const char* kEnableEventRefactor = "enableEventRefactor";
constexpr const char* kRemoveCSSParserLog = "removeCSSParserLog";
constexpr const char* kEnableDynamicComponent = "enableDynamicComponent";
constexpr const char* kEnableLepusDebug = "enableLepusDebug";
constexpr const char* kDefaultOverflowVisible = "defaultOverflowVisible";
constexpr const char* kDefaultDisplayLinear = "defaultDisplayLinear";
constexpr const char* kEnableCSSVariable = "enableCSSVariable";
constexpr const char* kEnableKeepPageData = "enableKeepPageData";
constexpr const char* kEnableCSSStrictMode = "enableCSSStrictMode";
constexpr const char* kImplicitAnimation = "implicitAnimation";
constexpr const char* kEnableLynxAir = "enableLynxAir";
constexpr const char* kEnableCSSEngine = "enableCSSEngine";
constexpr const char* kEnableComponentConfig = "enableComponentConfig";
constexpr const char* kEnableFiberArch = "enableFiberArch";
constexpr const char* kUseLepusNG = "useLepusNG";
constexpr const char* kClosureFix = "closureFix";
constexpr const char* kWorkletMap = "workletMap";
constexpr const char* kPackScript = "packScript";
constexpr const char* kScripts = "scripts";
constexpr const char* kTemplateDebugUrl = "templateDebugUrl";
constexpr const char* kLepusCode = "lepusCode";
constexpr const char* kRoot = "root";
constexpr const char* kLepusChunk = "lepusChunk";
constexpr const char* kEnableSSR = "enableSSR";
constexpr const char* kEnableCursor = "enableCursor";
constexpr const char* kForceCalcNewStyle = "forceCalcNewStyle";
constexpr const char* kLynxAirMode = "lynxAirMode";
constexpr const char* kDebugInfoOutside = "debugInfoOutside";
constexpr const char* kTrialOptions = "trialOptions";
constexpr const char* kTemplateInfo = "templateInfo";
constexpr const char* kCompilerOptions = "compilerOptions";
constexpr const char* kCss = "css";
constexpr const char* kCssMap = "cssMap";
constexpr const char* kCssSource = "cssSource";
constexpr const char* kParsedStyle = "parsedStyle";
constexpr const char* kConfig = "config";
constexpr const char* kEnableFlexibleTemplate = "enableFlexibleTemplate";
constexpr const char* kEnableReuseContext = "enableReuseContext";
constexpr const char* kElementTemplate = "elementTemplate";
constexpr const char* kAirRawCSS = "enableAirRawCSS";
constexpr const char* kAirStyles = "stylesMap";
constexpr const char* kEnableConcurrentElement = "enableConcurrentElement";
constexpr const char* kCustomSections = "customSections";
constexpr const char* kEnableLepusChunkAsyncDecode =
    "enableLepusChunkAsyncDecode";

#define GET_VALUE_FROM_JSON(Doc, Key, Type, Var)   \
  if (Doc.HasMember(Key) && Doc[Key].Is##Type()) { \
    Var = Doc[Key].Get##Type();                    \
  }

#define GET_UNLESS(expr)                 \
  (expr);                                \
  if (!encoder_options.parser_result_) { \
    return encoder_options;              \
  }

}  // namespace

void MetaFactory::GetSourceContent(rapidjson::Value& document,
                                   EncoderOptions& encoder_options) {
  // Get source content raw value
  const rapidjson::Value& source_content_value = document[kSourceContext];

  if (source_content_value.IsString()) {
    // If source_content_value is string, parser it.
    encoder_options.generator_options_.source_content_str_ =
        source_content_value.GetString();
    if (encoder_options.generator_options_.source_content_obj_
            .Parse(encoder_options.generator_options_.source_content_str_)
            .HasParseError()) {
      std::stringstream ss;
      ss << "error: source is not valid json file! msg:"
         << encoder_options.generator_options_.source_content_obj_
                .GetParseErrorMsg()
         << ", position: "
         << encoder_options.generator_options_.source_content_obj_
                .GetErrorOffset();
      encoder_options.err_msg_ = ss.str();
      encoder_options.parser_result_ = false;
      return;
    }
  } else if (source_content_value.IsObject()) {
    // If source_content_value is object, dump json string to
    // encoder_options.generator_options_.source_content_str_.
    encoder_options.generator_options_.source_content_obj_.CopyFrom(
        source_content_value,
        encoder_options.generator_options_.source_content_obj_.GetAllocator());
    encoder_options.generator_options_.source_content_str_ =
        base::ToJson(encoder_options.generator_options_.source_content_obj_);
  } else {
    std::stringstream ss;
    ss << "error: source content is not a string nor an object.";
    encoder_options.err_msg_ = ss.str();
    encoder_options.parser_result_ = false;
    return;
  }

  // Get dsl type
  GET_VALUE_FROM_JSON(encoder_options.generator_options_.source_content_obj_,
                      TEMPLATE_BUNDLE_APP_DSL, String,
                      encoder_options.generator_options_.dsl_)

  // Set PackageInstanceDSL according to dsl type
  encoder_options.generator_options_.instance_dsl_ =
      GetDSLType(encoder_options.generator_options_.dsl_.c_str());

  // Get app type
  rapid_value::MemberIterator it_app_type =
      encoder_options.generator_options_.source_content_obj_.FindMember(
          TEMPLATE_BUNDLE_APP_TYPE);
  if (!it_app_type->value.IsString()) {
    encoder_options.err_msg_ = "app type is undefined!";
    encoder_options.parser_result_ = false;
    return;
  }
  encoder_options.generator_options_.app_type_ = it_app_type->value.GetString();
  if (encoder_options.generator_options_.app_type_.empty()) {
    encoder_options.err_msg_ = "app type is empty!";
    encoder_options.parser_result_ = false;
    return;
  }

  // Set PackageInstanceType according to app type
  encoder_options.generator_options_.instance_type_ =
      encoder_options.generator_options_.app_type_ == APP_TYPE_CARD
          ? PackageInstanceType::CARD
          : PackageInstanceType::DYNAMIC_COMPONENT;

  // Get cli version
  // FIXME(zhangye): split encode_inner function, add rapidjson related unittest
  rapid_value::MemberIterator it_version =
      encoder_options.generator_options_.source_content_obj_.FindMember(
          TEMPLATE_SUPPORTED_VERSIONS);
  encoder_options.generator_options_.cli_version_ = "unknown";
  if (it_version !=
      encoder_options.generator_options_.source_content_obj_.MemberEnd()) {
    if (it_version->value.IsObject()) {
      auto it_version_object = it_version->value.GetObject();
      rapid_value::MemberIterator it_cli_version =
          it_version_object.FindMember(TEMPLATE_CLI_VERSION);
      if (it_cli_version != it_version_object.MemberEnd()) {
        if (it_cli_version->value.IsString()) {
          encoder_options.generator_options_.cli_version_ =
              it_cli_version->value.GetString();
        }
      }
    }
  }

  // Set lepus version
  encoder_options.generator_options_.lepus_version_ = ENGINE_VERSION;

  // Set styles in air
  encoder_options.generator_options_.air_parsed_styles_ =
      encoder_options.generator_options_.source_content_obj_[kAirStyles];

  return;
}

void MetaFactory::GetAndCheckTargetSdkVersion(
    rapidjson::Value& compiler_options, EncoderOptions& encoder_options) {
  // Get engineVersion and check engineVersion
  GET_VALUE_FROM_JSON(compiler_options, kTargetSdkVersion, String,
                      encoder_options.compile_options_.target_sdk_version_)
  if (!Config::IsHigherOrEqual(
          LYNX_TASM_MAX_SUPPORTED_VERSION,
          base::Version(
              encoder_options.compile_options_.target_sdk_version_)) ||
      !Config::IsHigherOrEqual(
          encoder_options.compile_options_.target_sdk_version_,
          MIN_SUPPORTED_LYNX_VERSION)) {
    std::stringstream ss;
    ss << "error: current compiler only support lynx version ["
       << MIN_SUPPORTED_LYNX_VERSION << ", " << LYNX_TASM_MAX_SUPPORTED_VERSION
       << "], but engine version is "
       << encoder_options.compile_options_.target_sdk_version_;
    encoder_options.err_msg_ = ss.str();
    encoder_options.parser_result_ = false;
    return;
  }
}

void MetaFactory::GetCSSMeta(rapidjson::Value& document,
                             EncoderOptions& encoder_options) {
  // get css object
  if (encoder_options.compile_options_.enable_fiber_arch_) {
    auto css = document[kCss].GetObject();
    auto& css_map = css[kCssMap];
    auto& css_source = css[kCssSource];
    auto& parsed_styles = css[kParsedStyle];
    encoder_options.generator_options_.css_ = css;
    encoder_options.generator_options_.css_map_ = css_map;
    encoder_options.generator_options_.css_source_ = css_source;
    encoder_options.generator_options_.parsed_styles_ = parsed_styles;
  } else {
    encoder_options.generator_options_.css_obj_.Parse("{}");
    for (rapid_value::MemberIterator itr =
             encoder_options.generator_options_.source_content_obj_
                 .MemberBegin();
         itr !=
         encoder_options.generator_options_.source_content_obj_.MemberEnd();
         ++itr) {
      const auto& name = std::string(itr->name.GetString());
      if (name.find(TEMPLATE_BUNDLE_TTSS) != std::string::npos) {
        char* c = strcpy(new char[name.length() + 1], name.c_str());
        encoder_options.generator_options_.css_obj_.AddMember(
            rapidjson::StringRef(c), itr->value,
            encoder_options.generator_options_.source_content_obj_
                .GetAllocator());
      }
    }
  }
}

void MetaFactory::GetTrialOptions(rapidjson::Document& document,
                                  EncoderOptions& encoder_options) {
  // Get Trial Options
  rapidjson::Value::ConstMemberIterator trial_options_objects =
      document.FindMember(kTrialOptions);
  if (trial_options_objects != document.MemberEnd()) {
    fml::RefPtr<lepus::Dictionary> trial_options = lepus::Dictionary::Create();
    for (const auto& feature : trial_options_objects->value.GetObject()) {
      lepus_value feature_info = lepus::jsonValueTolepusValue(
          AES().DecryptECB(feature.value.GetString()).c_str());
      trial_options->SetValue(feature.name.GetString(), feature_info);
    }
    encoder_options.generator_options_.trial_options_ =
        lepus::Value(trial_options);
  }

  bool enable_trial_options = false;
  if (Config::IsHigherOrEqual(
          encoder_options.compile_options_.target_sdk_version_,
          FEATURE_TRIAL_OPTIONS_VERSION) &&
      !encoder_options.generator_options_.trial_options_.IsNil()) {
    enable_trial_options = true;
  }
  encoder_options.compile_options_.enable_trial_options_ = enable_trial_options;
}

void MetaFactory::GetTemplateInfo(rapidjson::Document& document,
                                  EncoderOptions& encoder_options) {
  // Get template info, see issue: #7974
  const auto& template_info_json = document[kTemplateInfo];
  if (template_info_json.IsObject()) {
    encoder_options.generator_options_.template_info_ =
        lynx::lepus::jsonValueTolepusValue(template_info_json);
  }
}

void MetaFactory::GetLepusCode(rapidjson::Document& document,
                               EncoderOptions& encoder_options) {
  // Get lepus code
  if (!document.HasMember(kLepusCode)) {
    return;
  }
  auto& lepus_code = document[kLepusCode];
  if (lepus_code.IsString()) {
    encoder_options.generator_options_.lepus_code_ = lepus_code.GetString();
  } else if (lepus_code.IsObject()) {
    auto lepus_code_obj = lepus_code.GetObject();
    if (lepus_code_obj.HasMember(kRoot) && lepus_code_obj[kRoot].IsString()) {
      encoder_options.generator_options_.lepus_code_ =
          lepus_code_obj[kRoot].GetString();
    }
    // Get lepus chunk code
    if (lepus_code_obj.HasMember(kLepusChunk) &&
        lepus_code_obj[kLepusChunk].IsObject()) {
      for (auto& pair : lepus_code_obj[kLepusChunk].GetObject()) {
        if (!pair.value.IsString()) {
          continue;
        }
        encoder_options.generator_options_
            .lepus_chunk_code_[pair.name.GetString()] = pair.value.GetString();
      }
    }
  }
}

void MetaFactory::GetJSCode(rapidjson::Document& document,
                            EncoderOptions& encoder_options) {
  // Get JS Code
  const auto& raw_manifest = document[kManifest];
  if (raw_manifest.IsString()) {
    rapidjson::Document d;
    d.Parse(raw_manifest.GetString());
    for (const auto& m : d.GetObject()) {
      encoder_options.generator_options_.js_code_[m.name.GetString()] =
          m.value.GetString();
    }
  } else if (raw_manifest.IsObject()) {
    for (const auto& m : raw_manifest.GetObject()) {
      encoder_options.generator_options_.js_code_[m.name.GetString()] =
          m.value.GetString();
    }
  }
}

void MetaFactory::GetConfig(EncoderOptions& encoder_options) {
  if (encoder_options.generator_options_.source_content_obj_.HasMember(
          kConfig)) {
    auto& json = encoder_options.generator_options_.source_content_obj_;
    auto& allocator =
        encoder_options.generator_options_.source_content_obj_.GetAllocator();
    rapid_value& card_config = json[kConfig];

    // add dsl
    rapid_value config_temp(card_config, allocator);
    rapidjson::Value dsl(
        static_cast<int32_t>(encoder_options.generator_options_.instance_dsl_));
    config_temp.AddMember(TEMPLATE_BUNDLE_APP_DSL, dsl, allocator);

    // add bundle_module_mode_
    rapidjson::Value bundle_module_mode(static_cast<int32_t>(
        encoder_options.generator_options_.bundle_module_mode_));
    config_temp.AddMember(TEMPLATE_BUNDLE_MODULE_MODE, bundle_module_mode,
                          allocator);

    // add cli version
    std::string cli_version = "unknown";
    if (json.HasMember(TEMPLATE_SUPPORTED_VERSIONS) &&
        json[TEMPLATE_SUPPORTED_VERSIONS].HasMember(TEMPLATE_CLI_VERSION) &&
        json[TEMPLATE_SUPPORTED_VERSIONS][TEMPLATE_CLI_VERSION].IsString()) {
      cli_version =
          json[TEMPLATE_SUPPORTED_VERSIONS][TEMPLATE_CLI_VERSION].GetString();
    }
    config_temp.AddMember(TEMPLATE_CLI_VERSION, cli_version, allocator);

    // add trial options
    rapidjson::Document trial;
    if (encoder_options.generator_options_.trial_options_.IsTable()) {
      if (!trial
               .Parse(lepusValueToJSONString(
                   encoder_options.generator_options_.trial_options_, true))
               .HasParseError()) {
        config_temp.AddMember(rapidjson::StringRef(kTrialOptions), trial,
                              allocator);
      }
    }

    // let config be json string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    config_temp.Accept(writer);
    encoder_options.generator_options_.config_ = buffer.GetString();
  }
}

void MetaFactory::GetTemplateScript(rapidjson::Document& document,
                                    EncoderOptions& encoder_options) {
  // Get worklet_, deprecated now
  if (document.HasMember(kWorkletMap) && document[kWorkletMap].IsObject()) {
    encoder_options.generator_options_.worklet_ = document[kWorkletMap];
  }

  // Get packed_script_ and script_map_ for template script.
  if (document.HasMember(kPackScript) && document[kPackScript].IsString() &&
      document.HasMember(kScripts) && document[kScripts].IsObject()) {
    encoder_options.generator_options_.packed_script_ = document[kPackScript];
    encoder_options.generator_options_.script_map_ = document[kScripts];
  }
}

void MetaFactory::GetElementTemplate(rapidjson::Document& document,
                                     EncoderOptions& encoder_options) {
  // Get element template raw value
  const rapidjson::Value& element_template_value = document[kElementTemplate];

  // If element_template_value is string, parser it.
  if (element_template_value.IsString()) {
    if (encoder_options.generator_options_.element_template_
            .Parse(element_template_value.GetString())
            .HasParseError()) {
      std::stringstream ss;
      ss << "error: element template is not valid json file! msg:"
         << encoder_options.generator_options_.element_template_
                .GetParseErrorMsg()
         << ", position: "
         << encoder_options.generator_options_.element_template_
                .GetErrorOffset();
      encoder_options.err_msg_ = ss.str();
      encoder_options.parser_result_ = false;
      return;
    }
  } else {
    // copy element_template_value to
    // encoder_options.generator_options_.element_template_.
    encoder_options.generator_options_.element_template_.CopyFrom(
        element_template_value,
        encoder_options.generator_options_.element_template_.GetAllocator());
  }
}

EncoderOptions MetaFactory::GetEncoderOptions(rapidjson::Document& document) {
  EncoderOptions encoder_options{};
  // If document has kCompilerOptions and it is a object, let options =
  // document[kCompilerOptions]. And get compilerOptions info from options.
  rapidjson::Value& options = document.HasMember(kCompilerOptions) &&
                                      document[kCompilerOptions].IsObject()
                                  ? document[kCompilerOptions]
                                  : document;

  GET_UNLESS(GetAndCheckTargetSdkVersion(options, encoder_options));

  GET_UNLESS(GetSourceContent(document, encoder_options));

  GET_UNLESS(GetElementTemplate(document, encoder_options));

  GET_UNLESS(GetTrialOptions(document, encoder_options));

  GET_UNLESS(GetTemplateInfo(document, encoder_options));

  GET_UNLESS(GetJSCode(document, encoder_options));

  GET_UNLESS(GetLepusCode(document, encoder_options));

  GET_UNLESS(GetTemplateScript(document, encoder_options));

  GET_UNLESS(GetCustomSections(document, encoder_options));

  // Get silence
  GET_VALUE_FROM_JSON(options, kSilence, Bool,
                      encoder_options.generator_options_.silence_)

  // Get bundleModuleMode
  const char* bundleModuleModeInString = "";
  GET_VALUE_FROM_JSON(options, kEncodeBundleModuleMode, String,
                      bundleModuleModeInString)
  bool isBundleModuleReturnByFunction = !std::strcmp(
      bundleModuleModeInString, BUNDLE_MODULE_MODE_RETURN_BY_FUNCTION);
  encoder_options.generator_options_.bundle_module_mode_ =
      isBundleModuleReturnByFunction
          ? PackageInstanceBundleModuleMode::RETURN_BY_FUNCTION_MODE
          : PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE;

  // Get enableTTForFullVersion
  GET_VALUE_FROM_JSON(
      options, kEnableTTForFullVersion, Bool,
      encoder_options.generator_options_.enable_tt_for_full_version_)
  // Get enableDataSetAttrs
  GET_VALUE_FROM_JSON(options, kEnableDataSetAttrs, Bool,
                      encoder_options.generator_options_.enable_dataset_attrs_)
  // TODO(zhangkaijie.9): remove all code related to radon_mode_ and
  // enable_radon_
  // Get enableRadon
  GET_VALUE_FROM_JSON(options, kEnableRadon, Bool,
                      encoder_options.generator_options_.enable_radon_)
  // Get enableDebugInfo
  GET_VALUE_FROM_JSON(options, kEnableLepusDebug, Bool,
                      encoder_options.generator_options_.enable_debug_info_)
  // Get skipEncode
  GET_VALUE_FROM_JSON(options, kSkipEncode, Bool,
                      encoder_options.generator_options_.skip_encode_)
  // Get enableSSR
  GET_VALUE_FROM_JSON(options, kEnableSSR, Bool,
                      encoder_options.generator_options_.enable_ssr_)
  // Get enableCursor
  GET_VALUE_FROM_JSON(options, kEnableCursor, Bool,
                      encoder_options.generator_options_.enable_cursor_)

  const char* template_debug_url = "";
  GET_VALUE_FROM_JSON(options, kTemplateDebugUrl, String, template_debug_url);

  bool enableCSSExternalClass = true;
  GET_VALUE_FROM_JSON(options, kEnableCSSExternalClass, Bool,
                      enableCSSExternalClass)

  bool enableCSSClassMerge = false;
  GET_VALUE_FROM_JSON(options, kEnableCSSClassMerge, Bool, enableCSSClassMerge)

  bool enableRemoveCSSScope = false;
  GET_VALUE_FROM_JSON(options, kEnableRemoveCSSScope, Bool,
                      enableRemoveCSSScope)

  bool disableMultipleCascadeCSS = Config::IsHigherOrEqual(
      encoder_options.compile_options_.target_sdk_version_, LYNX_VERSION_2_0);
  GET_VALUE_FROM_JSON(options, kDisableMultipleCascadeCSS, Bool,
                      disableMultipleCascadeCSS)

  bool enableDynamicComponent = true;
  GET_VALUE_FROM_JSON(options, kEnableDynamicComponent, Bool,
                      enableDynamicComponent)

  bool defaultOverflowVisible = true;
  GET_VALUE_FROM_JSON(options, kDefaultOverflowVisible, Bool,
                      defaultOverflowVisible)

  bool enableCSSVariable = true;
  GET_VALUE_FROM_JSON(options, kEnableCSSVariable, Bool, enableCSSVariable)

  bool defaultImplicitAnimation = true;
  GET_VALUE_FROM_JSON(options, kImplicitAnimation, Bool,
                      defaultImplicitAnimation)

  bool enableKeepPageData = false;
  // When engineVersion >= 2.3, default value is true otherwise it is false.
  if (Config::IsHigherOrEqual(
          encoder_options.compile_options_.target_sdk_version_,
          LYNX_VERSION_2_3)) {
    enableKeepPageData = true;
  }
  GET_VALUE_FROM_JSON(options, kEnableKeepPageData, Bool, enableKeepPageData)

  bool defaultDisplayLinear = true;
  GET_VALUE_FROM_JSON(options, kDefaultDisplayLinear, Bool,
                      defaultDisplayLinear)

  bool enable_lynx_air = false;
  GET_VALUE_FROM_JSON(options, kEnableLynxAir, Bool, enable_lynx_air)

  uint8_t lynx_air_mode = AIR_MODE_OFF;
  GET_VALUE_FROM_JSON(options, kLynxAirMode, Uint, lynx_air_mode)

  bool enable_air_raw_css = true;
  GET_VALUE_FROM_JSON(options, kAirRawCSS, Bool, enable_air_raw_css);

  bool enable_css_engine = true;
  GET_VALUE_FROM_JSON(options, kEnableCSSEngine, Bool, enable_css_engine)

  bool enable_reuse_context = false;
  GET_VALUE_FROM_JSON(options, kEnableReuseContext, Bool, enable_reuse_context)

  bool enable_css_invalidation = false;
  GET_VALUE_FROM_JSON(options, kEnableCSSInvalidation, Bool,
                      enable_css_invalidation)

  // This encode option is managed by minSdkVersion in speedy.
  // In the experimental stage(`experimental_encodeQuickjsBytecode`), When this
  // option is enabled, minSdkVersion must be greater than 2.11; otherwise the
  // compilation will stop. The version restriction will alter after
  // experimental stage is finished.
  bool encode_quickjs_bytecode = false;
  GET_VALUE_FROM_JSON(options, kEncodeQuickjsBytecode, Bool,
                      encode_quickjs_bytecode)

  bool enable_async_lepus_chunk = false;
  GET_VALUE_FROM_JSON(options, kEnableLepusChunkAsyncDecode, Bool,
                      enable_async_lepus_chunk)

  FeOption enableCSSLazyDecode = FE_OPTION_UNDEFINED;
  if (options.HasMember(kEnableCSSLazyDecode)) {
    bool enable_css_lazy_decode = false;
    GET_VALUE_FROM_JSON(options, kEnableCSSLazyDecode, Bool,
                        enable_css_lazy_decode)
    enableCSSLazyDecode =
        enable_css_lazy_decode ? FE_OPTION_ENABLE : FE_OPTION_DISABLE;
  }

  FeOption enableCSSAsyncDecode = FE_OPTION_UNDEFINED;
  if (options.HasMember(kEnableCSSAsyncDecode)) {
    bool enable_css_async_decode = false;
    GET_VALUE_FROM_JSON(options, kEnableCSSAsyncDecode, Bool,
                        enable_css_async_decode)
    enableCSSAsyncDecode =
        enable_css_async_decode ? FE_OPTION_ENABLE : FE_OPTION_DISABLE;
  }

  FeOption event_refactor = FE_OPTION_UNDEFINED;
  if (options.HasMember(kEnableEventRefactor)) {
    bool enable_event_refactor = false;
    GET_VALUE_FROM_JSON(options, kEnableEventRefactor, Bool,
                        enable_event_refactor)
    event_refactor =
        enable_event_refactor ? FE_OPTION_ENABLE : FE_OPTION_DISABLE;
  }

  FeOption forceCalcNewStyle = FE_OPTION_UNDEFINED;
  if (options.HasMember(kForceCalcNewStyle)) {
    bool force_calc_new_style = true;
    GET_VALUE_FROM_JSON(options, kForceCalcNewStyle, Bool, force_calc_new_style)
    forceCalcNewStyle =
        force_calc_new_style ? FE_OPTION_ENABLE : FE_OPTION_DISABLE;
  }

  bool enable_component_config = false;
  GET_VALUE_FROM_JSON(options, kEnableComponentConfig, Bool,
                      enable_component_config);

  bool enable_fiber_arch = false;
  GET_VALUE_FROM_JSON(options, kEnableFiberArch, Bool, enable_fiber_arch);
  if (enable_fiber_arch &&
      !Config::IsHigherOrEqual(
          encoder_options.compile_options_.target_sdk_version_,
          FEATURE_FIBER_ARCH)) {
    std::stringstream ss;
    ss << "error: can't set " << kEnableFiberArch << " when engineVersion < "
       << FEATURE_FIBER_ARCH;
    encoder_options.err_msg_ = ss.str();
    encoder_options.parser_result_ = false;
    return encoder_options;
  }

  // Enable CSS selector when fiber arch is enabled
  bool enable_css_selector =
      enable_fiber_arch &&
      Config::IsHigherOrEqual(
          encoder_options.compile_options_.target_sdk_version_,
          LYNX_VERSION_2_10);
  GET_VALUE_FROM_JSON(options, kEnableCSSSelector, Bool, enable_css_selector)

  bool enable_css_strict_mode = false;
  GET_VALUE_FROM_JSON(options, kEnableCSSStrictMode, Bool,
                      enable_css_strict_mode)

  bool remove_css_parser_log = false;
  GET_VALUE_FROM_JSON(options, kRemoveCSSParserLog, Bool, remove_css_parser_log)

  // Get debuginfo_outside, use_lepus_ng, and lepus_closure_fix
  bool debuginfo_outside = false;
  bool use_lepusng = false;
  GET_VALUE_FROM_JSON(options, kUseLepusNG, Bool, use_lepusng)
  GET_VALUE_FROM_JSON(options, kDebugInfoOutside, Bool, debuginfo_outside);

  bool lepus_closure_fix = Config::IsHigherOrEqual(
      encoder_options.compile_options_.target_sdk_version_, LYNX_VERSION_1_6);
  GET_VALUE_FROM_JSON(options, kClosureFix, Bool, lepus_closure_fix)

  // Determine arch info, default is RADON_ARCH. If enable_fiber_arch, it is
  // FIBER_ARCH. If lynx_air_mode == AIR_MODE_STRICT, it is AIR_ARCH. If
  // enable_fiber_arch and lynx_air_mode != AIR_MODE_OFF, throw error.
  ArchOption arch = ArchOption::RADON_ARCH;
  if (enable_fiber_arch && lynx_air_mode != AIR_MODE_OFF &&
      lynx_air_mode != AIR_MODE_FIBER) {
    std::stringstream ss;
    ss << "error: can't enable fiber arch and air arch at the same time.";
    encoder_options.err_msg_ = ss.str();
    encoder_options.parser_result_ = false;
    return encoder_options;
  } else if (enable_fiber_arch) {
    arch = ArchOption::FIBER_ARCH;
  } else if (lynx_air_mode == AIR_MODE_STRICT) {
    arch = ArchOption::AIR_ARCH;
  }

  // Determine if enable flexible template. For radon arch, the default value is
  // false, and it can only be enabled when engineVersion >=
  // FEATURE_FLEXIBLE_TEMPLATE, otherwise an error will be reported. For fiber
  // arch and air arch, this value must be true and cannot be set to false.
  bool enable_flexible_template = false;
  if (arch == RADON_ARCH) {
    if (options.HasMember(kEnableFlexibleTemplate) &&
        !Config::IsHigherOrEqual(
            encoder_options.compile_options_.target_sdk_version_,
            FEATURE_FLEXIBLE_TEMPLATE)) {
      std::stringstream ss;
      ss << "error: can't set " << kEnableFlexibleTemplate
         << " when engineVersion < " << FEATURE_FLEXIBLE_TEMPLATE;
      encoder_options.err_msg_ = ss.str();
      encoder_options.parser_result_ = false;
      return encoder_options;
    }
    // For radon arch, if engine version >= FEATURE_FLEXIBLE_TEMPLATE, read
    // kEnableFlexibleTemplate and set value to enable_flexible_template. And
    // the default is false.
    GET_VALUE_FROM_JSON(options, kEnableFlexibleTemplate, Bool,
                        enable_flexible_template)
  } else if (arch == FIBER_ARCH || arch == AIR_ARCH) {
    // For fiber arch and air arch, enable_flexible_template must be true.
    enable_flexible_template = true;
  }

  bool enable_css_parser = false;
  bool enable_parallel_element = false;

  // The current online independent gray is using the enableConcurrentElement
  // config. After the independent gray verification is completed, the online
  // template will use the enableParallelElement config. At that time, the
  // related logic of enableConcurrentElement will also be deleted.
  if (arch == FIBER_ARCH && (options.HasMember(kEnableConcurrentElement) ||
                             options.HasMember(kEnableParallelElement))) {
    GET_VALUE_FROM_JSON(options, kEnableParallelElement, Bool,
                        enable_parallel_element);

    if (!enable_parallel_element) {
      GET_VALUE_FROM_JSON(options, kEnableConcurrentElement, Bool,
                          enable_parallel_element);
    }

    // If the element parallel flush is enabled, the css parser will be
    // enabled by default.
    if (enable_parallel_element) {
      enable_css_parser = true;
      if (!encoder_options.generator_options_.template_info_.IsObject()) {
        encoder_options.generator_options_.template_info_ =
            lepus::Value::CreateObject();
      }

      encoder_options.generator_options_.template_info_.SetProperty(
          kEnableConcurrentElement, lepus::Value(enable_parallel_element));

      encoder_options.generator_options_.template_info_.SetProperty(
          kEnableParallelElement, lepus::Value(enable_parallel_element));
    }
  }

  encoder_options.compile_options_.lepusng_debuginfo_outside_ =
      debuginfo_outside;
  encoder_options.generator_options_.lepus_closure_fix_ = lepus_closure_fix;

  CompileOptions compile_options{
      encoder_options.compile_options_.target_sdk_version_,
      std::string(template_debug_url),
      enable_css_parser,
      enableCSSExternalClass,
      use_lepusng,
      enableCSSClassMerge,
      enableRemoveCSSScope,
      disableMultipleCascadeCSS,
      remove_css_parser_log,
      encoder_options.compile_options_.enable_trial_options_,
      encoder_options.compile_options_.lepusng_debuginfo_outside_,
      enableDynamicComponent,
      enable_css_strict_mode,
      defaultOverflowVisible,
      enableCSSVariable,
      defaultImplicitAnimation,
      encoder_options.generator_options_.enable_radon_ ? RADON_MODE_RADON
                                                       : RADON_MODE_DOM_DIFF,
      encoder_options.generator_options_.dsl_ == DSL_TYPE_TT
          ? FRON_END_DSL_MINI_APP
          : FRON_END_DSL_REACT,
      enableKeepPageData,
      defaultDisplayLinear,
      enable_lynx_air,
      enableCSSLazyDecode,
      event_refactor,
      forceCalcNewStyle,
      enableCSSAsyncDecode,
      enable_css_engine,
      enable_component_config,
      lynx_air_mode,
      CONFIG_TYPE_EXPERIMENT_SETTINGS,
      enable_fiber_arch,
      enable_flexible_template,
      arch,
      enable_css_selector,
      enable_reuse_context,
      enable_css_invalidation,
      enable_air_raw_css,
      encode_quickjs_bytecode,
      enable_async_lepus_chunk};

  // Set compile_options_
  encoder_options.compile_options_ = compile_options;

  SourceGeneratorOptions source_generator_options{
      encoder_options.generator_options_.enable_tt_for_full_version_, false,
      encoder_options.generator_options_.enable_dataset_attrs_};

  // Set source_generator_options_
  encoder_options.source_generator_options_ = source_generator_options;

  // Get CSS Meta
  GET_UNLESS(GetCSSMeta(document, encoder_options));

  // Get Config
  GET_UNLESS(GetConfig(encoder_options));

  return encoder_options;
}

void MetaFactory::GetCustomSections(rapidjson::Document& document,
                                    EncoderOptions& encoder_options) {
  auto& custom_sections_json = document[kCustomSections];
  if (custom_sections_json.IsObject()) {
    encoder_options.generator_options_.custom_sections_ = custom_sections_json;
  }
}

}  // namespace tasm
}  // namespace lynx
