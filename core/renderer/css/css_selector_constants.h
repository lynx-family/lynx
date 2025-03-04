// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_SELECTOR_CONSTANTS_H_
#define CORE_RENDERER_CSS_CSS_SELECTOR_CONSTANTS_H_

namespace lynx {
namespace tasm {

/*
 *  selectors            Example               Example Description
 *  .class               .intro      Select All Elements with class="intro"
 *  #id                 #first       Select Elements with id="first"
 *  element               view                Select All view Elements
 *  element, element   view, checkbox   Select All view, checkbox Elements
 *  :not           #foo:not(.bar)  Select Elements with id="foo" but class!=bar
 *  ::placeholder    input::placeholder   Placeholder Text Style of input
 * Elements More Selectors to Expand
 */

static constexpr const char kCSSSelectorClass[] = ".";
static constexpr const char kCSSSelectorID[] = "#";
static constexpr const char kCSSSelectorSelection[] = "::selection";
static constexpr const char kCSSSelectorNot[] = ":not";
static constexpr const char kCSSSelectorPlaceholder[] = "::placeholder";
static constexpr const char kCSSSelectorAll[] = "*";
static constexpr const char kCSSSelectorFirstChild[] = ":first-child";
static constexpr const char kCSSSelectorLastChild[] = ":last-child";
static constexpr const char kCSSSelectorPseudoFocus[] = ":focus";
static constexpr const char kCSSSelectorPseudoActive[] = ":active";
static constexpr const char kCSSSelectorPseudoHover[] = ":hover";

// TODO: add more

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_SELECTOR_CONSTANTS_H_
