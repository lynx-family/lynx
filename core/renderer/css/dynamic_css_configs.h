// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_DYNAMIC_CSS_CONFIGS_H_
#define CORE_RENDERER_CSS_DYNAMIC_CSS_CONFIGS_H_

#include <unordered_set>

#include "core/renderer/css/css_property.h"
#include "core/renderer/tasm/config.h"

namespace lynx {
namespace tasm {

struct DynamicCSSConfigs {
  static tasm::DynamicCSSConfigs& GetDefaultDynamicCSSConfigs() {
    static base::NoDestructor<tasm::DynamicCSSConfigs> kDefaultCSSConfigs;
    return *kDefaultCSSConfigs;
  }

  // TODO(wangzhixuan.0821): Remove the following setting when once inheritance
  // is proved to be stable.
  bool OnceInheritanceDisabled() const {
    // cache the setting.
    // static bool disable = tasm::LynxEnv::GetInstance().GetBoolEnv(
    //     tasm::LynxEnv::Key::DISABLE_ONCE_DYNAMIC_CSS, false);
    // return disable;
    // TODO(zhixuan): Enable it after bugs are fixed.
    return once_inheritance_disabled_;
  }

  bool enable_css_inheritance_ = false;
  std::unordered_set<CSSPropertyID> custom_inherit_list_;
  // Hack to keep the old behavior that vw is resolved against screen metrics
  // only for font size if viweport size is specified as definite value.
  bool unify_vw_vh_behavior_ = false;
  bool font_scale_sp_only = false;
  bool once_inheritance_disabled_{true};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_DYNAMIC_CSS_CONFIGS_H_
