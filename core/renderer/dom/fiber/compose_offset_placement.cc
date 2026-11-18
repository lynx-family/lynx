// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_offset_placement.h"

#include <cmath>
#include <limits>

#include "base/include/auto_reset.h"
#include "base/include/log/logging.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx::tasm {
namespace {
BASE_STATIC_STRING_DECL(kOffsetX, "x");
BASE_STATIC_STRING_DECL(kOffsetY, "y");
}  // namespace

ComposeOffsetPlacement::ComposeOffsetPlacement(Element& owner,
                                               runtime::MTSRuntime& runtime,
                                               const lepus::Value& callback)
    : owner_(&owner), runtime_(runtime) {
  callback_.CopyWeakValue(callback);
}

void ComposeOffsetPlacement::visitor(void* rt, void* func,
                                     uint64_t trace_tool) const {
  if (callback_.IsJSValue()) {
    reinterpret_cast<LEPUS_MarkFunc*>(func)(static_cast<LEPUSRuntime*>(rt),
                                            WRAP_AS_JS_VALUE(callback_.value()),
                                            trace_tool);
  }
}

void ComposeOffsetPlacement::Attach() {
  owner_->MarkLayoutDirtyLite();
  attached_ = true;
  owner_->slnode()->SetPlacementOffsetFunctions(
      {this, [](void* context) {
         return static_cast<ComposeOffsetPlacement*>(context)->Resolve();
       }});
}

void ComposeOffsetPlacement::Detach() {
  if (!attached_) return;
  attached_ = false;
  if (!owner_->will_destroy() && owner_->slnode()) {
    owner_->slnode()->SetPlacementOffsetFunctions({});
    owner_->element_manager()->SetNeedsLayout();
  }
}

starlight::PlacementOffset ComposeOffsetPlacement::Resolve() {
  auto keep_alive = shared_from_this();
  if (!attached_ || resolving_ || owner_->will_destroy()) return {};
  base::AutoReset<bool> resolving(&resolving_, true);
  const auto result = runtime_.CallClosure(callback_, lepus::Value(owner_));
  if (!attached_ || !result.IsObject()) return {};
  const auto x = result.GetProperty(kOffsetX).ToLepusValue();
  const auto y = result.GetProperty(kOffsetY).ToLepusValue();
  const double pixels = owner_->computed_css_style()
                            ->GetLayoutComputedStyle()
                            ->PhysicalPixelsPerLayoutUnit();
  if (!x.IsNumber() || !y.IsNumber() || !std::isfinite(x.Number()) ||
      !std::isfinite(y.Number()) || !std::isfinite(pixels) || pixels <= 0 ||
      std::abs(x.Number() / pixels) > std::numeric_limits<float>::max() ||
      std::abs(y.Number() / pixels) > std::numeric_limits<float>::max()) {
    LOGE("offset placement resolver returned invalid physical coordinates");
    return {};
  }
  return {static_cast<float>(x.Number() / pixels),
          static_cast<float>(y.Number() / pixels)};
}

}  // namespace lynx::tasm
