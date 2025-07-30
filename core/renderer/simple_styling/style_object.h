// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIMPLE_STYLING_STYLE_OBJECT_H_
#define CORE_RENDERER_SIMPLE_STYLING_STYLE_OBJECT_H_

#include <list>
#include <memory>
#include <mutex>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/vector.h"
#include "core/renderer/simple_styling/style_object_decoder.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx::style {
class SimpleStyleNode;

class StyleObject : public lepus::RefCounted {
 public:
  explicit StyleObject(tasm::StyleMap style_map)
      : style_map_(std::move(style_map)),
        data_(nullptr),
        length_(0),
        creator_(nullptr) {}
  /**
   * @brief Construct a new StyleObject object.
   *
   * This constructor initializes a StyleObject with the specified range within
   * the binary data, a pointer to the binary data, the length of the data, and
   * a function pointer to a decoder creator.
   *
   * @param start The starting offset of the style object's data within the
   * binary data buffer.
   * @param end The ending offset of the style object's data within the binary
   * data buffer.
   * @param data A pointer to the binary data buffer containing the style
   * object's data.
   * @param length The total length of the binary data buffer.
   * @param creator A function pointer to a decoder creator function, used to
   * create a decoder for the style object.
   */
  explicit StyleObject(uint32_t start, uint32_t end, uint8_t* data,
                       size_t length, const tasm::StringListVec& string_list,
                       const DecoderCreatorFunc creator)
      : range_(start, end),
        data_(data),
        length_(length),
        string_list_(string_list),
        creator_(creator) {}

  virtual void BindToElement(SimpleStyleNode* element);

  /**
   * Unbind style object from the element before element is destroyed,
   * preventing from UAF.
   * @param element The element to be unbound.
   */
  virtual void UnbindFromElement(SimpleStyleNode* element);

  /**
   * Reset the style object to the initial state.
   * @param element The element to be reset.
   */
  void ResetStylesInElement(SimpleStyleNode* element) const;

  auto begin() const { return style_map_.begin(); }
  auto end() const { return style_map_.end(); }
  void FromBinary();
  lepus::RefType GetRefType() const override;

  const tasm::StyleMap& Properties() const { return style_map_; }

 protected:
  tasm::StyleMap style_map_;

 private:
  void DecodeImmediately();

  tasm::CSSRange range_;

  uint8_t* data_;
  size_t length_;
  tasm::StringListVec string_list_;

  std::once_flag decode_flag_;
  DecoderCreatorFunc creator_;
};

class DynamicStyleObject : public StyleObject {
 public:
  explicit DynamicStyleObject(tasm::StyleMap style_map)
      : StyleObject(std::move(style_map)) {}
  void BindToElement(SimpleStyleNode* element) override;
  void UnbindFromElement(SimpleStyleNode* element) override;
  void UpdateStyleMap(const tasm::StyleMap& style_map);
  void Reset();

 private:
  base::InlineVector<SimpleStyleNode*, 1> elements_;
};
struct StyleObjectArrayDeleter {
  StyleObjectArrayDeleter() = default;
  void operator()(StyleObject** obj_array) const {
    StyleObject** array = obj_array;
    while (*obj_array) {
      (*obj_array)->Release();
      ++obj_array;
    }
    free(array);
  }
};

std::unique_ptr<StyleObject*, StyleObjectArrayDeleter> CreateStyleObjectArray(
    int capacity);

}  // namespace lynx::style

#endif  // CORE_RENDERER_SIMPLE_STYLING_STYLE_OBJECT_H_
