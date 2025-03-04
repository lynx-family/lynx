// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/ref_counted_class.h"

#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/lepus_value.h"
namespace lynx {
namespace lepus {

LEPUSClassExoticMethods RefCounted::exotic_methods_ = {};

LEPUSClassDef RefCounted::ref_counted_class_def_ = {.class_name = class_name,
                                                    .exotic = &exotic_methods_};

void RefCounted::InitRefCountedClass(LEPUSRuntime* runtime) {
  class_id = LEPUS_NewClassID(&class_id);
  if (LEPUS_NewClass(runtime, class_id, &ref_counted_class_def_) < 0) {
    class_id = 0;
  }
  return;
}

}  // namespace lepus
}  // namespace lynx
