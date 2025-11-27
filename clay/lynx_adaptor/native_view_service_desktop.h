// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_VIEW_SERVICE_DESKTOP_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_VIEW_SERVICE_DESKTOP_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "clay/lynx_adaptor/native_platform_view.h"
#include "clay/ui/platform/native_view_service.h"
#include "platform/embedder/lynx_view_builder_priv.h"

namespace clay {
class ViewContext;

class NativeViewServiceDesktop final : public NativeViewService {
 public:
  std::unique_ptr<NativeViewPlugin> CreateNativeViewPlugin(
      int id, NativeView* view_ptr) override;

  std::unordered_map<std::string, NativePlatformView::Creator> view_factories;

  static void SetViewFactories(
      ViewContext* view_context,
      std::unordered_map<std::string, std::function<NativePlatformView*()>>
          factories);
  static void SetViewFactories(
      ViewContext* view_context,
      std::unordered_map<std::string,
                         std::pair<lynx_native_view_creator, void*>>
          creators);
  static void AddViewFactory(ViewContext* view_context, const char* name,
                             lynx_native_view_creator creator, void* opaque);
};

}  // namespace clay

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_VIEW_SERVICE_DESKTOP_H_
