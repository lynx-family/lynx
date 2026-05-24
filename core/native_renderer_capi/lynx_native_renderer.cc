// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// Implementation of `lynx_native_renderer_capi.h`.
//
// Thin wrappers around the internal C++ classes — every `extern "C"`
// function casts an opaque handle to the corresponding C++ pointer
// and forwards. By living inside liblynx.so itself, the wrappers
// retain access to the internal classes without forcing
// `-fvisibility=default` on the entire engine: only the
// LYNX_NATIVE_RENDERER_CAPI_EXPORT-tagged functions in this file land in `.dynsym`.
//
// Shell extraction (JNI reflection on Android, Obj-C ivar access on
// iOS) stays on the embedder side — this file just wraps a raw
// pointer the embedder hands over, so it doesn't need JNIEnv or any
// platform headers.

#include "core/native_renderer_capi/public/lynx_native_renderer_capi.h"

#include <memory>
#include <utility>

#include "base/include/value/base_string.h"
#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/ui_wrapper/painting/catalyzer.h"
#include "core/shell/lynx_shell.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "core/value_wrapper/value_impl_lepus.h"

// ----- Internal handle structures ------------------------------------------
//
// Opaque to the C API; allocated/freed by this translation unit.

struct lynx_shell_t {
  lynx::shell::LynxShell* shell = nullptr;
  // Cached after first run_on_tasm_thread initialises fiber-arch.
  lynx::tasm::ElementManager* manager = nullptr;
  // Strong reference to the installed root page (so the underlying
  // PageElement isn't released while the shell still references it).
  fml::RefPtr<lynx::tasm::PageElement> root_page;
  bool fiber_arch_initialized = false;
};

struct lynx_fiber_element_t {
  fml::RefPtr<lynx::tasm::FiberElement> ref;
};

// ----- Shell wrapping + lifecycle -------------------------------------------

LYNX_NATIVE_RENDERER_CAPI_EXPORT lynx_shell_t* lynx_shell_from_native_ptr(
    void* native_shell_ptr) {
  if (native_shell_ptr == nullptr) {
    return nullptr;
  }
  auto* handle = new lynx_shell_t();
  handle->shell = reinterpret_cast<lynx::shell::LynxShell*>(native_shell_ptr);
  return handle;
}


LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_shell_release(lynx_shell_t* shell) {
  if (shell == nullptr) return;
  // Drop the page first to avoid a dangling FiberElement → ElementManager
  // back-pointer if the caller has already torn down the LynxView.
  shell->root_page = nullptr;
  delete shell;
}

// ----- Thread dispatch ------------------------------------------------------

LYNX_NATIVE_RENDERER_CAPI_EXPORT bool lynx_shell_run_on_tasm_thread(
    lynx_shell_t* shell,
    lynx_tasm_callback_t callback,
    void* user_data) {
  if (shell == nullptr || shell->shell == nullptr || callback == nullptr) {
    return false;
  }
  lynx_shell_t* capture = shell;
  shell->shell->RunOnTasmThread([capture, callback, user_data]() {
    if (!capture->fiber_arch_initialized) {
      // Lazy-initialise fiber-arch + cache the ElementManager. The
      // ElementManager only becomes available once the shell's TASM
      // thread is running, which is the case here.
      auto* tasm = capture->shell->GetTasm();
      if (tasm != nullptr) {
        auto config = std::make_shared<lynx::tasm::PageConfig>();
        config->SetEnableFiberArch(true);
        tasm->SetPageConfig(config);
        auto* page_proxy = tasm->page_proxy();
        if (page_proxy != nullptr) {
          capture->manager = page_proxy->element_manager().get();
        }
      }
      capture->fiber_arch_initialized = true;
    }
    callback(user_data);
  });
  return true;
}

// ----- Element creation -----------------------------------------------------

