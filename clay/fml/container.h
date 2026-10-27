// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_CONTAINER_H_
#define CLAY_FML_CONTAINER_H_

#include <functional>
#include <map>
#include <unordered_map>

namespace fml {

template <class Collection = std::unordered_map<class Key, class Value,
                                                class Hash, class Equal>>
void erase_if(Collection& container,  // NOLINT
              std::function<bool(typename Collection::iterator)> predicate) {
  auto it = container.begin();
  while (it != container.end()) {
    if (predicate(it)) {
      it = container.erase(it);
      continue;
    }
    it++;
  }
}

}  // namespace fml

#endif  // CLAY_FML_CONTAINER_H_
