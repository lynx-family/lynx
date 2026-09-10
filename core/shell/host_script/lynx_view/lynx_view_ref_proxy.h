// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REF_PROXY_H_
#define CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REF_PROXY_H_

#include <string>

#include "core/shell/host_script/lynx_view/lynx_view_ref_types.h"

namespace lynx {
namespace shell {

// A non-owning, platform-specific proxy for a platform-created LynxView.
// Operations return whether the request was accepted, never page completion.
// Implementations copy request values before returning, recheck the target on
// execution, and report asynchronous failures/lifecycle through session Notify.
class LynxViewRefProxy {
 public:
  virtual ~LynxViewRefProxy() = default;

  virtual bool LoadTemplate(LynxViewRefLoadTemplateRequest request) = 0;
  virtual bool LoadSSR(LynxViewRefSsrRequest request) = 0;
  virtual bool HydrateSSR(LynxViewRefSsrRequest request) = 0;
  virtual bool UpdateMetaData(LynxViewRefUpdateMetaDataRequest request) = 0;
  virtual bool ReloadTemplate(LynxViewRefReloadTemplateRequest request) = 0;
  virtual bool SendGlobalEvent(LynxViewRefGlobalEventRequest request) = 0;

  virtual bool SetGlobalProps(std::string global_props_json) = 0;

  virtual void Invalidate() = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REF_PROXY_H_
