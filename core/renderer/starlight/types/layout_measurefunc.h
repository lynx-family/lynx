// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_MEASUREFUNC_H_
#define CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_MEASUREFUNC_H_

#include "core/renderer/starlight/types/layout_constraints.h"

namespace lynx {
namespace starlight {

typedef FloatSize (*SLMeasureFunc)(void* context,
                                   const Constraints& constraints,
                                   bool final_measure);
typedef void (*SLAlignmentFunc)(void* context);

// Function to check if the layout of the layout object depends the mode of the
// constraint.
//
// In order to optimize the performance of layout, starlight trys to reuse the
// previous layout result whenever possible. This function will be called when
// the current constraint is definite and the value of current constraint is the
// same as a previous layout result but the previous layout constraint is not
// definite  to check whether the layout result can be reused in this case.
//
// For example:
// Previous layout constraint is {width:indefinite, height:indefinite},
// and the previous layout result is {width:100, height:200},
// and the current given constraint is {width:exactly 100 ,height:exactly 200}.
// In this case the function will be called to check whether current layout with
// current constraint can be skipped by reusing the previous layout result.
//
// Notice that the layout result also implicitly includes THE LAYOUT OF
// CHILDREN. In this case the layout result size of current will always be the
// same as previous layout result size. But the way that the current layout
// object layouts its children or content can be different. For example if one
// of the children has its width specified as "width:50%". During previous
// layout, the child's width cannot be resolved against an indefinite
// constraint. For current layout, the chil's width can be resolved to 50. The
// layout of the child will be different between current and previous layout.
// Thus the layout result can NOT be reused in this case.
//
// ATTENTION: When implementing this function, the check for common CSS (i.e.
// width height etc.) can be ignored because starlight will handle it.

using SLCanReuseLayoutWithSameSizeAsGivenConstraintFunc =
    bool (*)(void* context, bool is_horizontal_constraint);

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_MEASUREFUNC_H_
