// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_MARKDOWN_VIEW_BUNDLE_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_MARKDOWN_VIEW_BUNDLE_H_

#include <memory>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "markdown/platform/harmony/harmony_markdown_measurer.h"

namespace lynx {
namespace tasm {
namespace harmony {

class MarkdownViewBundle final : public fml::RefCountedThreadSafeStorage {
 public:
  static fml::RefPtr<MarkdownViewBundle> Create(
      std::shared_ptr<serval::markdown::NativeMarkdownMeasurer>
          markdown_measurer,
      serval::markdown::MeasureResult measured_size) {
    return fml::AdoptRef(
        new MarkdownViewBundle(std::move(markdown_measurer), measured_size));
  }

  const std::shared_ptr<serval::markdown::NativeMarkdownMeasurer>&
  GetMarkdownMeasurer() const {
    return markdown_measurer_;
  }
  serval::markdown::MeasureResult GetMeasuredSize() const {
    return measured_size_;
  }

 private:
  explicit MarkdownViewBundle(
      std::shared_ptr<serval::markdown::NativeMarkdownMeasurer>
          markdown_measurer,
      serval::markdown::MeasureResult measured_size)
      : markdown_measurer_(std::move(markdown_measurer)),
        measured_size_(measured_size) {}

  ~MarkdownViewBundle() override = default;
  void ReleaseSelf() const override { delete this; }

  std::shared_ptr<serval::markdown::NativeMarkdownMeasurer> markdown_measurer_;
  serval::markdown::MeasureResult measured_size_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_MARKDOWN_VIEW_BUNDLE_H_
