// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
/*
 * Copyright (C) 2008, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CORE_RENDERER_CSS_NG_SELECTOR_LYNX_CSS_SELECTOR_LIST_H_
#define CORE_RENDERER_CSS_NG_SELECTOR_LYNX_CSS_SELECTOR_LIST_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/ng/selector/lynx_css_selector.h"

namespace lynx {
namespace css {

// This class represents a CSS selector, i.e. a pattern of one or more
// simple selectors. https://www.w3.org/TR/css3-selectors/

// More specifically, a CSS selector is a chain of one or more sequences
// of simple selectors separated by combinators.
//
// For example, "div.c1 > span.c2 + .c3#ident" is represented as a
// LynxCSSSelectorList that owns six LynxCSSSelector instances.
//
// The simple selectors are stored in memory in the following order:
// .c3, #ident, span, .c2, div, .c1
// (See LynxCSSSelector.h for more information.)
//
// First() and Next() can be used to traverse from right to left through
// the chain of sequences: .c3#ident then span.c2 then div.c1
//
// SelectorAt and IndexOfNextSelectorAfter provide an equivalent API:
// size_t index = 0;
// do {
//   const LynxCSSSelector& sequence = selectorList.SelectorAt(index);
//   ...
//   index = IndexOfNextSelectorAfter(index);
// } while (index != UINT_MAX);
//
// Use LynxCSSSelector::TagHistory() and LynxCSSSelector::IsLastInTagHistory()
// to traverse through each sequence of simple selectors,
// from .c3 to #ident; from span to .c2; from div to .c1
//
// StyleRule stores its selectors in an identical memory layout,
// but not as part of a LynxCSSSelectorList (see its class comments).
// It reuses many of the exposed static member functions from
// LynxCSSSelectorList to provide a subset of its API.
class LynxCSSSelectorList {
 public:
  LynxCSSSelectorList() = default;

  LynxCSSSelectorList(LynxCSSSelectorList&& o)
      : selector_array_(std::move(o.selector_array_)) {}

  LynxCSSSelectorList(std::unique_ptr<LynxCSSSelector[]> selector_array)
      : selector_array_(std::move(selector_array)) {}

  LynxCSSSelectorList& operator=(LynxCSSSelectorList&& o) {
    DCHECK(this != &o);
    selector_array_ = std::move(o.selector_array_);
    return *this;
  }

  ~LynxCSSSelectorList() = default;

  bool IsValid() const { return !!selector_array_; }
  const LynxCSSSelector* First() const { return selector_array_.get(); }
  static const LynxCSSSelector* Next(const LynxCSSSelector&);

  // The CSS selector represents a single sequence of simple selectors.
  bool HasOneSelector() const { return selector_array_ && !Next(*First()); }

  std::string SelectorsText() const { return SelectorsText(First()); }
  static std::string SelectorsText(const LynxCSSSelector* first);

  // Selector lists don't know their length, computing it is O(n) and should be
  // avoided when possible. Instead iterate from first() and using next().
  // unsigned ComputeLength() const;

  // Return the specificity of the selector with the highest specificity.
  unsigned CalcSpecificity() const;

  LynxCSSSelectorList(const LynxCSSSelectorList&) = delete;
  LynxCSSSelectorList& operator=(const LynxCSSSelectorList&) = delete;

 private:
  // End of a multipart selector is indicated by is_last_in_tag_history_ bit in
  // the last item. End of the array is indicated by is_last_in_selector_list_
  // bit in the last item.
  std::unique_ptr<LynxCSSSelector[]> selector_array_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_SELECTOR_LYNX_CSS_SELECTOR_LIST_H_
