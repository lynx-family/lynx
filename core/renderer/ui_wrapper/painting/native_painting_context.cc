// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/native_painting_context.h"

#include <utility>

#include "base/include/debug/lynx_assert.h"

namespace lynx::tasm {

NativePaintingContext::ScopedDisplayListBatch::ScopedDisplayListBatch(
    NativePaintingContext* context, size_t capacity)
    : context_(context) {
  batch_.reserve(capacity);
  context_->BeginDisplayListBatch(&batch_);
}

NativePaintingContext::ScopedDisplayListBatch::~ScopedDisplayListBatch() {
  context_->EndDisplayListBatch(&batch_);
}

void NativePaintingContext::UpdateDisplayList(int id, DisplayList list) {
  if (current_display_list_batch_ != nullptr) {
    current_display_list_batch_->emplace_back(
        DisplayListUpdate{id, std::move(list)});
    return;
  }
  EnqueueDisplayList(id, std::move(list));
}

void NativePaintingContext::ReconstructEventTargetTreeRecursively() {
  if (current_display_list_batch_ != nullptr) {
    SubmitDisplayListBatch();
  }
  EnqueueReconstructEventTargetTreeRecursively();
}

void NativePaintingContext::BeginDisplayListBatch(
    DisplayListUpdateBatch* batch) {
  DCHECK(current_display_list_batch_ == nullptr);
  current_display_list_batch_ = batch;
}

void NativePaintingContext::EndDisplayListBatch(DisplayListUpdateBatch* batch) {
  DCHECK(current_display_list_batch_ == batch);
  SubmitDisplayListBatch();
  current_display_list_batch_ = nullptr;
}

void NativePaintingContext::SubmitDisplayListBatch() {
  if (!current_display_list_batch_->empty()) {
    EnqueueDisplayLists(std::move(*current_display_list_batch_));
  }
}

}  // namespace lynx::tasm
