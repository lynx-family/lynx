// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_STAGED_IMAGE_READ_SCOPE_H_
#define CLAY_COMMON_GRAPHICS_STAGED_IMAGE_READ_SCOPE_H_
#include <functional>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "clay/gfx/shared_image/output_texture.h"
namespace clay::display {
struct SourceReadState {
  bool pending = false;
};
// Raster-only batch. External input storage stays pinned until all output
// writes finish, even when the tree/page is destroyed or a partial batch is
// discarded. Write guards must finish EndWrite before the scope ends so each
// batch can retain its own completion independently of later record reuse.
class StagedImageReadScope {
 public:
  explicit StagedImageReadScope(
      fml::RefPtr<fml::TaskRunner> runner,
      std::function<void(std::function<void()>)> context);
  ~StagedImageReadScope();
  static StagedImageReadScope* Current();
  bool Contains(const void* source) const {
    return sources_.count(source) != 0;
  }
  void Hold(const void* source, std::shared_ptr<SourceReadState> state,
            std::function<void()> release);
  void Track(std::shared_ptr<ResourceRecord> record) {
    records_.push_back(std::move(record));
  }
  void Block() { blocked_ = true; }
  bool blocked() const { return blocked_; }

 private:
  fml::RefPtr<fml::TaskRunner> runner_;
  std::function<void(std::function<void()>)> context_;
  std::unordered_set<const void*> sources_;
  std::vector<std::function<void()>> releases_;
  std::vector<std::shared_ptr<ResourceRecord>> records_;
  bool blocked_ = false;
};
}  // namespace clay::display
#endif  // CLAY_COMMON_GRAPHICS_STAGED_IMAGE_READ_SCOPE_H_
