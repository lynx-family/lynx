// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_TESTING_UTILS_H_
#define CORE_LIST_TESTING_UTILS_H_

namespace lynx {
namespace list {

#ifndef TEST_LIST_CONTAINER_RESOLVE_PROP
#define TEST_LIST_CONTAINER_RESOLVE_PROP(name) \
  TEST_F(ListContainerImplTest, ResolveProp##name)
#endif

#ifndef LIST_CONTAINER_DEFINE_PROP_VALUE
#define LIST_CONTAINER_DEFINE_PROP_VALUE(name, value_type, init_value) \
  auto key = value_factory_->CreateString(kProp##name);                \
  auto value = value_factory_->Create##value_type(init_value);
#endif

#ifndef LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE
#define LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(name, init_lepus_value) \
  auto key = value_factory_->CreateString(kProp##name);                \
  auto value = pub::ValueImplLepus(init_lepus_value);
#endif

#ifndef TO_MOCK_LIST_ITEM_ELEMENT
#define TO_MOCK_LIST_ITEM_ELEMENT(item_delegate) \
  (static_cast<MockListItemElement*>(item_delegate))
#endif
}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_TESTING_UTILS_H_
