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

#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"

#include <algorithm>
#include <memory>
#include <string>

namespace lynx {
namespace css {

unsigned LynxCSSSelectorList::CalcSpecificity() const {
  unsigned specificity = 0;

  for (const LynxCSSSelector* s = First(); s; s = Next(*s))
    specificity = std::max(specificity, s->CalcSpecificity());

  return specificity;
}

std::string LynxCSSSelectorList::SelectorsText(const LynxCSSSelector* first) {
  std::string result;

  for (const LynxCSSSelector* s = first; s; s = Next(*s)) {
    if (s != first) result.append(", ");
    result.append(s->ToString());
  }

  return result;
}

const LynxCSSSelector* LynxCSSSelectorList::Next(
    const LynxCSSSelector& current) {
  // Skip subparts of compound selectors.
  const LynxCSSSelector* last = &current;
  while (!last->IsLastInTagHistory()) last++;
  return last->IsLastInSelectorList() ? nullptr : last + 1;
}

}  // namespace css
}  // namespace lynx
