// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/registry/markdown_registry.h"

#include <dlfcn.h>

#include <mutex>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

constexpr const char* kMarkdownSoName = "liblynx_xelement_markdown.so";
constexpr const char* kMarkdownUICreatorSymbol = "UILynxMarkdownCreateFunc";
constexpr const char* kMarkdownShadowCreatorSymbol =
    "UILynxMarkdownShadowNodeCreateFunc";

LynxContext::UICreatorFunc g_ui_creator = nullptr;
LynxContext::LayoutNodeCreatorFuc g_shadow_creator = nullptr;
void* g_markdown_handle = nullptr;

template <typename T>
std::pair<T, const char*> TryResolveSymbol(void* handle, const char* symbol) {
  dlerror();
  void* sym = dlsym(handle, symbol);
  const char* err = dlerror();
  if (err) {
    return {nullptr, err};
  }
  return {reinterpret_cast<T>(sym), nullptr};
}

void LogDlsymFailure(const char* symbol, const char* err) {
  if (err) {
    LOGE("XElement markdown dlsym " << symbol << " failed: " << err);
  } else {
    LOGE("XElement markdown dlsym " << symbol
                                    << " failed: resolved to nullptr");
  }
}

void ResolveMarkdownSymbols() {
  LOGI("XElement markdown resolve symbols from RTLD_DEFAULT");
  {
    auto [func, err] = TryResolveSymbol<LynxContext::UICreatorFunc>(
        RTLD_DEFAULT, kMarkdownUICreatorSymbol);
    if (func && !err) {
      g_ui_creator = func;
      LOGI("XElement markdown UI creator resolved from RTLD_DEFAULT");
    }
  }
  {
    auto [func, err] = TryResolveSymbol<LynxContext::LayoutNodeCreatorFuc>(
        RTLD_DEFAULT, kMarkdownShadowCreatorSymbol);
    if (func && !err) {
      g_shadow_creator = func;
      LOGI("XElement markdown shadow creator resolved from RTLD_DEFAULT");
    }
  }
  if (g_ui_creator && g_shadow_creator) {
    return;
  }

  LOGI("XElement markdown dlopen " << kMarkdownSoName);
  dlerror();
  g_markdown_handle = dlopen(kMarkdownSoName, RTLD_NOW | RTLD_GLOBAL);
  if (!g_markdown_handle) {
    const char* err = dlerror();
    LOGE("XElement markdown dlopen " << kMarkdownSoName
                                     << " failed: " << (err ? err : "unknown"));
    return;
  }
  LOGI("XElement markdown dlopen " << kMarkdownSoName << " success");

  if (!g_ui_creator) {
    auto [func, err] = TryResolveSymbol<LynxContext::UICreatorFunc>(
        g_markdown_handle, kMarkdownUICreatorSymbol);
    if (func && !err) {
      g_ui_creator = func;
      LOGI("XElement markdown UI creator resolved from " << kMarkdownSoName);
    } else {
      LogDlsymFailure(kMarkdownUICreatorSymbol, err);
    }
  }
  if (!g_shadow_creator) {
    auto [func, err] = TryResolveSymbol<LynxContext::LayoutNodeCreatorFuc>(
        g_markdown_handle, kMarkdownShadowCreatorSymbol);
    if (func && !err) {
      g_shadow_creator = func;
      LOGI("XElement markdown shadow creator resolved from "
           << kMarkdownSoName);
    } else {
      LogDlsymFailure(kMarkdownShadowCreatorSymbol, err);
    }
  }
}

void EnsureMarkdownSymbolsResolved() {
  static std::once_flag once_flag;
  std::call_once(once_flag, ResolveMarkdownSymbols);
}

LynxContext::UICreatorFunc GetMarkdownUICreatorFunc() {
  EnsureMarkdownSymbolsResolved();
  return g_ui_creator;
}

LynxContext::LayoutNodeCreatorFuc GetMarkdownShadowCreatorFunc() {
  EnsureMarkdownSymbolsResolved();
  return g_shadow_creator;
}

UIBase* MarkdownUICreator(LynxContext* context, int sign,
                          const std::string& tag) {
  auto func = GetMarkdownUICreatorFunc();
  if (!func) {
    LOGE("XElement markdown UI creator missing, sign: " << sign
                                                        << ", tag: " << tag);
    return nullptr;
  }
  LOGI("XElement markdown dispatch UI creator, sign: " << sign
                                                       << ", tag: " << tag);
  return func(context, sign, tag);
}

ShadowNode* MarkdownShadowCreator(int sign, const std::string& tag) {
  auto func = GetMarkdownShadowCreatorFunc();
  if (!func) {
    LOGE("XElement markdown shadow creator missing, sign: " << sign << ", tag: "
                                                            << tag);
    return nullptr;
  }
  LOGI("XElement markdown dispatch shadow creator, sign: " << sign
                                                           << ", tag: " << tag);
  return func(sign, tag);
}

}  // namespace

void MarkdownRegistry::Initialize() {
  LOGI("XElement markdown register markdown tag");
  auto& map = LynxContext::GetCAPINodeInfoMap();
  map["markdown"] = {MarkdownUICreator, MarkdownShadowCreator,
                     LayoutNodeType::CUSTOM};
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
