// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/transform_layer.h"

#include <memory>
#include <optional>
#include <string>

#include "clay/flow/animation/animation_mutator.h"

namespace clay {

TransformLayer::TransformLayer(const skity::Matrix& transform)
    : transform_(transform) {
  // Checks (in some degree) that skity::Matrix transform_ is valid and
  // initialized.
  //
  // If transform_ is uninitialized, this assert may look flaky as it doesn't
  // fail all the time, and some rerun may make it pass. But don't ignore it and
  // just rerun the test if this is triggered, since even a flaky failure here
  // may signify a potentially big problem in the code.
  //
  // We have to write this flaky test because there is no reliable way to test
  // whether a variable is initialized or not in C++.
  skity::Matrix& mat = std::get<skity::Matrix>(transform_);
  FML_DCHECK(mat.IsFinite());
  if (!mat.IsFinite()) {
    FML_DLOG(ERROR) << "TransformLayer is constructed with an invalid matrix.";
    mat.Reset();
  }
}

TransformLayer::TransformLayer(const clay::TransformOperations& transform,
                               skity::Vec2 origin, skity::Vec2 offset)
    : transform_(transform), origin_(origin), offset_(offset) {}

void TransformLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const TransformLayer*>(old_layer);
  skity::Matrix transform = GetMatrix();
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (transform != prev->GetMatrix()) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }
  if (HasAnimationRunning()) {
    context->MarkSubtreeHasRasterAnimation();
    if (!context->IsSubtreeDirty()) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }
  context->PushTransform(transform);
  DiffChildren(context, prev);
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void TransformLayer::Preroll(PrerollContext* context) {
  bool prev = context->parent_has_running_transform_animation;
  context->parent_has_running_transform_animation |= HasAnimationRunning();

  auto mutator = context->state_stack.save();
  skity::Matrix transform = GetMatrix();
  mutator.transform(transform);

  skity::Rect child_paint_bounds = skity::Rect::MakeEmpty();
  PrerollChildren(context, &child_paint_bounds);

  if (HasAnimationRunning()) {
    context->has_running_transform_animation = true;
  }

  transform.MapRect(&child_paint_bounds, child_paint_bounds);
  set_paint_bounds(child_paint_bounds);

  context->parent_has_running_transform_animation = prev;
}

void TransformLayer::Paint(PaintContext& context) const {
  FML_DCHECK(needs_painting(context));

  auto mutator = context.state_stack.save();
  mutator.transform(GetMatrix());

  PaintChildren(context);
}

skity::Matrix TransformLayer::GetMatrix() const {
  if (HasAnimation()) {
    const std::shared_ptr<AnimationMutator>& mutator = GetAnimationMutator();
    if (mutator->asTransform()) {
      clay::TransformOperations transform = mutator->asTransform()->transform();
      return transform.Apply()
          .matrix()
          .PreTranslate(-origin_.x, -origin_.y)
          .PostTranslate(origin_.x, origin_.y)
          .PostTranslate(offset_.x, offset_.y);
    } else if (mutator->asScrollOffset()) {
      return mutator->asScrollOffset()->transform();
    } else {
      FML_DLOG(INFO) << "TransformLayer::GetMatrix() called with an unexpected "
                        "animation type:"
                     << static_cast<int>(mutator->GetType());
    }
  }
  if (std::holds_alternative<skity::Matrix>(transform_)) {
    return std::get<skity::Matrix>(transform_);
  } else {
    clay::TransformOperations transform =
        std::get<clay::TransformOperations>(transform_);
    return transform.Apply()
        .matrix()
        .PreTranslate(-origin_.x, -origin_.y)
        .PostTranslate(origin_.x, origin_.y)
        .PostTranslate(offset_.x, offset_.y);
  }
}

clay::TransformOperations TransformLayer::GetTransform() const {
  return std::get<clay::TransformOperations>(transform_);
}

#ifndef NDEBUG
std::string TransformLayer::ToString() const {
  std::stringstream ss;
  skity::Matrix transform = GetMatrix();
  ss << ContainerLayer::ToString();
  ss << " transformX=" << transform.GetTranslateX();
  ss << " transformY=" << transform.GetTranslateY();
  ss << " scaleX=" << transform.GetScaleX();
  ss << " scaleY=" << transform.GetScaleY();
  ss << " transform_="
     << "(" << transform.GetScaleX() << "," << transform.GetSkewX() << ","
     << transform.GetTranslateX() << "," << transform.GetSkewY() << ","
     << transform.GetScaleY() << "," << transform.GetTranslateY() << ","
     << transform.GetPersp0() << "," << transform.GetPersp1() << ","
     << transform.GetPersp2() << ")";
  return ss.str();
}
#endif

}  // namespace clay
