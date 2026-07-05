// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_UI_COMPONENT_SVG_IMAGE_VIEW_H_
#define CLAY_UI_COMPONENT_SVG_IMAGE_VIEW_H_

#include <memory>
#include <string>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/public/value.h"
#include "clay/ui/component/base_image_view.h"

namespace clay {

class Image;

class SVGImageView : public BaseImageView {
 public:
  SVGImageView(int id, PageView* page_view);
  ~SVGImageView() override = default;

  bool IsSVG() const override { return true; }
  const std::string& GetContent() const { return content_; }
  void SetContent(const std::string content);

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  void DidUpdateAttributes() override;
  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void NotifyLoadIfNeeded();

  uint64_t update_counter_ = 0;
  std::string content_;
  bool pending_load_event_ = false;
  fml::WeakPtrFactory<SVGImageView> weak_factory_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SVG_IMAGE_VIEW_H_