namespace {

fml::RefPtr<lynx::tasm::FiberElement> CreateForTag(
    lynx::tasm::ElementManager* manager,
    lynx_element_tag_e tag) {
  using namespace lynx;
  if (manager == nullptr) return nullptr;
  switch (tag) {
    case LYNX_ELEMENT_TAG_PAGE:
      // The "0" + id=0 pair mirrors the constants Lynx's internal
      // ReactLynx renderer uses for the root page. Native embedders
      // get the same shape so layout / pipeline behaves identically.
      return manager->CreateFiberPage(base::String("0"), 0);
    case LYNX_ELEMENT_TAG_VIEW:
      return manager->CreateFiberView();
    case LYNX_ELEMENT_TAG_TEXT:
      return manager->CreateFiberText(base::String("text"));
    case LYNX_ELEMENT_TAG_RAW_TEXT:
      return manager->CreateFiberRawText();
    case LYNX_ELEMENT_TAG_IMAGE:
      // TODO: expose a proper CreateFiberImage once the public API
      // grows it. For now, fall back to a View — the platform image
      // attribute is set via lynx_element_set_attribute("src", ...)
      // and Lynx's UI layer still handles rendering correctly.
      return manager->CreateFiberView();
    case LYNX_ELEMENT_TAG_SCROLL_VIEW:
      return manager->CreateFiberScrollView(
          base::String(lynx::tasm::kElementScrollViewTag));
  }
  return nullptr;
}

}  // namespace

LYNX_NATIVE_RENDERER_CAPI_EXPORT lynx_fiber_element_t* lynx_create_fiber_element(
    lynx_shell_t* shell,
    lynx_element_tag_e tag) {
  if (shell == nullptr || shell->manager == nullptr) return nullptr;
  auto ref = CreateForTag(shell->manager, tag);
  if (!ref) return nullptr;
  return new lynx_fiber_element_t{std::move(ref)};
}

LYNX_NATIVE_RENDERER_CAPI_EXPORT lynx_fiber_element_t*
lynx_create_fiber_element_by_name(lynx_shell_t* shell, const char* tag_name) {
  if (shell == nullptr || shell->manager == nullptr || tag_name == nullptr ||
      tag_name[0] == '\0') {
    return nullptr;
  }
  // `CreateFiberNode` is the generic factory ElementManager exposes
  // for any registered tag — built-ins (`view` / `text` / `image` /
  // `scroll-view`) and custom (`x-input` / `x-refresh` / …) alike.
  // For tags Lynx's behaviour registry doesn't know, it returns
  // nullptr and we surface that to the embedder.
  auto ref = shell->manager->CreateFiberNode(lynx::base::String(tag_name));
  if (!ref) return nullptr;
  return new lynx_fiber_element_t{std::move(ref)};
}

LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_release(lynx_fiber_element_t* element) {
  delete element;
}

LYNX_NATIVE_RENDERER_CAPI_EXPORT int32_t lynx_element_id(lynx_fiber_element_t* element) {
  if (element == nullptr || !element->ref) return 0;
  return element->ref->impl_id();
}

// ----- Element manipulation -------------------------------------------------

LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_set_attribute(
    lynx_fiber_element_t* element,
    const char* key,
    const char* value) {
  if (element == nullptr || !element->ref || key == nullptr ||
      value == nullptr) {
    return;
  }
  element->ref->SetAttribute(
      lynx::base::String(key),
      lynx::lepus::Value(lynx::base::String(value)));
}

LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_set_inline_styles(
    lynx_fiber_element_t* element,
    const char* css) {
  if (element == nullptr || !element->ref || css == nullptr) return;
  element->ref->SetRawInlineStyles(lynx::base::String(css));
}

LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_append_child(
    lynx_fiber_element_t* parent,
    lynx_fiber_element_t* child) {
  if (parent == nullptr || child == nullptr || !parent->ref || !child->ref) {
    return;
  }
  parent->ref->InsertNode(child->ref);
}

LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_remove_child(
    lynx_fiber_element_t* parent,
    lynx_fiber_element_t* child) {
  if (parent == nullptr || child == nullptr || !parent->ref || !child->ref) {
    return;
  }
  parent->ref->RemoveNode(child->ref);
}

