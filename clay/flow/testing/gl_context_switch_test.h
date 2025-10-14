// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_TESTING_GL_CONTEXT_SWITCH_TEST_H_
#define CLAY_FLOW_TESTING_GL_CONTEXT_SWITCH_TEST_H_

#include "clay/common/graphics/gl_context_switch.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

//------------------------------------------------------------------------------
/// The renderer context used for testing
class TestSwitchableGLContext : public SwitchableGLContext {
 public:
  explicit TestSwitchableGLContext(int context);

  ~TestSwitchableGLContext() override;

  bool SetCurrent() override;

  bool RemoveCurrent() override;

  int GetContext();

  static int GetCurrentContext();

  //------------------------------------------------------------------------------
  /// Set the current context
  ///
  /// This is to mimic how other programs outside flutter sets the context.
  static void SetCurrentContext(int context);

 private:
  int context_;

  BASE_DISALLOW_COPY_AND_ASSIGN(TestSwitchableGLContext);
};

}  // namespace testing
}  // namespace clay

#endif  // CLAY_FLOW_TESTING_GL_CONTEXT_SWITCH_TEST_H_
