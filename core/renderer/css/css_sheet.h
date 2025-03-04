// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_SHEET_H_
#define CORE_RENDERER_CSS_CSS_SHEET_H_

#include <memory>
#include <string>

#include "base/include/value/base_string.h"

namespace lynx {
namespace tasm {

class CSSSheet {
 public:
  enum SheetType {
    CLASS_SELECT = 1,
    ID_SELECT = 1 << 1,
    NAME_SELECT = 1 << 2,
    // deprecated
    AFTER_SELECT = 1 << 3,
    // deprecated
    BEFORE_SELECT = 1 << 4,
    NOT_SELECT = 1 << 5,
    PLACEHOLDER_SELECT = 1 << 6,
    ALL_SELECT = 1 << 7,
    FIRST_CHILD_SELECT = 1 << 8,
    LAST_CHILD_SELECT = 1 << 9,
    PSEUDO_FOCUS_SELECT = 1 << 10,
    SELECTION_SELECT = 1 << 11,
    PSEUDO_ACTIVE_SELECT = 1 << 12,
    PSEUDO_HOVER_SELECT = 1 << 13,
  };

  CSSSheet(const std::string& str);
  ~CSSSheet() {}

  int GetType() { return type_; }

  const base::String& GetSelector() { return selector_; }

  const base::String& GetName() { return name_; }

  void SetParent(std::shared_ptr<CSSSheet> ptr) { parent_ = ptr.get(); }

  CSSSheet* GetParent() { return parent_; }

  bool IsTouchPseudo() const;

 private:
  // for desirialize
  CSSSheet() {}
  friend class TemplateBinaryWriter;
  friend class TemplateBinaryReader;
  friend class TemplateBinaryReaderSSR;
  friend class LynxBinaryBaseCSSReader;

  void ConfirmType();

  int type_;
  // Single Rule, like .info, view
  base::String selector_;

  // Characters after removing the rules, such as "view" and "info"
  base::String name_;
  // std::shared_ptr<CSSSheet> parent_;
  CSSSheet* parent_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_SHEET_H_
