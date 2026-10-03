// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/arena_manager.h"
#include "clay/ui/gesture/platform_view_gesture_recognizer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {
class DeferredMember : public ArenaMember {
 public:
  fml::WeakPtr<ArenaMember> weak() { return weak_factory_.GetWeakPtr(); }
  void OnGestureAccepted(int) override { ++accepted; }
  void OnGestureRejected(int) override { ++rejected; }
  const char* GetMemberTag() const override { return "deferred_test"; }
  int accepted = 0;
  int rejected = 0;
};

class DeferredArenaTest : public ::testing::Test {
 protected:
  void SetUp() override {
    native_entry = arena.Add(1, native.weak());
    native_entry->DeferToThisMember();
    parent_entry = arena.Add(1, parent.weak());
    PointerEvent down(PointerEvent::EventType::kDownEvent);
    down.pointer_id = 1;
    arena.Close(down);
  }
  ArenaManager arena;
  DeferredMember native;
  DeferredMember parent;
  std::unique_ptr<ArenaEntry> native_entry;
  std::unique_ptr<ArenaEntry> parent_entry;
};

TEST_F(DeferredArenaTest, NativeAcceptanceOverridesEarlierParentRequest) {
  parent_entry->Resolve(GestureDisposition::kAccept);
  EXPECT_EQ(parent.accepted, 0);
  native_entry->Resolve(GestureDisposition::kAccept);
  EXPECT_EQ(native.accepted, 1);
  EXPECT_EQ(parent.accepted, 0);
  EXPECT_EQ(parent.rejected, 1);
}

TEST_F(DeferredArenaTest, NativeRejectionCommitsWaitingParent) {
  parent_entry->Resolve(GestureDisposition::kAccept);
  native_entry->Resolve(GestureDisposition::kReject);
  EXPECT_EQ(parent.accepted, 1);
  EXPECT_EQ(native.rejected, 1);
}

TEST_F(DeferredArenaTest, WaitingParentCanWithdrawAcceptance) {
  EXPECT_TRUE(parent_entry->Resolve(GestureDisposition::kAccept));
  EXPECT_FALSE(parent_entry->Resolve(GestureDisposition::kReject));
  native_entry->Resolve(GestureDisposition::kReject);
  EXPECT_EQ(parent.accepted, 0);
  EXPECT_EQ(parent.rejected, 1);
  EXPECT_EQ(native.rejected, 1);
}

TEST_F(DeferredArenaTest, SweepWaitsForNativeDecision) {
  arena.Sweep(1);
  EXPECT_EQ(native.accepted, 0);
  EXPECT_EQ(parent.accepted, 0);
  native_entry->Resolve(GestureDisposition::kAccept);
  EXPECT_EQ(native.accepted, 1);
  EXPECT_EQ(parent.rejected, 1);
}

TEST_F(DeferredArenaTest, PendingNativeDoesNotWinByDefault) {
  parent_entry->Resolve(GestureDisposition::kReject);
  EXPECT_EQ(native.accepted, 0);
  native_entry->Resolve(GestureDisposition::kAccept);
  EXPECT_EQ(native.accepted, 1);
}

TEST_F(DeferredArenaTest, OtherPointerResolvesIndependently) {
  DeferredMember other;
  auto entry = arena.Add(2, other.weak());
  PointerEvent down(PointerEvent::EventType::kDownEvent);
  down.pointer_id = 2;
  arena.Close(down);
  EXPECT_EQ(other.accepted, 1);
  EXPECT_EQ(native.accepted, 0);
}

TEST(PlatformGesturePointerTest, AcceptanceRemovesPendingRecord) {
  ArenaManager arena;
  PlatformViewGestureRecognizer native(&arena);
  PointerEvent down(PointerEvent::EventType::kDownEvent);
  down.pointer_id = 1;
  native.AddPointer(down);
  DeferredMember parent;
  auto entry = arena.Add(1, parent.weak());
  arena.Close(down);
  ASSERT_TRUE(native.HasPendingPointer(1));
  entry->Resolve(GestureDisposition::kAccept);
  EXPECT_TRUE(native.UpdateDecision(1, GestureDisposition::kAccept));
  EXPECT_EQ(parent.rejected, 1);
  EXPECT_FALSE(native.UpdateDecision(1, GestureDisposition::kReject));
  EXPECT_FALSE(native.HasPendingPointer(1));
  EXPECT_EQ(parent.accepted, 0);
}

