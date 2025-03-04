// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_TTML_CONSTANT_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_TTML_CONSTANT_H_

namespace lynx {
namespace tasm {
// tasm
static constexpr const char TEMPLATE_BUNDLE_NAME[] = "path";
static constexpr const char TEMPLATE_BUNDLE_PAGES[] = "pages";
static constexpr const char TEMPLATE_BUNDLE_DYNAMIC_COMPONENTS[] =
    "dynamic_components";
static constexpr const char TEMPLATE_BUNDLE_TTSS[] = ".ttss";
static constexpr const char TEMPLATE_BUNDLE_DATA[] = "data";
static constexpr const char TEMPLATE_BUNDLE_APP_TYPE[] = "appType";
static constexpr const char TEMPLATE_BUNDLE_APP_DSL[] = "dsl";
static constexpr const char TEMPLATE_SUPPORTED_VERSIONS[] = "__version";
static constexpr const char TEMPLATE_CLI_VERSION[] = "cli";
static constexpr const char TEMPLATE_AUTO_EXPOSE[] = "autoExpose";
static constexpr const char TEMPLATE_BUNDLE_MODULE_MODE[] = "bundleModuleMode";

static constexpr const char REACT_PRE_PROCESS_LIFECYCLE[] =
    "getDerivedStateFromProps";
static constexpr const char REACT_ERROR_PROCESS_LIFECYCLE[] =
    "getDerivedStateFromError";
static constexpr const char REACT_SHOULD_COMPONENT_UPDATE[] =
    "shouldComponentUpdate";

static constexpr const char REACT_SHOULD_COMPONENT_UPDATE_KEY[] =
    "$$shouldComponentUpdate";
static constexpr const char REACT_NATIVE_STATE_VERSION_KEY[] =
    "$$nativeStateVersion";
static constexpr const char REACT_JS_STATE_VERSION_KEY[] = "$$jsStateVersion";
static constexpr const char REACT_RENDER_ERROR_KEY[] = "$$renderError";
static constexpr const char JS_RENDER_ERROR[] = "JS_RENDER_ERROR";
static constexpr const char LEPUS_RENDER_ERROR[] = "LEPUS_RENDER_ERROR";

static constexpr const char SCREEN_METRICS_OVERRIDER[] =
    "getScreenMetricsOverride";

static constexpr const char* APP_TYPE_CARD = "card";
static constexpr const char* PAGE_ID = "card";
static constexpr const char* APP_TYPE_DYNAMIC_COMPONENT = "DynamicComponent";

static constexpr const char* DSL_TYPE_TT = "tt";
static constexpr const char* DSL_TYPE_REACT = "react";
static constexpr const char* DSL_TYPE_REACT_NODIFF = "react_nodiff";
static constexpr const char* DSL_TYPE_STANDALONE = "standalone";
static constexpr const char* DSL_TYPE_REACT_UNKOWN = "unkown";

enum class PackageInstanceDSL {
  TT,
  REACT,
  REACT_NODIFF,
  STANDALONE,
  UNKOWN = 100
};
inline const char* GetDSLName(PackageInstanceDSL dsl) {
  switch (dsl) {
    case PackageInstanceDSL::TT:
      return DSL_TYPE_TT;
    case PackageInstanceDSL::REACT:
      return DSL_TYPE_REACT;
    case PackageInstanceDSL::REACT_NODIFF:
      return DSL_TYPE_REACT_NODIFF;
    case PackageInstanceDSL::STANDALONE:
      return DSL_TYPE_STANDALONE;
    default:
      return DSL_TYPE_REACT_UNKOWN;
  }
}

inline PackageInstanceDSL GetDSLType(const char* dsl) {
  if (strcmp(DSL_TYPE_TT, dsl) == 0) {
    return PackageInstanceDSL::TT;
  }
  if (strcmp(DSL_TYPE_REACT, dsl) == 0) {
    return PackageInstanceDSL::REACT;
  }
  if (strcmp(DSL_TYPE_REACT_NODIFF, dsl) == 0) {
    return PackageInstanceDSL::REACT_NODIFF;
  }
  return PackageInstanceDSL::UNKOWN;
}

static constexpr const char* BUNDLE_MODULE_MODE_RETURN_BY_FUNCTION =
    "ReturnByFunction";

enum class PackageInstanceBundleModuleMode {
  EVAL_REQUIRE_MODE = 0,       // default
  RETURN_BY_FUNCTION_MODE = 1  // enable flag on lynx_core bundler
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_TTML_CONSTANT_H_
