// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/base/threading/task_runner_manufactor.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

class TaskRunnerManufactorTest : public ::testing::Test {
 protected:
  TaskRunnerManufactorTest() = default;
  ~TaskRunnerManufactorTest() override = default;

  void SetUp() override { UIThread::Init(); }
};

TEST_F(TaskRunnerManufactorTest, AllOnUIThreadMode) {
  TaskRunnerManufactor ui_mode_manufactor =
      TaskRunnerManufactor(ALL_ON_UI, false, false);
  ASSERT_EQ(ui_mode_manufactor.GetTASMTaskRunner()->GetLoop(),
            ui_mode_manufactor.GetLayoutTaskRunner()->GetLoop());
}

TEST_F(TaskRunnerManufactorTest, PartOnLayoutMode) {
  TaskRunnerManufactor part_on_layout_manufactor =
      TaskRunnerManufactor(PART_ON_LAYOUT, false, false);
  ASSERT_NE(part_on_layout_manufactor.GetLayoutTaskRunner()->GetLoop(),
            part_on_layout_manufactor.GetTASMTaskRunner()->GetLoop());
}

TEST_F(TaskRunnerManufactorTest, MostOnTASMMode) {
  TaskRunnerManufactor most_on_tasm_mode_manufactor =
      TaskRunnerManufactor(MOST_ON_TASM, false, false);
  ASSERT_EQ(most_on_tasm_mode_manufactor.GetLayoutTaskRunner()->GetLoop(),
            most_on_tasm_mode_manufactor.GetTASMTaskRunner()->GetLoop());
}

TEST_F(TaskRunnerManufactorTest, DefaultMultiThreadMode) {
  TaskRunnerManufactor multi_thread_mode_manufactor =
      TaskRunnerManufactor(MULTI_THREADS, false, false);
  ASSERT_NE(multi_thread_mode_manufactor.GetLayoutTaskRunner()->GetLoop(),
            multi_thread_mode_manufactor.GetTASMTaskRunner()->GetLoop());
}

TEST_F(TaskRunnerManufactorTest, MultiTASMThreadMode) {
  TaskRunnerManufactor multi_tasm_manufactor_1 =
      TaskRunnerManufactor(MULTI_THREADS, true, false);
  TaskRunnerManufactor multi_tasm_manufactor_2 =
      TaskRunnerManufactor(MULTI_THREADS, true, false);
  ASSERT_NE(multi_tasm_manufactor_1.GetTASMTaskRunner()->GetLoop(),
            multi_tasm_manufactor_2.GetTASMTaskRunner()->GetLoop());
  ASSERT_EQ(multi_tasm_manufactor_1.GetLayoutTaskRunner()->GetLoop(),
            multi_tasm_manufactor_2.GetLayoutTaskRunner()->GetLoop());
}

TEST_F(TaskRunnerManufactorTest, MultiLayoutThreadMode) {
  TaskRunnerManufactor multi_layout_manufactor_1 =
      TaskRunnerManufactor(MULTI_THREADS, false, true);
  TaskRunnerManufactor multi_layout_manufactor_2 =
      TaskRunnerManufactor(MULTI_THREADS, false, true);
  ASSERT_EQ(multi_layout_manufactor_1.GetTASMTaskRunner()->GetLoop(),
            multi_layout_manufactor_2.GetTASMTaskRunner()->GetLoop());
  ASSERT_NE(multi_layout_manufactor_1.GetLayoutTaskRunner()->GetLoop(),
            multi_layout_manufactor_2.GetLayoutTaskRunner()->GetLoop());
}

TEST_F(TaskRunnerManufactorTest, MultiLayoutThreadModeAndCache) {
  TaskRunnerManufactor multi_layout_manufactor_1 =
      TaskRunnerManufactor(MULTI_THREADS, false, true, false, true);
  TaskRunnerManufactor multi_layout_manufactor_2 =
      TaskRunnerManufactor(MULTI_THREADS, false, true, false, true);
  TaskRunnerManufactor multi_layout_manufactor_3 =
      TaskRunnerManufactor(MULTI_THREADS, false, true, false, true);
  TaskRunnerManufactor multi_layout_manufactor_4 =
      TaskRunnerManufactor(MULTI_THREADS, false, true, false, true);
  ASSERT_EQ(multi_layout_manufactor_1.GetTASMTaskRunner()->GetLoop(),
            multi_layout_manufactor_2.GetTASMTaskRunner()->GetLoop());
  ASSERT_NE(multi_layout_manufactor_1.GetLayoutTaskRunner()->GetLoop(),
            multi_layout_manufactor_2.GetLayoutTaskRunner()->GetLoop());
  ASSERT_NE(multi_layout_manufactor_1.GetLayoutTaskRunner()->GetLoop(),
            multi_layout_manufactor_3.GetLayoutTaskRunner()->GetLoop());
  ASSERT_EQ(multi_layout_manufactor_1.GetLayoutTaskRunner()->GetLoop(),
            multi_layout_manufactor_4.GetLayoutTaskRunner()->GetLoop());
}

TEST_F(TaskRunnerManufactorTest, MultiJSGroupThreadMode) {
  TaskRunnerManufactor single_js_thread =
      TaskRunnerManufactor(ALL_ON_UI, false, false, false, false, "");
  TaskRunnerManufactor multi_js_thread_1 =
      TaskRunnerManufactor(ALL_ON_UI, false, false, false, false, "Group1");
  TaskRunnerManufactor multi_js_thread_2 =
      TaskRunnerManufactor(ALL_ON_UI, false, false, false, false, "Group2");
  TaskRunnerManufactor multi_js_thread_3 =
      TaskRunnerManufactor(ALL_ON_UI, false, false, false, false, "Group3");
  TaskRunnerManufactor multi_js_thread_temp_1 =
      TaskRunnerManufactor(ALL_ON_UI, false, false, false, false, "Group1");
  ASSERT_NE(single_js_thread.GetJSTaskRunner()->GetLoop(),
            multi_js_thread_1.GetLayoutTaskRunner()->GetLoop());
  ASSERT_NE(single_js_thread.GetJSTaskRunner()->GetLoop(),
            multi_js_thread_2.GetLayoutTaskRunner()->GetLoop());
  ASSERT_NE(single_js_thread.GetJSTaskRunner()->GetLoop(),
            multi_js_thread_3.GetLayoutTaskRunner()->GetLoop());
  ASSERT_EQ(single_js_thread.GetJSTaskRunner()->GetLoop(),
            TaskRunnerManufactor::GetJSRunner("")->GetLoop());
  ASSERT_EQ(multi_js_thread_1.GetJSTaskRunner()->GetLoop(),
            TaskRunnerManufactor::GetJSRunner("Group1")->GetLoop());
  ASSERT_EQ(multi_js_thread_1.GetJSTaskRunner()->GetLoop(),
            multi_js_thread_temp_1.GetJSTaskRunner()->GetLoop());
}

}  // namespace base
}  // namespace lynx
