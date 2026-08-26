// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <functional>
#include <string>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "clay/fml/paths.h"
#include "clay/ui/component/editable/input_view.h"
#include "clay/ui/resource/font_collection.h"
#include "clay/ui/testing/ui_test.h"
#include "gtest/gtest.h"

namespace clay {

class EditableViewTest : public UITest {
 protected:
  void UISetUp() override {}

  void LoadTestFont(const std::string& family_name,
                    std::function<void()> completion) {
    auto directory = fml::paths::GetExecutableDirectoryPath();
    ASSERT_TRUE(directory.first);
    auto dir = directory.second;
    auto pos = dir.find_last_of('/');
    if (pos != std::string::npos && dir.substr(pos + 1) == "exe.unstripped") {
      dir = dir.substr(0, pos);
    }
    auto font_path = fml::paths::JoinPaths(
        {dir, "gen/lynx/clay/third_party/txt/assets/Roboto-Bold.ttf"});
    font_path = "file://" + font_path;
    AsyncStart();
    FontCollection::Instance()->PreLoadFontOnMem(
        ui_task_runner(), nullptr, nullptr, family_name, {std::move(font_path)},
        [this, completion = std::move(completion)](bool success) {
          EXPECT_TRUE(success);
          completion();
          AsyncEnd();
        });
  }
};

TEST_F_UI(EditableViewTest, AsyncFontLoadRequestsRelayout) {
  const std::string family_name = "editable_font_loaded_after_layout";
  auto* input = new InputView(-1, page_.get());
  page_->AddChild(input);
  input->SetContent("existing content");
  input->SetFontFamily(family_name);
  input->SetBound(0, 0, 200, 80);

  Layout();
  ASSERT_FALSE(input->NeedsLayout());

  LoadTestFont(family_name, [input]() { EXPECT_TRUE(input->NeedsLayout()); });
}

}  // namespace clay
