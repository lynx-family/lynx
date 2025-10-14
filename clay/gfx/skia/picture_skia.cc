// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skia/picture_skia.h"

#include "third_party/skia/src/core/SkBigPicture.h"
#include "third_party/skia/src/core/SkRecord.h"
#include "third_party/skia/src/core/SkRecords.h"

namespace clay {

namespace {  // Anonymous namespace for helpers

// Overload for commands that have a direct SkPaint member.
[[maybe_unused]] SkPaint* AsPaintPtr(SkPaint* paint) { return paint; }

// Overload for commands that have an Optional<SkPaint> member (e.g.,
// SaveLayer). It's assumed SkRecords::Optional has operator bool() and
// operator*().
[[maybe_unused]] SkPaint* AsPaintPtr(SkRecords::Optional<SkPaint>* paint) {
  if (paint && *paint) {
    return &(**paint);
  }
  return nullptr;
}

}  // namespace

// A mutator for SkRecord to get a mutable SkPaint pointer. It uses the
// AsPaintPtr helpers to handle both direct and optional paints.
struct PaintMutator {
  template <typename T>
  auto operator()(T* record) const -> decltype(AsPaintPtr(&record->paint)) {
    return AsPaintPtr(&record->paint);
  }

  // Fallback for commands with no 'paint' member.
  SkPaint* operator()(...) const { return nullptr; }
};

void PictureSkia::DispatchToWorklet(ClayAnimationPropertyType type,
                                    const clay::Color& value) {
  if (!picture_) {
    return;
  }

  SkBigPicture* big_picture = static_cast<SkBigPicture*>(picture_.get());
  if (!big_picture) {
    return;
  }
  for (auto& op : dynamic_ops_) {
    if ((ClayAnimationPropertyType::kColor == type &&
         op.first == DynamicOpType::kSetTextColor) ||
        (ClayAnimationPropertyType::kBackgroundColor == type &&
         op.first == DynamicOpType::kSetBackgroundColor)) {
      int offset = op.second;
      SkRecord* record = const_cast<SkRecord*>(big_picture->record());
      if (!record || offset < 0) {
        return;
      }
      PaintMutator mutator;
      SkPaint* paint = record->mutate(offset, mutator);
      if (!paint) {
        return;
      }
      paint->setColor(value);
    }
  }
}

Color PictureSkia::ObtainWorkletValue(ClayAnimationPropertyType type) const {
  if (!picture_) {
    return Color();
  }

  SkBigPicture* big_picture = static_cast<SkBigPicture*>(picture_.get());
  if (!big_picture) {
    return Color();
  }

  for (auto op : dynamic_ops_) {
    if ((ClayAnimationPropertyType::kColor == type &&
         op.first == DynamicOpType::kSetTextColor) ||
        (ClayAnimationPropertyType::kBackgroundColor == type &&
         op.first == DynamicOpType::kSetBackgroundColor)) {
      int offset = op.second;
      SkRecord* record = const_cast<SkRecord*>(big_picture->record());
      if (!record || offset < 0) {
        return Color();
      }
      PaintMutator mutator;
      SkPaint* paint = record->mutate(offset, mutator);
      if (paint) {
        // 32-bit ARGB color value
        return Color(paint->getColor());
      }
    }
  }
  return Color();
}
}  // namespace clay
