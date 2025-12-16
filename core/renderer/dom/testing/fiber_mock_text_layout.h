// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_TESTING_FIBER_MOCK_TEXT_LAYOUT_H_
#define CORE_RENDERER_DOM_TESTING_FIBER_MOCK_TEXT_LAYOUT_H_

#define private public
#define protect public

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "core/public/text_layout_impl.h"

namespace lynx {
namespace tasm {
class FiberElement;
class TextElement;
class ViewElement;
class ImageElement;

namespace testing {
class PropArrayMock {
 public:
  PropArrayMock() = default;

  void AddProp(int value) { props_.emplace_back(value); }
  void AddProp(unsigned int value) { props_.emplace_back(value); }
  void AddProp(const char* value) { props_.emplace_back(std::string(value)); }

  void AddProp(bool value) { props_.emplace_back(value); }

  void AddProp(double value) { props_.emplace_back(value); }

  const std::vector<std::variant<int, unsigned int, std::string, bool, double>>&
  GetProps() const {
    return props_;
  }

 private:
  std::vector<std::variant<int, unsigned int, std::string, bool, double>>
      props_;

  PropArrayMock(const PropArrayMock&) = delete;
  PropArrayMock& operator=(const PropArrayMock&) = delete;
};

class TextLayoutMock : public TextLayoutImpl {
 public:
  TextLayoutMock();
  ~TextLayoutMock() override;

  LayoutResult Measure(Element* element, float width, int width_mode,
                       float height, int height_mode) override;

  void Align(Element* element) override{};

  void DispatchLayoutBefore(Element* element) override;

 private:
  void BuildTextPropsBuffer(TextElement* element, std::string& output,
                            size_t& current_length, bool use_utf16,
                            PropArrayMock* props, bool* has_inline_view);
  void ProcessChildProps(FiberElement* element, std::string& output,
                         size_t& current_length, bool use_utf16,
                         PropArrayMock* props, bool* has_inline_view);
  static void AppendTextProps(TextElement* element, size_t pos_start,
                              size_t pos_end, PropArrayMock* props);
  static void AppendViewProps(ViewElement* view_element, size_t start,
                              size_t end, PropArrayMock* props);
  static void AppendImageProps(ImageElement* image_element, size_t start,
                               size_t end, PropArrayMock* props);
  std::unordered_map<int32_t, std::unique_ptr<PropArrayMock>>
      text_layout_results_;
};

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_TESTING_FIBER_MOCK_TEXT_LAYOUT_H_
