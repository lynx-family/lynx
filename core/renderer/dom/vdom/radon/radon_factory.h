// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_FACTORY_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_FACTORY_H_

#include <memory>
#include <unordered_map>

namespace lynx {
namespace tasm {
class RadonBase;
using PtrLookupMap = std::unordered_map<RadonBase*, RadonBase*>;

namespace radon_factory {
std::unique_ptr<RadonBase> Copy(const RadonBase& node, PtrLookupMap& map);
std::unique_ptr<RadonBase> Copy(const RadonBase& node);

// used for render_function::CloneSubTree
RadonBase* CopyRadonRawPtrForDiff(RadonBase& node, PtrLookupMap& map);
void CopyRadonDiffSubTreeAndAddToParent(RadonBase& parent, RadonBase& node,
                                        PtrLookupMap& map);
RadonBase* CopyRadonDiffSubTree(RadonBase& node);

}  // namespace radon_factory
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_FACTORY_H_