// ----- Pipeline -------------------------------------------------------------

LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_shell_set_root_element(
    lynx_shell_t* shell,
    lynx_fiber_element_t* page) {
  if (shell == nullptr || shell->manager == nullptr || page == nullptr ||
      !page->ref) {
    return;
  }
  auto page_ref = fml::RefPtr<lynx::tasm::PageElement>(
      static_cast<lynx::tasm::PageElement*>(page->ref.get()));
  shell->manager->SetFiberPageElement(page_ref);
  shell->root_page = std::move(page_ref);
}

LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_shell_flush(lynx_shell_t* shell) {
  if (shell == nullptr || shell->manager == nullptr || !shell->root_page) {
    return;
  }
  shell->root_page->FlushActionsAsRoot();
  auto options = std::make_shared<lynx::tasm::PipelineOptions>();
  shell->manager->OnPatchFinish(options, shell->root_page.get());
}

// ----- UI method dispatch ---------------------------------------------------

LYNX_NATIVE_RENDERER_CAPI_EXPORT int32_t lynx_ui_invoke_method(
    lynx_shell_t* shell,
    int32_t sign,
    const char* method_name,
    const lynx_ui_method_value_t* args,
    size_t arg_count) {
  if (shell == nullptr || shell->manager == nullptr ||
      method_name == nullptr) {
    return -1;
  }
  auto* catalyzer = shell->manager->catalyzer();
  if (catalyzer == nullptr) {
    return -2;
  }

  // Package the args as `{"args": [arg0, arg1, ...]}` — the
  // convention Whisker's `WhiskerValue.fromNSDictionary` (iOS) /
  // `WhiskerValue.fromReadableMap` (Android) decoders expect from
  // their `@WhiskerUIMethod`-emitted forwarders.
  auto args_array = lynx::lepus::CArray::Create();
  for (size_t i = 0; args != nullptr && i < arg_count; i++) {
    const lynx_ui_method_value_t& v = args[i];
    switch (v.type) {
      case LYNX_UI_METHOD_VALUE_NULL:
        args_array->emplace_back(lynx::lepus::Value());
        break;
      case LYNX_UI_METHOD_VALUE_BOOL:
        args_array->emplace_back(lynx::lepus::Value(v.v.b));
        break;
      case LYNX_UI_METHOD_VALUE_INT:
        args_array->emplace_back(lynx::lepus::Value(v.v.i));
        break;
      case LYNX_UI_METHOD_VALUE_DOUBLE:
        args_array->emplace_back(lynx::lepus::Value(v.v.f));
        break;
      case LYNX_UI_METHOD_VALUE_STRING:
        args_array->emplace_back(lynx::lepus::Value(
            lynx::base::String(v.v.s != nullptr ? v.v.s : "")));
        break;
    }
  }
  auto params_dict = lynx::lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kArgs, "args");
  params_dict->SetValue(kArgs, lynx::lepus::Value(std::move(args_array)));
  lynx::lepus::Value params_lepus(std::move(params_dict));

  // Fire-and-forget: the platform Invoke routes the actual UI
  // method call to the main / UI thread via `dispatch_async` (iOS)
  // / a posted runnable (Android). The callback fires after the
  // method completes — typically synchronously inside the platform
  // method, but the Whisker C wrapper has no useful way to surface
  // an async result without growing an async API. v1 contract is
  // sync-only / discard-result.
  auto noop_callback = [](int32_t /*code*/, const lynx::pub::Value& /*data*/) {};

  catalyzer->Invoke(static_cast<int64_t>(sign),
                    std::string(method_name),
                    lynx::pub::ValueImplLepus(params_lepus),
                    noop_callback);
  return 0;
}

// ----- subsecond ASLR anchor ------------------------------------------------

// Intentionally non-empty so the linker doesn't merge it with other
// trivially-empty functions or strip it. The volatile keeps the
// compiler honest about the assignment having an observable effect.
LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_aslr_reference(void) {
  static volatile int marker = 0;
  marker = marker + 1;
}
