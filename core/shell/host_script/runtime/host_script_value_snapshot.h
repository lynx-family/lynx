// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_VALUE_SNAPSHOT_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_VALUE_SNAPSHOT_H_

#include <optional>

#include "base/include/value/base_value.h"
#include "core/public/pub_value.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx::shell {

// Copies a JavaScript value only when nothing is lost: functions, host
// objects, thenables, class instances, symbols, accessors, inherited array
// entries, cycles and nesting deeper than 64 levels are rejected. Properties
// are read through descriptors so snapshotting never invokes their getters.
std::optional<lepus::Value> SnapshotJSValue(runtime::js::Runtime& rt,
                                            const runtime::js::Value& value);

// Whether a platform value can be copied without loss under the same rules.
bool CanSnapshot(const pub::Value& value);

runtime::js::Value RestoreJSValue(runtime::js::Runtime& rt,
                                  const lepus::Value& value);

}  // namespace lynx::shell

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_VALUE_SNAPSHOT_H_
