// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/element_context_delegate.h"

#include "base/include/concurrent_queue.h"
#include "core/renderer/dom/element_context_task_queue.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class ElementContextDelegateTest : public ::testing::Test {
 public:
  ElementContextDelegateTest(){};
  ~ElementContextDelegateTest() override{};

  void SetUp() override {
    element_context_delegate_ =
        std::make_shared<ElementContextDelegate>(nullptr, nullptr);
    element_context_delegate_->element_context_task_queue_ =
        std::make_unique<ElementContextTaskQueue>([]() { return true; });
  }

  std::shared_ptr<ElementContextDelegate> element_context_delegate_;
};

TEST_F(ElementContextDelegateTest, EnqueueTask) {
  element_context_delegate_->EnqueueTask([]() {});
  EXPECT_TRUE(!(element_context_delegate_->element_context_task_queue_
                    ->task_queue_.Empty()));
  element_context_delegate_->FlushEnqueuedTasks();
  EXPECT_TRUE(element_context_delegate_->element_context_task_queue_
                  ->task_queue_.Empty());
}

TEST_F(ElementContextDelegateTest, OnChildElementContextAdded) {
  std::shared_ptr<ElementContextDelegate> child_element_context =
      std::make_shared<ElementContextDelegate>(element_context_delegate_.get(),
                                               nullptr);
  element_context_delegate_->OnChildElementContextAdded(child_element_context);
  EXPECT_TRUE(
      !(element_context_delegate_->scoped_children_element_contexts_.empty()));
}

TEST_F(ElementContextDelegateTest, RemoveSelf) {
  std::shared_ptr<ElementContextDelegate> child_element_context =
      std::make_shared<ElementContextDelegate>(element_context_delegate_.get(),
                                               nullptr);
  auto child_element_context_ptr = child_element_context.get();
  element_context_delegate_->OnChildElementContextAdded(child_element_context);
  std::shared_ptr<ElementContextDelegate> grandchild_element_context =
      std::make_shared<ElementContextDelegate>(child_element_context_ptr,
                                               nullptr);
  auto grandchild_element_context_ptr = grandchild_element_context.get();
  child_element_context_ptr->OnChildElementContextAdded(
      grandchild_element_context);
  child_element_context_ptr->RemoveSelf();
  EXPECT_TRUE(
      element_context_delegate_->scoped_children_element_contexts_.size() == 1);
  EXPECT_TRUE(
      element_context_delegate_->scoped_children_element_contexts_[0].get() ==
      grandchild_element_context_ptr);
}

TEST_F(ElementContextDelegateTest, TestTaskFlushOrder) {
  std::shared_ptr<ElementContextDelegate> child_element_context =
      std::make_shared<ElementContextDelegate>(element_context_delegate_.get(),
                                               nullptr);
  child_element_context->element_context_task_queue_ =
      std::make_unique<ElementContextTaskQueue>([]() { return true; });
  auto child_element_context_ptr = child_element_context.get();
  element_context_delegate_->OnChildElementContextAdded(child_element_context);

  int order = 0;
  element_context_delegate_->EnqueueTask([&order]() { order = 1; });
  child_element_context_ptr->EnqueueTask([&order]() { order = 2; });
  element_context_delegate_->FlushEnqueuedTasks();
  EXPECT_TRUE(order == 1);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
