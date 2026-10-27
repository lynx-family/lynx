// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/gl_context_switch_test.h"

namespace clay {
namespace testing {

static thread_local int current_context;

TestSwitchableGLContext::TestSwitchableGLContext(int context)
    : context_(context){};

TestSwitchableGLContext::~TestSwitchableGLContext() = default;

bool TestSwitchableGLContext::SetCurrent() {
  SetCurrentContext(context_);
  return true;
};

bool TestSwitchableGLContext::RemoveCurrent() {
  SetCurrentContext(-1);
  return true;
};

int TestSwitchableGLContext::GetContext() { return context_; };

int TestSwitchableGLContext::GetCurrentContext() { return current_context; };

void TestSwitchableGLContext::SetCurrentContext(int context) {
  current_context = context;
};
}  // namespace testing
}  // namespace clay
