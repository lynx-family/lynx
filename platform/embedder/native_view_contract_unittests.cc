// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/embedder/interfaces/native_view.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::embedder {
namespace {
struct Completion {
  lynx_native_view_callback callback = nullptr;
  lynx_native_view_callback_info_t info{};
};
NativeView* State(lynx_native_view_t* handle) {
  return reinterpret_cast<NativeView*>(handle);
}
void BindMethod(lynx_native_view_t* handle) {
  lynx_native_view_bind_on_method_invoked(
      handle, [](lynx_native_view_t*, void* opaque, const char*, lynx_value,
                 lynx_native_view_callback callback,
                 lynx_native_view_callback_info_t info) {
        *static_cast<Completion*>(opaque) = {callback, info};
      });
}
TEST(NativeViewContract, LateAndDuplicateCompletionAfterRelease) {
  Completion completion;
  auto* handle = lynx_native_view_create(&completion);
  BindMethod(handle);
  lepus::Value value;
  pub::ValueImplLepus wrapped(value);
  int calls = 0;
  State(handle)->OnMethodInvoked("pending", wrapped,
                                 [&](int, const pub::Value&) { ++calls; });
  lynx_native_view_release(handle);
  completion.callback(completion.info, 0,
                      {.val_ptr = 0, .type = lynx_value_null});
  completion.callback(completion.info, 0,
                      {.val_ptr = 0, .type = lynx_value_null});
  EXPECT_EQ(calls, 0);
}
TEST(NativeViewContract, CompletionMayReenterAndRelease) {
  Completion completion;
  auto* handle = lynx_native_view_create(&completion);
  BindMethod(handle);
  lepus::Value value;
  pub::ValueImplLepus wrapped(value);
  int calls = 0;
  State(handle)->OnMethodInvoked(
      "pending", wrapped, [&](int code, const pub::Value&) {
        EXPECT_EQ(code, 7);
        ++calls;
        completion.callback(completion.info, 0,
                            {.val_ptr = 0, .type = lynx_value_null});
        lynx_native_view_release(handle);
      });
  completion.callback(completion.info, 7,
                      {.val_ptr = 0, .type = lynx_value_null});
  EXPECT_EQ(calls, 1);
}
TEST(NativeViewContract, DestroyCancelsMethodsWhileHostRetainsState) {
  Completion completion;
  auto* handle = lynx_native_view_create(&completion);
  BindMethod(handle);
  lepus::Value value;
  pub::ValueImplLepus wrapped(value);
  int calls = 0;
  State(handle)->OnMethodInvoked("pending", wrapped,
                                 [&](int, const pub::Value&) { ++calls; });
  State(handle)->OnDestroy();
  completion.callback(completion.info, 0,
                      {.val_ptr = 0, .type = lynx_value_null});
  EXPECT_EQ(calls, 0);
  lynx_native_view_release(handle);
}
TEST(NativeViewContract, CancelMayReenterAndReleaseCallbackOwnedView) {
  for (bool destroy_before_release : {false, true}) {
    SCOPED_TRACE(destroy_before_release);
    Completion completion;
    auto* handle = lynx_native_view_create(&completion);
    BindMethod(handle);
    int releases = 0;
    auto* owned_view = lynx_native_view_create(&releases);
    lynx_native_view_bind_release(
        owned_view, [](auto*, void* opaque) { ++*static_cast<int*>(opaque); });
    auto owner = std::shared_ptr<lynx_native_view_t>(
        owned_view, [&completion](lynx_native_view_t* view) {
          completion.callback(completion.info, 0,
                              {.val_ptr = 0, .type = lynx_value_null});
          lynx_native_view_release(view);
        });
    lepus::Value value;
    pub::ValueImplLepus wrapped(value);
    int calls = 0;
    State(handle)->OnMethodInvoked(
        "pending", wrapped,
        [owner, &calls](int, const pub::Value&) { ++calls; });
    owner.reset();
    EXPECT_EQ(releases, 0);
    if (destroy_before_release) {
      State(handle)->OnDestroy();
      EXPECT_EQ(releases, 1);
    }
    lynx_native_view_release(handle);
    EXPECT_EQ(releases, 1);
    EXPECT_EQ(calls, 0);
  }
}
TEST(NativeViewContract, UnboundSurfaceAndRetainRelease) {
  int releases = 0;
  auto* handle = lynx_native_view_create(&releases);
  lynx_native_view_bind_release(handle, [](lynx_native_view_t*, void* opaque) {
    ++*static_cast<int*>(opaque);
  });
  EXPECT_EQ(lynx_native_view_retain(handle), handle);
  EXPECT_EQ(lynx_native_view_acquire_surface(handle, 20, 20), nullptr);
  const float matrix[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  EXPECT_EQ(
      lynx_native_view_acquire_surface_with_transform(handle, 20, 20, matrix),
      nullptr);
  EXPECT_FALSE(
      lynx_native_view_present_surface(handle, 20, 20, matrix, nullptr));
  lynx_native_view_release(handle);
  EXPECT_EQ(releases, 0);
  lynx_native_view_release(handle);
  EXPECT_EQ(releases, 1);
}
}  // namespace
}  // namespace lynx::embedder