TEST(PlatformGesturePointerTest, LonePlatformIsNotActive) {
  ArenaManager arena;
  PlatformViewGestureRecognizer native(&arena);
  PointerEvent down(PointerEvent::EventType::kDownEvent);
  down.pointer_id = 1;
  native.AddPointer(down);
  EXPECT_FALSE(native.HasPendingPointer(1));
  arena.Close(down);
  EXPECT_FALSE(native.HasPendingPointer(1));
  arena.Sweep(1);
  // A subsequent Clay-only sequence must resolve normally.
  DeferredMember local;
  auto entry = arena.Add(1, local.weak());
  arena.Close(down);
  EXPECT_EQ(local.accepted, 1);
}

TEST(PlatformGesturePointerTest, PlatformRejectionReleasesDeferredSweep) {
  ArenaManager arena;
  PlatformViewGestureRecognizer native(&arena);
  PointerEvent down(PointerEvent::EventType::kDownEvent);
  down.pointer_id = 1;
  native.AddPointer(down);
  DeferredMember first;
  DeferredMember second;
  auto first_entry = arena.Add(1, first.weak());
  auto second_entry = arena.Add(1, second.weak());
  arena.Close(down);
  ASSERT_TRUE(native.HasPendingPointer(1));
  arena.Sweep(1);
  EXPECT_EQ(first.accepted, 0);
  EXPECT_TRUE(native.UpdateDecision(1, GestureDisposition::kReject));
  EXPECT_FALSE(native.HasPendingPointer(1));
  EXPECT_EQ(first.accepted, 1);
  EXPECT_EQ(second.rejected, 1);
  EXPECT_FALSE(native.UpdateDecision(1, GestureDisposition::kAccept));
}

TEST(PlatformGesturePointerTest, ClayAcceptanceBeforeCloseWaitsForPlatform) {
  ArenaManager arena;
  PlatformViewGestureRecognizer native(&arena);
  PointerEvent down(PointerEvent::EventType::kDownEvent);
  down.pointer_id = 1;
  native.AddPointer(down);
  DeferredMember local;
  auto entry = arena.Add(1, local.weak());
  entry->Resolve(GestureDisposition::kAccept);
  EXPECT_FALSE(native.HasPendingPointer(1));
  arena.Close(down);
  EXPECT_EQ(local.accepted, 0);
  ASSERT_TRUE(native.HasPendingPointer(1));
  EXPECT_TRUE(native.UpdateDecision(1, GestureDisposition::kReject));
  EXPECT_EQ(local.accepted, 1);
  EXPECT_FALSE(native.HasPendingPointer(1));
}

TEST(PlatformGesturePointerTest, CancelBeforeClosePreservesClayDecision) {
  ArenaManager arena;
  PlatformViewGestureRecognizer native(&arena);
  PointerEvent down(PointerEvent::EventType::kDownEvent);
  down.pointer_id = 1;
  native.AddPointer(down);
  DeferredMember local;
  auto entry = arena.Add(1, local.weak());
  native.CancelAll();
  arena.Close(down);
  EXPECT_EQ(local.accepted, 1);
  EXPECT_EQ(local.rejected, 0);
}

TEST(PlatformGesturePointerTest, DestructionWithdrawsPlatformCandidate) {
  ArenaManager arena;
  DeferredMember parent;
  {
    PlatformViewGestureRecognizer native(&arena);
    PointerEvent down(PointerEvent::EventType::kDownEvent);
    down.pointer_id = 1;
    native.AddPointer(down);
    auto entry = arena.Add(1, parent.weak());
    arena.Close(down);
    entry->Resolve(GestureDisposition::kAccept);
  }
  EXPECT_EQ(parent.accepted, 1);
  EXPECT_EQ(parent.rejected, 0);
}
}  // namespace
}  // namespace clay
