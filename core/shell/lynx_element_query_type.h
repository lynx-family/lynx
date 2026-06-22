// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_ELEMENT_QUERY_TYPE_H_
#define CORE_SHELL_LYNX_ELEMENT_QUERY_TYPE_H_

#include <cstdint>

namespace lynx {
namespace shell {

enum class LynxElementQueryType : int32_t {
  kRootSign = 0,
  kIsAlive = 1,
  kIdentity = 2,
  kParentSign = 3,
  kChildrenSigns = 4,
  kFindById = 5,
  kDataset = 6,
  kAttributes = 7,
  kAttribute = 8,
  kJSONString = 9,
  kPositionInfo = 10,
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_ELEMENT_QUERY_TYPE_H_
