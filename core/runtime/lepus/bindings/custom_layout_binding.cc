// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>
#include <memory>

#include "base/include/value/table.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/custom_layout.h"
#include "core/renderer/dom/fiber/modifier_element.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx {
namespace tasm {
namespace {

ComposeElementHandle* LayoutHandle(const lepus::Value& value) {
  if (!value.IsRefCounted() ||
      value.RefCounted()->GetRefType() != lepus::RefType::kComposeElementHandle)
    return nullptr;
  return static_cast<ComposeElementHandle*>(value.RefCounted().get());
}

Element* LayoutChild(const lepus::Value& value) {
  if (auto* handle = LayoutHandle(value)) return handle->mount_root().get();
  if (value.IsRefCounted() &&
      value.RefCounted()->GetRefType() == lepus::RefType::kElement)
    return static_cast<Element*>(value.RefCounted().get());
  return nullptr;
}

bool Owns(runtime::MTSRuntime* runtime, Element* element) {
  auto* tasm = static_cast<TemplateAssembler*>(runtime->GetDelegate());
  return element && !element->will_destroy() && tasm &&
         element->element_manager() ==
             tasm->page_proxy()->element_manager().get();
}

float PixelScale(Element& element) {
  return element.computed_css_style()
      ->GetLayoutComputedStyle()
      ->PhysicalPixelsPerLayoutUnit();
}

lepus::Value Error(runtime::MTSRuntime* runtime) {
  return runtime->ReportFatalError("Invalid custom Layout operation", false,
                                   error::E_ELEMENT_API_ERROR);
}

lepus::Value MeasureLayoutChild(runtime::MTSRuntime* runtime,
                                CustomLayout& layout, Element& child,
                                const lepus::Value* bounds) {
  const float scale = PixelScale(layout.owner());
  float constraints[4];
  for (int i = 0; i < 4; ++i) {
    if (!bounds[i].IsNumber()) return Error(runtime);
    const double bound = bounds[i].Number();
    constraints[i] = bound == -1 ? -1.f : static_cast<float>(bound / scale);
  }
  LayoutResult size;
  if (!layout.MeasureChild(child, constraints[0], constraints[1],
                           constraints[2], constraints[3], size)) {
    return Error(runtime);
  }
  auto result = lepus::Value(lepus::Dictionary::Create());
  result.SetProperty(BASE_STATIC_STRING(kWidth),
                     lepus::Value(size.width_ * scale));
  result.SetProperty(BASE_STATIC_STRING(kHeight),
                     lepus::Value(size.height_ * scale));
  return result;
}

int32_t NonnegativeInt32(const lepus::Value& value) {
  if (!value.IsNumber()) return -1;
  const double number = value.Number();
  if (!(number >= 0 && number <= std::numeric_limits<int32_t>::max()))
    return -1;
  auto integer = static_cast<int32_t>(number);
  return integer == number ? integer : -1;
}

}  // namespace

lepus::Value RendererFunctions::FiberSetCustomLayout(runtime::MTSContext* ctx,
                                                     lepus::Value* argv,
                                                     int argc) {
  auto* runtime = runtime::MTSRuntime::ToContext(ctx);
  auto* handle = argc ? LayoutHandle(argv[0]) : nullptr;
  auto* owner = handle ? handle->content_element().get() : nullptr;
  if (argc != 3 || !Owns(runtime, owner) || owner->GetTag() != "view")
    return Error(runtime);
  if (argv[1].IsNil() && argv[2].IsNil()) {
    if (!handle->SetCustomLayout(nullptr)) return Error(runtime);
    return lepus::Value();
  }
  if (!argv[1].IsCallable() || !argv[2].IsCallable()) return Error(runtime);
  // Reuse the existing callback/visitor convention. Compose supplies
  // dispatchers that weakly reference its policy state, avoiding owner cycles
  // in RC mode.
  if (!handle->SetCustomLayout(
          CustomLayout::Create(*owner, *runtime, argv[1], argv[2])))
    return Error(runtime);
  return lepus::Value();
}

lepus::Value RendererFunctions::FiberMeasureElement(runtime::MTSContext* ctx,
                                                    lepus::Value* argv,
                                                    int argc) {
  auto* runtime = runtime::MTSRuntime::ToContext(ctx);
  auto* handle = argc ? LayoutHandle(argv[0]) : nullptr;
  auto* owner = handle ? handle->content_element().get() : nullptr;
  auto* child = argc > 1 ? LayoutChild(argv[1]) : nullptr;
  if (argc != 6 || !Owns(runtime, owner) || !Owns(runtime, child) ||
      !handle->custom_layout())
    return Error(runtime);
  return MeasureLayoutChild(runtime, *handle->custom_layout(), *child,
                            argv + 2);
}

lepus::Value RendererFunctions::FiberPlaceElement(runtime::MTSContext* ctx,
                                                  lepus::Value* argv,
                                                  int argc) {
  auto* runtime = runtime::MTSRuntime::ToContext(ctx);
  auto* handle = argc ? LayoutHandle(argv[0]) : nullptr;
  auto* owner = handle ? handle->content_element().get() : nullptr;
  auto* child = argc > 1 ? LayoutChild(argv[1]) : nullptr;
  if (argc != 5 || !Owns(runtime, owner) || !Owns(runtime, child) ||
      !handle->custom_layout())
    return Error(runtime);
  for (int i = 2; i < 5; ++i)
    if (!argv[i].IsNumber()) return Error(runtime);
  float scale = PixelScale(*owner);
  if (!handle->custom_layout()->PlaceChild(*child, argv[2].Number() / scale,
                                           argv[3].Number() / scale,
                                           argv[4].Number()))
    return Error(runtime);
  return lepus::Value();
}

lepus::Value RendererFunctions::FiberInvalidateLayout(runtime::MTSContext* ctx,
                                                      lepus::Value* argv,
                                                      int argc) {
  auto* runtime = runtime::MTSRuntime::ToContext(ctx);
  auto* handle = argc ? LayoutHandle(argv[0]) : nullptr;
  auto* owner = handle ? handle->content_element().get() : nullptr;
  if (argc != 1 || !Owns(runtime, owner) || !handle->custom_layout() ||
      handle->custom_layout()->IsExecuting())
    return Error(runtime);
  handle->custom_layout()->Invalidate();
  return lepus::Value();
}

lepus::Value RendererFunctions::FiberMeasureLayoutModifierChild(
    runtime::MTSContext* ctx, lepus::Value* argv, int argc) {
  auto* runtime = runtime::MTSRuntime::ToContext(ctx);
  if (argc == 4) {
    if (auto* layout = CustomLayout::ActiveModifierMeasure(*runtime)) {
      if (auto* child = layout->ModifierChild()) {
        return MeasureLayoutChild(runtime, *layout, *child, argv);
      }
    }
  }
  return Error(runtime);
}

lepus::Value RendererFunctions::FiberInvalidateLayoutModifier(
    runtime::MTSContext* ctx, lepus::Value* argv, int argc) {
  auto* runtime = runtime::MTSRuntime::ToContext(ctx);
  if (argc != 2) return Error(runtime);
  const int32_t id = NonnegativeInt32(argv[0]);
  const int32_t epoch = NonnegativeInt32(argv[1]);
  auto* tasm = static_cast<TemplateAssembler*>(runtime->GetDelegate());
  if (id < 0 || epoch < 0 || !tasm) return Error(runtime);
  auto* manager = tasm->page_proxy()->element_manager().get();
  auto* target = manager->node_manager()->Get(id);
  if (!target || target->will_destroy() || !target->is_modifier() ||
      !target->IsAttached()) {
    return lepus::Value(false);
  }
  const auto& frame = static_cast<ModifierElement&>(*target);
  auto* layout = frame.layout();
  if (!layout || frame.layout_epoch() != epoch || layout->IsExecuting() ||
      !layout->UsesRuntime(*runtime)) {
    return lepus::Value(false);
  }
  layout->Invalidate();
  return lepus::Value(true);
}

}  // namespace tasm
}  // namespace lynx
