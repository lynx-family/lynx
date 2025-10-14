// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(NativeViewTest, TextureRegistry) {
  int view_id = 0;
  fml::Thread thread("mock_ui_platform");
  thread.GetTaskRunner()->PostSyncTask([&] {
    std::unique_ptr<PageView> page_view =
        std::make_unique<PageView>(view_id++, nullptr, thread.GetTaskRunner());
    std::unique_ptr<NativeView> root = std::make_unique<NativeView>(
        view_id++, "test-platform", page_view.get());
    EXPECT_EQ(root->IsNativeViewAvailable(), false);
  });
}

}  // namespace clay
