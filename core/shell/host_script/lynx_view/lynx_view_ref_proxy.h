// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REF_PROXY_H_
#define CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REF_PROXY_H_

#include "core/shell/host_script/lynx_view/lynx_view_ref_types.h"

namespace lynx {
namespace shell {

// A non-owning, platform-specific proxy for a platform-created LynxView.
// Implementations must copy any platform values they need before returning
// and invoke each completion at most once.
class LynxViewRefProxy {
 public:
  virtual ~LynxViewRefProxy() = default;

  virtual void LoadTemplate(LynxViewRefLoadTemplateRequest request,
                            LynxViewRefOperationCompletion completion) = 0;
  virtual void LoadSSR(LynxViewRefSsrRequest request,
                       LynxViewRefOperationCompletion completion) = 0;
  virtual void HydrateSSR(LynxViewRefSsrRequest request,
                          LynxViewRefOperationCompletion completion) = 0;
  virtual void UpdateData(LynxViewRefUpdateDataRequest request,
                          LynxViewRefOperationCompletion completion) = 0;
  virtual void ReloadTemplate(LynxViewRefReloadTemplateRequest request,
                              LynxViewRefOperationCompletion completion) = 0;
  virtual void SendGlobalEvent(LynxViewRefGlobalEventRequest request,
                               LynxViewRefOperationCompletion completion) = 0;

  virtual void Invalidate() = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REF_PROXY_H_
