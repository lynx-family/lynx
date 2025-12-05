// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_SKITY_LAZY_IMAGE_TRAVELLER_H_
#define CLAY_GFX_SKITY_SKITY_LAZY_IMAGE_TRAVELLER_H_

#include <functional>
#include <memory>
#include <vector>

// Forward declarations to avoid heavy Skity includes in headers.
namespace skity {
class DisplayList;
}  // namespace skity

namespace clay {

//------------------------------------------------------------------------------
/// Can be fed to the dispatch() method of a DisplayList to feed the resulting
/// rendering operations to an SkCanvas instance.
///
/// Receives all methods on Dispatcher and sends them to an SkCanvas
///

using LazyImageDecodeCallback = std::function<void(bool)>;

class SkityLazyImageTraveller {
 public:
  static bool Traversal(skity::DisplayList* disp_list,
                        const LazyImageDecodeCallback& callback);
};

}  // namespace clay

#endif  // CLAY_GFX_SKITY_SKITY_LAZY_IMAGE_TRAVELLER_H_
