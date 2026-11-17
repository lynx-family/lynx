// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

// Include dependencies before exposing ET private members so the private macro
// does not affect dependency types.
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/vector.h"
#include "base/trace/native/trace_event.h"
#include "core/base/thread/once_task.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/generated_elements_result.h"
#define private public
#include "core/renderer/dom/fiber/element_template_instance.h"
#undef private

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/list_element_template_reuse_pool.h"
#include "core/renderer/dom/fiber/list_update_info_recorder.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/template_assembler.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

// Dependencies are already loaded. Expose only the List ET fixture state to
// exercise the production visitor entry point directly.
#define private public
#include "core/renderer/dom/fiber/list_element_template_instance.h"
#undef private

namespace lynx {
namespace tasm {
namespace testing {
namespace {

using Instance = fml::RefPtr<ElementTemplateInstance>;
using Action = ListElementTemplateReusePool::Action;

class TrackingElementTemplateInstance final : public ElementTemplateInstance {
 public:
  explicit TrackingElementTemplateInstance(ElementManager* manager)
      : ElementTemplateInstance(manager) {}

  void visitor(void* rt, void* mark_func, uint64_t trace_tool) override {
    ++visit_count_;
    last_runtime_ = rt;
    last_mark_func_ = mark_func;
    last_trace_tool_ = trace_tool;
  }

  int visit_count_{0};
  void* last_runtime_{nullptr};
  void* last_mark_func_{nullptr};
  uint64_t last_trace_tool_{0};
};

}  // namespace

class ListElementTemplateReusePoolTest : public ::testing::Test {
 protected:
  void SetUp() override {
    LynxEnvConfig env_config(1080, 1920, 1.f, 1.f);
    auto manager = std::make_unique<ElementManager>(
        std::make_unique<MockPaintingContext>(), &tasm_delegate_, env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    manager->SetConfig(config);
    manager_ = manager.get();
    tasm_ = std::make_shared<TemplateAssembler>(
        tasm_delegate_, std::move(manager), &tasm_delegate_, 0);
  }

  Instance CreateUnmaterializedItem(int uid, const char* type = "Card",
                                    const char* bundle = "__Card__") {
    auto item = fml::AdoptRef(new ElementTemplateInstance(manager_));
    item->SetTemplateKey(base::String(type));
    item->SetBundleUrl(base::String(bundle));
    item->SetUid(lepus::Value(uid));
    return item;
  }

  Instance CreateMaterializedItem(int uid, const char* type = "Card",
                                  const char* bundle = "__Card__") {
    auto item = CreateUnmaterializedItem(uid, type, bundle);
    // The pool only checks the physical root and sign. Supply a root directly
    // to isolate template decoding and page materialization.
    // Later transfer and invalidation scenarios also change bindings only in
    // the fixture, without adding production APIs for testing.
    item->result_ = manager_->CreateFiberView();
    return item;
  }

  fml::RefPtr<TrackingElementTemplateInstance> CreateTrackingItem(int uid) {
    auto item = fml::AdoptRef(new TrackingElementTemplateInstance(manager_));
    item->SetTemplateKey(base::String("Card"));
    item->SetUid(lepus::Value(uid));
    item->result_ = manager_->CreateFiberView();
    return item;
  }

  void SetChildren(const Instance& parent,
                   std::initializer_list<Instance> children) {
    auto slot = lepus::CArray::Create();
    for (const auto& child : children) {
      slot->emplace_back(lepus::Value(child));
    }
    auto slots = lepus::CArray::Create();
    slots->emplace_back(lepus::Value(std::move(slot)));
    parent->InitializeChildSlots(lepus::Value(std::move(slots)));
  }

  Instance CreateParent(std::initializer_list<Instance> children) {
    auto parent = CreateUnmaterializedItem(0, "Parent");
    // Establish only logical relationships, without calling GetRoot() or
    // mounting physical subtrees.
    SetChildren(parent, children);
    return parent;
  }

  void SetReuseIdentifier(const Instance& item, const lepus::Value& value) {
    auto info = lepus::Dictionary::Create();
    info->SetValue("reuse-identifier", value);
    item->SetListItemPlatformInfo(lepus::Value(std::move(info)));
  }

  void ExpectCreate(const Action& action) const {
    EXPECT_EQ(action.type_, Action::Type::kCreate);
    EXPECT_EQ(action.reusable_instance_, nullptr);
    EXPECT_EQ(action.reusable_root_, nullptr);
  }

  void ExpectUnavailable(const Action& action) const {
    EXPECT_EQ(action.type_, Action::Type::kUnavailable);
    EXPECT_EQ(action.reusable_instance_, nullptr);
    EXPECT_EQ(action.reusable_root_, nullptr);
  }

  void ExpectUseExisting(const Action& action, const Instance& item) const {
    EXPECT_EQ(action.type_, Action::Type::kUseExisting);
    ASSERT_NE(action.reusable_root_, nullptr);
    EXPECT_EQ(action.reusable_instance_, item);
    EXPECT_EQ(action.reusable_root_, item->PeekMaterializedRoot());
  }

  void ExpectReuse(const Action& action, int32_t sign,
                   const Instance& source) const {
    EXPECT_EQ(action.type_, Action::Type::kReuse);
    EXPECT_EQ(action.reusable_instance_, source);
    ASSERT_NE(action.reusable_root_, nullptr);
    EXPECT_EQ(action.reusable_root_->impl_id(), sign);
    EXPECT_EQ(action.reusable_root_, source->PeekMaterializedRoot());
  }

  // Members are destroyed in reverse declaration order: release pooled ETs
  // before the TASM that owns ElementManager.
  ::testing::NiceMock<test::MockTasmDelegate> tasm_delegate_;
  std::shared_ptr<TemplateAssembler> tasm_;
  ElementManager* manager_{nullptr};
  ListElementTemplateReusePool pool_;
};

// Basic registration and recycling.
TEST_F(ListElementTemplateReusePoolTest, InvalidRequestsPreserveCandidates) {
  // Purpose: Invalid signs and null requests must neither materialize targets
  // nor consume existing candidates.
  // Input: Look up and enqueue an unknown sign in an empty pool, then register
  // and enqueue source before making a null request.
  // Expected: Unknown signs return nullptr; an empty pool returns kCreate
  // without materializing the target. A null request returns kUnavailable and
  // leaves source reusable.
  auto target = CreateUnmaterializedItem(1);
  EXPECT_EQ(pool_.GetInstance(-1), nullptr);
  EXPECT_EQ(pool_.GetInstance(12345), nullptr);
  EXPECT_EQ(pool_.Enqueue(12345), nullptr);
  ExpectCreate(pool_.Dequeue(target));
  EXPECT_EQ(target->PeekMaterializedRoot(), nullptr);

  // A null request must not consume an existing candidate.
  auto source = CreateMaterializedItem(2);
  const auto sign = source->PeekMaterializedRoot()->impl_id();
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, source));
  ASSERT_EQ(pool_.Enqueue(sign), source);
  ExpectUnavailable(pool_.Dequeue(nullptr));
  ExpectReuse(pool_.Dequeue(target), sign, source);
}

TEST_F(ListElementTemplateReusePoolTest, RegistrationRejectsInvalidBindings) {
  // Purpose: Registration alone does not enable recycling. Invalid bindings
  // must not replace the owner or remove existing reuse eligibility.
  // Input: Register A and query before enqueuing it. Then enqueue A and try
  // null, an unmaterialized B, C with a different root, and an unknown sign.
  // Expected: Registration alone yields kCreate. All invalid bindings fail, A
  // remains the owner and can still be selected, and B stays unmaterialized.
  auto a = CreateMaterializedItem(1);
  auto b = CreateUnmaterializedItem(2);
  auto c = CreateMaterializedItem(3);
  const auto sign = a->PeekMaterializedRoot()->impl_id();
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, a));
  EXPECT_EQ(pool_.GetInstance(sign), a);
  ExpectCreate(pool_.Dequeue(b));
  pool_.Enqueue(sign);

  EXPECT_FALSE(pool_.RegisterNewBinding(sign, nullptr));
  EXPECT_FALSE(pool_.RegisterNewBinding(sign, b));
  EXPECT_FALSE(pool_.RegisterNewBinding(sign, c));
  EXPECT_FALSE(pool_.RegisterNewBinding(-1, c));
  EXPECT_EQ(pool_.GetInstance(sign), a);
  EXPECT_EQ(pool_.GetInstance(c->PeekMaterializedRoot()->impl_id()), nullptr);
  EXPECT_EQ(b->PeekMaterializedRoot(), nullptr);
  ExpectReuse(pool_.Dequeue(b), sign, a);
}

TEST_F(ListElementTemplateReusePoolTest,
       DequeueOnlyReservesWithoutTransferringTree) {
  // Purpose: Dequeue only reserves a source; it must not transfer the physical
  // tree or change UIDs or logical relationships.
  // Input: Give A a logical parent, a child ET, and a physical parent. Register
  // and enqueue A, then dequeue for an unmaterialized B.
  // Expected: Action returns A and its root. A's root, UID, relationships, and
  // serialized state remain unchanged, and B stays unmaterialized.
  auto a = CreateUnmaterializedItem(1);
  auto child = CreateUnmaterializedItem(2);
  SetChildren(a, {child});
  a->result_ = manager_->CreateFiberView();
  auto parent = CreateParent({a});
  auto root = a->PeekMaterializedRoot();
  auto physical_parent = manager_->CreateFiberView();
  physical_parent->InsertNode(root);
  auto before = a->Serialize();
  auto target = CreateUnmaterializedItem(3);
  ASSERT_TRUE(
      pool_.RegisterNewBinding(a->PeekMaterializedRoot()->impl_id(), a));
  pool_.Enqueue(a->PeekMaterializedRoot()->impl_id());

  auto action = pool_.Dequeue(target);

  ExpectReuse(action, a->PeekMaterializedRoot()->impl_id(), a);
  EXPECT_EQ(pool_.GetInstance(a->PeekMaterializedRoot()->impl_id()), a);
  EXPECT_EQ(a->PeekMaterializedRoot(), root);
  EXPECT_EQ(root->parent(), physical_parent.get());
  EXPECT_EQ(a->GetLogicalParent(), parent.get());
  EXPECT_EQ(child->GetLogicalParent(), a.get());
  EXPECT_EQ(a->Serialize(), before);
  EXPECT_EQ(target->PeekMaterializedRoot(), nullptr);
}

TEST_F(ListElementTemplateReusePoolTest,
       RepeatedEnqueuePreservesFirstInFirstOutOrder) {
  // Purpose: Repeated enqueues of a sign must not duplicate its entry or change
  // its original FIFO position.
  // Input: Register A, B, and C, enqueue them in order A, B, A, C, B, then
  // dequeue repeatedly.
  // Expected: Each enqueue returns its owner. Dequeue selects A, B, and C once
  // each, then returns kCreate.
  auto a = CreateMaterializedItem(1);
  auto b = CreateMaterializedItem(2);
  auto c = CreateMaterializedItem(3);
  auto target = CreateUnmaterializedItem(4);
  for (const auto& item : {a, b, c}) {
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
  }
  for (const auto& item : {a, b, a, c, b}) {
    // Repeated enqueues must still return the owner so the caller can send the
    // enqueue callback correctly.
    EXPECT_EQ(pool_.Enqueue(item->PeekMaterializedRoot()->impl_id()), item);
  }

  for (const auto& item : {a, b, c}) {
    ExpectReuse(pool_.Dequeue(target), item->PeekMaterializedRoot()->impl_id(),
                item);
  }
  ExpectCreate(pool_.Dequeue(target));
}

// Template identity.
TEST_F(ListElementTemplateReusePoolTest,
       RecycledItemsAreGroupedByTemplateType) {
  // Purpose: Candidates with different template types use separate buckets;
  // querying one must not consume another.
  // Input: Enqueue an Other item A before a Card item B, then query Card and
  // Other targets.
  // Expected: The Card target selects B, and the Other target can still select
  // A.
  auto a = CreateMaterializedItem(1, "Other");
  auto b = CreateMaterializedItem(2, "Card");
  auto card = CreateUnmaterializedItem(3, "Card");
  auto other = CreateUnmaterializedItem(4, "Other");
  for (const auto& item : {a, b}) {
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
    pool_.Enqueue(item->PeekMaterializedRoot()->impl_id());
  }

  ExpectReuse(pool_.Dequeue(card), b->PeekMaterializedRoot()->impl_id(), b);
  ExpectReuse(pool_.Dequeue(other), a->PeekMaterializedRoot()->impl_id(), a);
}

TEST_F(ListElementTemplateReusePoolTest, MainBundleAliasesShareTemplateType) {
  // Purpose: An empty bundle and the main-bundle sentinel share template
  // identity; UIDs do not affect reuse grouping.
  // Input: Enqueue a Card item A with an empty bundle, then query a Card item B
  // with the __Card__ bundle and a different UID.
  // Expected: B can reuse A, and the query does not materialize B.
  auto a = CreateMaterializedItem(1, "Card", "");
  auto b = CreateUnmaterializedItem(2, "Card", "__Card__");
  ASSERT_TRUE(
      pool_.RegisterNewBinding(a->PeekMaterializedRoot()->impl_id(), a));
  pool_.Enqueue(a->PeekMaterializedRoot()->impl_id());

  ExpectReuse(pool_.Dequeue(b), a->PeekMaterializedRoot()->impl_id(), a);
  EXPECT_EQ(b->PeekMaterializedRoot(), nullptr);
}

TEST_F(ListElementTemplateReusePoolTest,
       ExternalBundlesDoNotShareTemplateType) {
  // Purpose: Matching templateKey values must not allow reuse across external
  // bundles or between external and main bundles.
  // Input: Enqueue a Card from bundle-A, then query Card targets from bundle-B,
  // the main bundle, and bundle-A.
  // Expected: The first two queries return kCreate; only the bundle-A target
  // selects the original candidate.
  auto source = CreateMaterializedItem(1, "Card", "bundle-A");
  auto other = CreateUnmaterializedItem(2, "Card", "bundle-B");
  auto main = CreateUnmaterializedItem(3, "Card", "__Card__");
  auto matching = CreateUnmaterializedItem(4, "Card", "bundle-A");
  ASSERT_TRUE(pool_.RegisterNewBinding(
      source->PeekMaterializedRoot()->impl_id(), source));
  pool_.Enqueue(source->PeekMaterializedRoot()->impl_id());

  ExpectCreate(pool_.Dequeue(other));
  ExpectCreate(pool_.Dequeue(main));
  ExpectReuse(pool_.Dequeue(matching),
              source->PeekMaterializedRoot()->impl_id(), source);
}

TEST_F(ListElementTemplateReusePoolTest,
       TypedItemsUseTagInsteadOfCompiledIdentity) {
  // Purpose: A typed ET uses its tag as template identity, ignoring its
  // compiled template key and bundle.
  // Input: Enqueue a view source, then query a view target with different
  // key/bundle values and a text target.
  // Expected: The text target returns kCreate. The view target selects the
  // source regardless of key/bundle differences.
  auto source = CreateMaterializedItem(1, "Ignored-A", "bundle-A");
  source->SetTypedTag(base::String("view"));
  auto matching = CreateUnmaterializedItem(2, "Ignored-B", "bundle-B");
  matching->SetTypedTag(base::String("view"));
  auto other = CreateUnmaterializedItem(3);
  other->SetTypedTag(base::String("text"));
  ASSERT_TRUE(pool_.RegisterNewBinding(
      source->PeekMaterializedRoot()->impl_id(), source));
  pool_.Enqueue(source->PeekMaterializedRoot()->impl_id());

  ExpectCreate(pool_.Dequeue(other));
  ExpectReuse(pool_.Dequeue(matching),
              source->PeekMaterializedRoot()->impl_id(), source);
}

// Selection, reservations, and invalidation.
TEST_F(ListElementTemplateReusePoolTest,
       DequeuePrefersOwnRootAndRemovesItsRecyclingEligibility) {
  // Purpose: An existing root takes precedence over pooled candidates. Taking
  // its own root revokes only that item's reuse eligibility while preserving
  // registration.
  // Input: Enqueue A and B in the shared bucket. Query an unregistered target
  // with a root, take B's own root, then enqueue B again.
  // Expected: The unregistered target uses its root without becoming
  // registered. A remains available, and B becomes reusable by other targets
  // only after another enqueue. Taking B's root must not reserve the FIFO head
  // A.
  auto a = CreateMaterializedItem(1);
  auto b = CreateMaterializedItem(2);
  auto target = CreateUnmaterializedItem(3);
  auto unregistered = CreateMaterializedItem(4);
  for (const auto& item : {a, b, target, unregistered}) {
    SetReuseIdentifier(item, lepus::Value("shared"));
  }
  for (const auto& item : {a, b}) {
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
    ASSERT_EQ(pool_.Enqueue(item->PeekMaterializedRoot()->impl_id()), item);
  }

  ExpectUseExisting(pool_.Dequeue(unregistered), unregistered);
  EXPECT_EQ(pool_.GetInstance(unregistered->PeekMaterializedRoot()->impl_id()),
            nullptr);
  ExpectUseExisting(pool_.Dequeue(b), b);
  ExpectReuse(pool_.Dequeue(target), a->PeekMaterializedRoot()->impl_id(), a);
  ExpectCreate(pool_.Dequeue(target));
  EXPECT_EQ(pool_.GetInstance(b->PeekMaterializedRoot()->impl_id()), b);

  // After using its own root, B must be explicitly enqueued again before other
  // targets can reuse it.
  ASSERT_EQ(pool_.Enqueue(b->PeekMaterializedRoot()->impl_id()), b);
  ExpectReuse(pool_.Dequeue(target), b->PeekMaterializedRoot()->impl_id(), b);
}

TEST_F(ListElementTemplateReusePoolTest,
       ReservationsRejectReenqueueAndRestoreFifo) {
  // Purpose: Repeated enqueues must not unlock reservations. Cancelling in
  // reverse order must preserve FIFO and leave other reservations intact.
  // Input: Enqueue A, B, and C in the shared bucket. Reserve A and B, enqueue A
  // again, select C, cancel B then A, and finally cancel C.
  // Expected: The repeated enqueue returns nullptr and only C remains
  // selectable. After cancellation, A precedes B; C stays reserved until
  // separately cancelled.
  auto a = CreateMaterializedItem(1);
  auto b = CreateMaterializedItem(2);
  auto c = CreateMaterializedItem(3);
  auto target = CreateUnmaterializedItem(4);
  for (const auto& item : {a, b, c, target}) {
    SetReuseIdentifier(item, lepus::Value("shared"));
  }
  for (const auto& item : {a, b, c}) {
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
    ASSERT_EQ(pool_.Enqueue(item->PeekMaterializedRoot()->impl_id()), item);
  }
  auto action_a = pool_.Dequeue(target);
  auto action_b = pool_.Dequeue(target);
  ExpectReuse(action_a, a->PeekMaterializedRoot()->impl_id(), a);
  ExpectReuse(action_b, b->PeekMaterializedRoot()->impl_id(), b);

  // Repeated Enqueue must return nullptr and keep A/B reserved, leaving only C
  // available.
  EXPECT_EQ(pool_.Enqueue(a->PeekMaterializedRoot()->impl_id()), nullptr);
  auto action_c = pool_.Dequeue(target);
  ExpectReuse(action_c, c->PeekMaterializedRoot()->impl_id(), c);
  ExpectCreate(pool_.Dequeue(target));

  // Reverse cancellation preserves the original FIFO order and leaves C
  // reserved.
  pool_.CancelDequeue(action_b);
  pool_.CancelDequeue(action_a);
  for (const auto& item : {a, b}) {
    ExpectReuse(pool_.Dequeue(target), item->PeekMaterializedRoot()->impl_id(),
                item);
  }
  ExpectCreate(pool_.Dequeue(target));
  pool_.CancelDequeue(action_c);
  ExpectReuse(pool_.Dequeue(target), c->PeekMaterializedRoot()->impl_id(), c);
}

TEST_F(ListElementTemplateReusePoolTest,
       NonReuseOrIncompleteActionsCannotUnlockReservation) {
  // Purpose: Non-reuse actions and actions missing required fields must not
  // unlock existing reservations.
  // Input: Reserve A, then cancel a default Action, non-kReuse actions, and
  // actions missing the source instance or root.
  // Expected: Invalid cancellations leave no available candidate. Only
  // cancelling the original valid Action makes A available again.
  // Only exercise invalid actions explicitly handled by the API; do not forge
  // another valid reservation with the same key/sign.
  auto a = CreateMaterializedItem(1);
  auto target = CreateUnmaterializedItem(2);
  ASSERT_TRUE(
      pool_.RegisterNewBinding(a->PeekMaterializedRoot()->impl_id(), a));
  ASSERT_EQ(pool_.Enqueue(a->PeekMaterializedRoot()->impl_id()), a);
  auto action = pool_.Dequeue(target);
  ExpectReuse(action, a->PeekMaterializedRoot()->impl_id(), a);

  pool_.CancelDequeue(Action{});
  for (const auto type : {Action::Type::kCreate, Action::Type::kUseExisting,
                          Action::Type::kUnavailable}) {
    auto invalid = action;
    invalid.type_ = type;
    pool_.CancelDequeue(invalid);
  }
  auto invalid = action;
  invalid.reusable_instance_ = nullptr;
  pool_.CancelDequeue(invalid);
  invalid = action;
  invalid.reusable_root_ = nullptr;
  pool_.CancelDequeue(invalid);
  ExpectCreate(pool_.Dequeue(target));

  pool_.CancelDequeue(action);
  ExpectReuse(pool_.Dequeue(target), a->PeekMaterializedRoot()->impl_id(), a);
}

TEST_F(ListElementTemplateReusePoolTest,
       RegistrationRevokesAvailabilityButPreservesReservations) {
  // Purpose: Re-registering an available owner revokes reuse eligibility, but
  // re-registration must not unlock a reservation.
  // Input: Enqueue A/B, re-register A, enqueue and reserve A again, then try
  // registering that owner before cancelling the reservation.
  // Expected: The first registration succeeds and only B can be selected
  // initially. Registration while reserved fails without changing the owner,
  // and cancellation restores A's availability.
  auto a = CreateMaterializedItem(1);
  auto b = CreateMaterializedItem(2);
  auto target = CreateUnmaterializedItem(3);
  const auto sign = a->PeekMaterializedRoot()->impl_id();
  for (const auto& item : {a, b}) {
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
    ASSERT_EQ(pool_.Enqueue(item->PeekMaterializedRoot()->impl_id()), item);
  }

  // Re-registering an available owner revokes eligibility; another Enqueue is
  // required before selection.
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, a));
  ExpectReuse(pool_.Dequeue(target), b->PeekMaterializedRoot()->impl_id(), b);
  ExpectCreate(pool_.Dequeue(target));
  ASSERT_EQ(pool_.Enqueue(sign), a);
  auto action = pool_.Dequeue(target);
  ExpectReuse(action, sign, a);

  // Reject re-registration while reserved so it cannot unlock the candidate.
  EXPECT_FALSE(pool_.RegisterNewBinding(sign, a));
  EXPECT_EQ(pool_.GetInstance(sign), a);
  ExpectCreate(pool_.Dequeue(target));
  pool_.CancelDequeue(action);
  ExpectReuse(pool_.Dequeue(target), sign, a);
}

TEST_F(ListElementTemplateReusePoolTest,
       TransferRebindsOwnerAndRejectsOldCancellation) {
  // Purpose: Registration must follow physical root ownership after transfer,
  // and an old Action must not restore the old source's reuse eligibility.
  // Input: Reserve source's root, transfer and register it to new_owner, then
  // cancel the old Action with either an empty old bucket or another candidate
  // remaining.
  // Expected: Registration always points to new_owner. The old candidate does
  // not reappear, other candidates remain selectable, and another enqueue
  // returns new_owner.
  for (bool keep_bucket : {false, true}) {
    SCOPED_TRACE(keep_bucket);
    auto source = CreateMaterializedItem(1);
    auto remaining = CreateMaterializedItem(2);
    auto new_owner = CreateUnmaterializedItem(3);
    const auto sign = source->PeekMaterializedRoot()->impl_id();
    ASSERT_TRUE(pool_.RegisterNewBinding(sign, source));
    ASSERT_EQ(pool_.Enqueue(sign), source);
    if (keep_bucket) {
      const auto remaining_sign = remaining->PeekMaterializedRoot()->impl_id();
      ASSERT_TRUE(pool_.RegisterNewBinding(remaining_sign, remaining));
      ASSERT_EQ(pool_.Enqueue(remaining_sign), remaining);
    }
    auto action = pool_.Dequeue(new_owner);
    ExpectReuse(action, sign, source);
    new_owner->result_ = std::move(source->result_);
    ASSERT_TRUE(pool_.RegisterNewBinding(sign, new_owner));

    // Exercise cancellation both after the old bucket disappears and while
    // another candidate keeps it present.
    pool_.CancelDequeue(action);
    EXPECT_EQ(pool_.GetInstance(sign), new_owner);
    EXPECT_EQ(source->PeekMaterializedRoot(), nullptr);
    if (keep_bucket) {
      ExpectReuse(pool_.Dequeue(source),
                  remaining->PeekMaterializedRoot()->impl_id(), remaining);
    }
    ExpectCreate(pool_.Dequeue(source));
    ASSERT_EQ(pool_.Enqueue(sign), new_owner);
    ExpectReuse(pool_.Dequeue(source), sign, new_owner);
    pool_.Clear();
  }
}

TEST_F(ListElementTemplateReusePoolTest,
       EnqueueIgnoresOwnersWithMissingOrDifferentRoot) {
  // Purpose: Only an owner still holding the physical root for its registered
  // sign can be enqueued.
  // Input: Register A/B, clear A's root and replace B's root, enqueue their old
  // signs, then restore the original roots and query.
  // Expected: Both invalid enqueues return nullptr. Restoring the roots leaves
  // no candidates, proving that invalid enqueues created no entries.
  auto a = CreateMaterializedItem(1);
  auto b = CreateMaterializedItem(2);
  auto root_a = a->PeekMaterializedRoot();
  auto root_b = b->PeekMaterializedRoot();
  ASSERT_TRUE(pool_.RegisterNewBinding(root_a->impl_id(), a));
  ASSERT_TRUE(pool_.RegisterNewBinding(root_b->impl_id(), b));
  a->result_ = nullptr;
  b->result_ = manager_->CreateFiberView();

  EXPECT_EQ(pool_.Enqueue(root_a->impl_id()), nullptr);
  EXPECT_EQ(pool_.Enqueue(root_b->impl_id()), nullptr);
  a->result_ = root_a;
  b->result_ = root_b;

  auto target = CreateUnmaterializedItem(3);
  ExpectCreate(pool_.Dequeue(target));
}

TEST_F(ListElementTemplateReusePoolTest,
       DequeueRemovesInvalidCandidatesBeforeNextAvailableItem) {
  // Purpose: Candidates with missing roots, changed roots, or changed template
  // types must not block later candidates; their eligibility must be removed.
  // Input: Enqueue four candidates, then clear the first root, replace the
  // second with another non-null root, and change the third type. Leave the
  // fourth valid.
  // Expected: Only the fourth candidate is selected. Restoring the first three
  // roots and types still yields kCreate.
  auto missing_root = CreateMaterializedItem(1);
  auto changed_root = CreateMaterializedItem(2);
  auto changed_type = CreateMaterializedItem(3);
  auto valid = CreateMaterializedItem(4);
  auto target = CreateUnmaterializedItem(5);
  auto root_a = missing_root->PeekMaterializedRoot();
  auto root_b = changed_root->PeekMaterializedRoot();
  for (const auto& item : {missing_root, changed_root, changed_type, valid}) {
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
    ASSERT_EQ(pool_.Enqueue(item->PeekMaterializedRoot()->impl_id()), item);
  }
  missing_root->result_ = nullptr;
  changed_root->result_ = manager_->CreateFiberView();
  changed_type->SetTemplateKey(base::String("Other"));

  ExpectReuse(pool_.Dequeue(target), valid->PeekMaterializedRoot()->impl_id(),
              valid);
  // Invalid eligibility has been removed; restoring the original state must not
  // enqueue items automatically.
  missing_root->result_ = root_a;
  changed_root->result_ = root_b;
  changed_type->SetTemplateKey(base::String("Card"));
  ExpectCreate(pool_.Dequeue(target));
}

TEST_F(ListElementTemplateReusePoolTest,
       DequeuePrunesLostRootAfterCancellation) {
  // Purpose: Verify invalid candidate cleanup after cancelling a source that
  // lost its root, while preserving owner registration.
  // Input: Enqueue A/B, reserve A, clear its root and cancel the reservation,
  // query once, then restore A's root and query again.
  // Expected: The first query skips A and selects B. Restoring the root does
  // not restore A's old eligibility, but its owner registration remains.
  // CancelDequeue only restores the reservation flag; the next Dequeue
  // validates and removes the invalid root entry.
  auto a = CreateMaterializedItem(1);
  auto b = CreateMaterializedItem(2);
  auto root = a->PeekMaterializedRoot();
  auto target = CreateUnmaterializedItem(3);
  for (const auto& item : {a, b}) {
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
    ASSERT_EQ(pool_.Enqueue(item->PeekMaterializedRoot()->impl_id()), item);
  }
  auto action = pool_.Dequeue(target);
  ExpectReuse(action, root->impl_id(), a);

  a->result_ = nullptr;
  pool_.CancelDequeue(action);
  ExpectReuse(pool_.Dequeue(target), b->PeekMaterializedRoot()->impl_id(), b);

  // The invalid source entry has been removed; restoring its root must not
  // restore reuse eligibility automatically.
  a->result_ = root;
  ExpectCreate(pool_.Dequeue(target));
  EXPECT_EQ(pool_.GetInstance(root->impl_id()), a);
}

TEST_F(ListElementTemplateReusePoolTest,
       SeparatePoolsKeepIndependentEligibility) {
  // Purpose: Separate pools maintain independent owner registrations and reuse
  // eligibility.
  // Input: Register and enqueue A/B in separate pools, clear the first pool,
  // then query both.
  // Expected: The first pool returns kCreate. The second cannot look up A's
  // sign but can still reuse B.
  ListElementTemplateReusePool other_pool;
  auto a = CreateMaterializedItem(1);
  auto b = CreateMaterializedItem(2);
  auto target = CreateUnmaterializedItem(3);
  ASSERT_TRUE(
      pool_.RegisterNewBinding(a->PeekMaterializedRoot()->impl_id(), a));
  ASSERT_TRUE(
      other_pool.RegisterNewBinding(b->PeekMaterializedRoot()->impl_id(), b));
  pool_.Enqueue(a->PeekMaterializedRoot()->impl_id());
  other_pool.Enqueue(b->PeekMaterializedRoot()->impl_id());

  pool_.Clear();

  ExpectCreate(pool_.Dequeue(target));
  EXPECT_EQ(other_pool.GetInstance(a->PeekMaterializedRoot()->impl_id()),
            nullptr);
  ExpectReuse(other_pool.Dequeue(target), b->PeekMaterializedRoot()->impl_id(),
              b);
}

TEST_F(ListElementTemplateReusePoolTest,
       MovingPoolPreservesFifoAndOutstandingReservation) {
  // Purpose: Moving a pool by construction or assignment must preserve
  // registrations, FIFO order, and outstanding reservations.
  // Input: Enqueue A/B and reserve A, move-construct then move-assign the pool,
  // and cancel A in the destination.
  // Expected: The destination can still look up A/B and selects them in their
  // original order, then returns kCreate.
  auto a = CreateMaterializedItem(1);
  auto b = CreateMaterializedItem(2);
  auto target = CreateUnmaterializedItem(3);
  for (const auto& item : {a, b}) {
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
    pool_.Enqueue(item->PeekMaterializedRoot()->impl_id());
  }
  auto action = pool_.Dequeue(target);
  ExpectReuse(action, a->PeekMaterializedRoot()->impl_id(), a);

  ListElementTemplateReusePool moved(std::move(pool_));
  ListElementTemplateReusePool destination;
  destination = std::move(moved);
  EXPECT_EQ(destination.GetInstance(a->PeekMaterializedRoot()->impl_id()), a);
  EXPECT_EQ(destination.GetInstance(b->PeekMaterializedRoot()->impl_id()), b);
  destination.CancelDequeue(action);

  ExpectReuse(destination.Dequeue(target), a->PeekMaterializedRoot()->impl_id(),
              a);
  ExpectReuse(destination.Dequeue(target), b->PeekMaterializedRoot()->impl_id(),
              b);
  ExpectCreate(destination.Dequeue(target));
}

TEST_F(ListElementTemplateReusePoolTest,
       ExistingRootRequiresValidOwnerAndReservation) {
  // Purpose: Existing physical roots must also pass owner and reservation
  // checks; rejected requests must not alter registration.
  // Input: Point impostor at owner's root and query it. Then reserve owner, try
  // taking its own root and enqueuing it again, and finally cancel.
  // Expected: Both impersonation and taking a reserved root return
  // kUnavailable. Re-enqueue returns nullptr and registration remains intact.
  // Cancellation makes the owner's root usable again.
  auto owner = CreateMaterializedItem(1);
  auto target = CreateUnmaterializedItem(2);
  auto impostor = CreateUnmaterializedItem(3);
  const auto sign = owner->PeekMaterializedRoot()->impl_id();
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, owner));
  ASSERT_EQ(pool_.Enqueue(sign), owner);

  // Holding the same root does not establish registered ownership; the original
  // candidate remains selectable.
  impostor->result_ = owner->PeekMaterializedRoot();
  ExpectUnavailable(pool_.Dequeue(impostor));
  EXPECT_EQ(pool_.GetInstance(sign), owner);
  auto action = pool_.Dequeue(target);
  ExpectReuse(action, sign, owner);

  // Once the root is reserved, even its original owner cannot take or
  // re-enqueue it.
  ExpectUnavailable(pool_.Dequeue(owner));
  EXPECT_EQ(pool_.Enqueue(sign), nullptr);
  EXPECT_EQ(pool_.GetInstance(sign), owner);
  pool_.CancelDequeue(action);
  ExpectUseExisting(pool_.Dequeue(owner), owner);
  ExpectCreate(pool_.Dequeue(target));
}

// Reuse-identifier grouping and binding transfers.
TEST_F(ListElementTemplateReusePoolTest,
       ReuseIdentifiersIsolateCandidatesAndPreserveFifo) {
  // Purpose: Items of the same template type but different reuse-identifiers
  // use separate buckets, and cross-bucket queries must preserve FIFO within
  // each bucket.
  // Input: Give A/B/C identifiers first/second/first, enqueue them in A/B/C
  // order, then query second before first.
  // Expected: Select B, A, then C. Exhausted buckets return kCreate, and query
  // targets remain unmaterialized.
  auto a = CreateMaterializedItem(1);
  auto b = CreateMaterializedItem(2);
  auto c = CreateMaterializedItem(3);
  auto first_group = CreateUnmaterializedItem(4);
  auto second_group = CreateUnmaterializedItem(5);
  SetReuseIdentifier(a, lepus::Value("first"));
  SetReuseIdentifier(b, lepus::Value("second"));
  SetReuseIdentifier(c, lepus::Value("first"));
  SetReuseIdentifier(first_group, lepus::Value("first"));
  SetReuseIdentifier(second_group, lepus::Value("second"));
  for (const auto& item : {a, b, c}) {
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
    ASSERT_EQ(pool_.Enqueue(item->PeekMaterializedRoot()->impl_id()), item);
  }

  // Different identifiers isolate candidates; cross-bucket requests preserve
  // A/C order in the first bucket.
  ExpectReuse(pool_.Dequeue(second_group), b->PeekMaterializedRoot()->impl_id(),
              b);
  ExpectReuse(pool_.Dequeue(first_group), a->PeekMaterializedRoot()->impl_id(),
              a);
  ExpectReuse(pool_.Dequeue(first_group), c->PeekMaterializedRoot()->impl_id(),
              c);
  ExpectCreate(pool_.Dequeue(first_group));
  ExpectCreate(pool_.Dequeue(second_group));
  EXPECT_EQ(first_group->PeekMaterializedRoot(), nullptr);
}

TEST_F(ListElementTemplateReusePoolTest,
       NonStringIdentifiersShareTheDefaultGroup) {
  // Purpose: Non-string reuse-identifiers are ignored and share the default
  // bucket with items lacking an identifier.
  // Input: Use null, undefined, numeric, boolean, and object identifiers in
  // turn, querying against default sources and targets in both directions.
  // Expected: Both directions select from the same default FIFO bucket;
  // explicit non-string values do not create separate groups.
  const std::vector<lepus::Value> identifiers = {
      lepus::Value(), lepus::Value(lepus::Value::kCreateAsUndefinedTag),
      lepus::Value(7), lepus::Value(true),
      lepus::Value(lepus::Dictionary::Create())};
  for (size_t index = 0; index < identifiers.size(); ++index) {
    SCOPED_TRACE(index);
    auto with_identifier = CreateMaterializedItem(1);
    auto without_identifier = CreateMaterializedItem(2);
    auto default_target = CreateUnmaterializedItem(3);
    auto matching_target = CreateUnmaterializedItem(4);
    SetReuseIdentifier(with_identifier, identifiers[index]);
    SetReuseIdentifier(matching_target, identifiers[index]);
    for (const auto& item : {with_identifier, without_identifier}) {
      ASSERT_TRUE(pool_.RegisterNewBinding(
          item->PeekMaterializedRoot()->impl_id(), item));
      ASSERT_EQ(pool_.Enqueue(item->PeekMaterializedRoot()->impl_id()), item);
    }

    // Non-string identifiers are ignored, so both directions use the default
    // bucket.
    ExpectReuse(pool_.Dequeue(default_target),
                with_identifier->PeekMaterializedRoot()->impl_id(),
                with_identifier);
    ExpectReuse(pool_.Dequeue(matching_target),
                without_identifier->PeekMaterializedRoot()->impl_id(),
                without_identifier);
    pool_.Clear();
  }
}

TEST_F(ListElementTemplateReusePoolTest,
       EmptyStringIdentifierUsesSeparateGroup) {
  // Purpose: An explicit empty-string identifier uses a separate bucket from an
  // absent identifier, preventing cross-bucket selection.
  // Input: Enqueue only a default source and query an empty-string target, then
  // add an empty-string source and query both buckets.
  // Expected: The absent empty-string bucket returns kCreate. Once both buckets
  // exist, each selects only its matching source and returns kCreate when
  // exhausted.
  auto with_identifier = CreateMaterializedItem(1);
  auto without_identifier = CreateMaterializedItem(2);
  auto default_target = CreateUnmaterializedItem(3);
  auto matching_target = CreateUnmaterializedItem(4);
  SetReuseIdentifier(with_identifier, lepus::Value(""));
  SetReuseIdentifier(matching_target, lepus::Value(""));
  const auto default_sign =
      without_identifier->PeekMaterializedRoot()->impl_id();
  const auto empty_sign = with_identifier->PeekMaterializedRoot()->impl_id();
  ASSERT_TRUE(pool_.RegisterNewBinding(default_sign, without_identifier));
  ASSERT_EQ(pool_.Enqueue(default_sign), without_identifier);

  // An absent empty-string bucket must not consume a default-bucket candidate.
  ExpectCreate(pool_.Dequeue(matching_target));
  ASSERT_TRUE(pool_.RegisterNewBinding(empty_sign, with_identifier));
  ASSERT_EQ(pool_.Enqueue(empty_sign), with_identifier);
  ExpectReuse(pool_.Dequeue(default_target), default_sign, without_identifier);
  ExpectCreate(pool_.Dequeue(default_target));
  ExpectReuse(pool_.Dequeue(matching_target), empty_sign, with_identifier);
  ExpectCreate(pool_.Dequeue(matching_target));
}

TEST_F(ListElementTemplateReusePoolTest,
       RebindingToDifferentIdentifierRevokesOldEligibility) {
  // Purpose: Transferring a physical root to a new owner with a different
  // identifier must revoke eligibility in the old bucket.
  // Input: Reserve an old source, transfer and register its root to a new
  // owner, enqueue it into the new bucket, then cancel the old Action.
  // Expected: The old target returns kCreate, and the new target selects the
  // new owner with the same sign. Old cancellation cannot restore old-bucket
  // eligibility.
  auto source = CreateMaterializedItem(1);
  auto new_owner = CreateUnmaterializedItem(2);
  auto old_group = CreateUnmaterializedItem(3);
  auto new_group = CreateUnmaterializedItem(4);
  SetReuseIdentifier(source, lepus::Value("old"));
  SetReuseIdentifier(old_group, lepus::Value("old"));
  SetReuseIdentifier(new_owner, lepus::Value("new"));
  SetReuseIdentifier(new_group, lepus::Value("new"));
  const auto sign = source->PeekMaterializedRoot()->impl_id();
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, source));
  ASSERT_EQ(pool_.Enqueue(sign), source);
  auto action = pool_.Dequeue(old_group);
  ExpectReuse(action, sign, source);
  new_owner->result_ = std::move(source->result_);
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, new_owner));
  ASSERT_EQ(pool_.Enqueue(sign), new_owner);

  // Each logical instance keeps its identifier stable; after transfer, only the
  // new owner's bucket can select this root.
  pool_.CancelDequeue(action);
  ExpectCreate(pool_.Dequeue(old_group));
  ExpectReuse(pool_.Dequeue(new_group), sign, new_owner);
}

// Lifetime and GC.
TEST_F(ListElementTemplateReusePoolTest,
       ClearReleasesRegistrationWithoutChangingRetainedInstance) {
  // Purpose: Registration and lookup independently keep the owner alive. Clear
  // removes registration and eligibility without changing an externally
  // retained instance.
  // Input: Register an item with a child, release its external reference, and
  // obtain a lookup. Enqueue and clear the pool, inspect state, then release
  // the lookup.
  // Expected: Owner lifetime follows the remaining references before and after
  // Clear. Root, UID, and child relationships stay unchanged, with no automatic
  // registration. Releasing lookup destroys the owner.
  auto item = CreateMaterializedItem(1);
  auto child = CreateUnmaterializedItem(2);
  SetChildren(item, {child});
  const auto sign = item->PeekMaterializedRoot()->impl_id();
  auto original_root = item->PeekMaterializedRoot();
  auto original_uid = item->GetUid();
  auto original_children = item->child_slots_;
  auto weak_item = item->WeakFromThis();
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, item));

  // Registration alone must retain the owner before Enqueue; lookup then holds
  // an independent reference.
  item = nullptr;
  ASSERT_NE(weak_item.get(), nullptr);
  auto lookup = pool_.GetInstance(sign);
  ASSERT_EQ(lookup.get(), weak_item.get());
  ASSERT_EQ(pool_.Enqueue(sign), lookup);
  pool_.Clear();
  EXPECT_EQ(pool_.GetInstance(sign), nullptr);
  EXPECT_NE(weak_item.get(), nullptr);

  // Clearing the pool leaves root, UID, and logical tree unchanged and does not
  // automatically re-register the retained root.
  ExpectUseExisting(pool_.Dequeue(lookup), lookup);
  EXPECT_EQ(pool_.GetInstance(sign), nullptr);
  EXPECT_EQ(lookup->PeekMaterializedRoot(), original_root);
  EXPECT_EQ(lookup->PeekMaterializedRoot()->impl_id(), sign);
  EXPECT_EQ(lookup->GetUid(), original_uid);
  EXPECT_TRUE(lookup->child_slots_.IsEqual(original_children));
  auto children = lookup->child_slots_.GetProperty(uint32_t{0});
  ASSERT_TRUE(children.IsArray());
  ASSERT_EQ(children.GetLength(), 1);
  EXPECT_EQ(ElementTemplateInstance::ResolveElementTemplateInstanceValue(
                children.GetProperty(uint32_t{0})),
            child);
  EXPECT_EQ(child->GetLogicalParent(), lookup.get());
  lookup = nullptr;
  EXPECT_EQ(weak_item.get(), nullptr);
}

TEST_F(ListElementTemplateReusePoolTest, PoolDestructionReleasesRetainedOwner) {
  // Purpose: Destroying a pool must release retained owners without requiring
  // an explicit Clear.
  // Input: Register and enqueue item in a temporary pool, release the external
  // item reference, then leave the pool's scope.
  // Expected: The weak reference remains valid while the pool exists and
  // expires when the pool is destroyed.
  auto item = CreateMaterializedItem(1);
  const auto sign = item->PeekMaterializedRoot()->impl_id();
  auto weak_item = item->WeakFromThis();
  {
    ListElementTemplateReusePool temporary_pool;
    ASSERT_TRUE(temporary_pool.RegisterNewBinding(sign, item));
    temporary_pool.Enqueue(sign);
    item = nullptr;
    EXPECT_NE(weak_item.get(), nullptr);
  }
  EXPECT_EQ(weak_item.get(), nullptr);
}

TEST_F(ListElementTemplateReusePoolTest, ReuseActionRetainsSourceAfterClear) {
  // Purpose: Action independently retains the source instance and physical
  // root, so clearing the pool cannot leave an in-flight Action dangling.
  // Input: Reserve source for target, release the external source reference,
  // clear the pool and cancel the old Action, then release Action.
  // Expected: The pool is empty and target remains unmaterialized, but Action
  // still holds the original source and root. Releasing Action destroys source.
  auto source = CreateMaterializedItem(1);
  auto target = CreateUnmaterializedItem(2);
  const auto sign = source->PeekMaterializedRoot()->impl_id();
  auto weak_source = source->WeakFromThis();
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, source));
  pool_.Enqueue(sign);
  Action action = pool_.Dequeue(target);
  ExpectReuse(action, sign, source);

  source = nullptr;
  pool_.Clear();
  pool_.CancelDequeue(action);
  EXPECT_EQ(pool_.GetInstance(sign), nullptr);
  ExpectCreate(pool_.Dequeue(target));
  ASSERT_NE(weak_source.get(), nullptr);
  EXPECT_EQ(action.reusable_instance_.get(), weak_source.get());
  ASSERT_NE(action.reusable_root_, nullptr);
  EXPECT_EQ(action.reusable_root_,
            action.reusable_instance_->PeekMaterializedRoot());
  EXPECT_EQ(action.reusable_root_->impl_id(), sign);
  EXPECT_EQ(target->PeekMaterializedRoot(), nullptr);

  action = Action{};
  EXPECT_EQ(weak_source.get(), nullptr);
}

TEST_F(ListElementTemplateReusePoolTest,
       ReuseActionRetainsOldSourceAfterBindingTransfersToTarget) {
  // Purpose: After root transfer and registration update, Action must retain
  // both the old source and the originally selected physical root.
  // Input: Reserve source for target, transfer and register the root to target,
  // release external source, then release Action.
  // Expected: Action retains the rootless old source and original root.
  // Releasing it destroys the old source without changing target's registration
  // or root.
  auto source = CreateMaterializedItem(1);
  auto target = CreateUnmaterializedItem(2);
  const auto sign = source->PeekMaterializedRoot()->impl_id();
  auto weak_source = source->WeakFromThis();
  auto original_root = source->PeekMaterializedRoot();
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, source));
  pool_.Enqueue(sign);
  Action action = pool_.Dequeue(target);
  ExpectReuse(action, sign, source);

  // The pool does not transfer physical trees; set only the physical state
  // required before registering the new owner.
  target->result_ = std::move(source->result_);
  ASSERT_EQ(source->PeekMaterializedRoot(), nullptr);
  ASSERT_EQ(target->PeekMaterializedRoot(), original_root);
  ASSERT_TRUE(pool_.RegisterNewBinding(sign, target));
  EXPECT_EQ(pool_.GetInstance(sign), target);
  source = nullptr;

  ASSERT_NE(weak_source.get(), nullptr);
  EXPECT_EQ(action.reusable_instance_.get(), weak_source.get());
  // After transfer, Action still stores the originally selected root; it must
  // not fetch it again from the rootless old owner.
  EXPECT_EQ(action.reusable_root_, original_root);
  EXPECT_EQ(target->PeekMaterializedRoot()->impl_id(), sign);
  action = Action{};
  EXPECT_EQ(weak_source.get(), nullptr);
  EXPECT_EQ(pool_.GetInstance(sign), target);
  EXPECT_EQ(target->PeekMaterializedRoot(), original_root);
}

TEST_F(ListElementTemplateReusePoolTest,
       ListVisitorVisitsDirectRemovedAndReparentedOwnersOnce) {
  // Purpose: The List visitor combines pool and logical-child traversal without
  // duplicate visits and forwards GC parameters.
  // Input: Register three tracked ETs, keeping one directly attached, removing
  // another, and reparenting the third, then invoke the host List's visitor.
  // Expected: Each ET is visited once with the supplied runtime, mark_func, and
  // trace_tool.
  auto direct = CreateTrackingItem(1);
  auto removed = CreateTrackingItem(2);
  auto reparented = CreateTrackingItem(3);
  auto host = fml::AdoptRef(new ListElementTemplateInstance(manager_));
  host->SetTypedTag(base::String("list"));
  host->SetUid(lepus::Value(4));
  SetChildren(host, {direct, removed, reparented});
  auto other_parent = CreateParent({});
  auto& pool = host->reuse_pool_;
  ASSERT_TRUE(pool.RegisterNewBinding(direct->PeekMaterializedRoot()->impl_id(),
                                      direct));
  ASSERT_TRUE(pool.RegisterNewBinding(
      removed->PeekMaterializedRoot()->impl_id(), removed));
  ASSERT_TRUE(pool.RegisterNewBinding(
      reparented->PeekMaterializedRoot()->impl_id(), reparented));
  host->RemoveNodeFromChildSlot(uint32_t{0}, lepus::Value(Instance(removed)));
  other_parent->InsertNodeIntoChildSlot(
      uint32_t{0}, lepus::Value(Instance(reparented)), lepus::Value());
  ASSERT_EQ(direct->GetLogicalParent(), host.get());
  ASSERT_EQ(removed->GetLogicalParent(), nullptr);
  ASSERT_EQ(reparented->GetLogicalParent(), other_parent.get());

  int runtime_token = 0;
  int mark_token = 0;
  void* runtime = &runtime_token;
  void* mark_func = &mark_token;
  constexpr uint64_t trace_tool = 12345;
  host->visitor(runtime, mark_func, trace_tool);

  EXPECT_EQ(direct->visit_count_, 1);
  EXPECT_EQ(removed->visit_count_, 1);
  EXPECT_EQ(reparented->visit_count_, 1);
  for (const auto& tracked : {direct, removed, reparented}) {
    EXPECT_EQ(tracked->last_runtime_, runtime);
    EXPECT_EQ(tracked->last_mark_func_, mark_func);
    EXPECT_EQ(tracked->last_trace_tool_, trace_tool);
  }
}

TEST_F(ListElementTemplateReusePoolTest,
       ListVisitorTraversesRetainedOwnerThroughOrdinaryNestedInstances) {
  // Purpose: A retained old owner remains traversable through ordinary
  // intermediate ETs to nested descendants.
  // Input: Build owner -> middle -> tracked leaf, remove owner from List, clear
  // insertion/removal records, then invoke visitor.
  // Expected: The owner stays alive through the pool alone, leaf is visited
  // once with forwarded GC parameters, and clearing the pool destroys owner and
  // detaches middle from its old parent.
  auto leaf = CreateTrackingItem(1);
  auto middle = CreateParent({leaf});
  auto owner = CreateMaterializedItem(2);
  SetChildren(owner, {middle});
  auto host = fml::AdoptRef(new ListElementTemplateInstance(manager_));
  host->SetTypedTag(base::String("list"));
  host->SetUid(lepus::Value(3));
  SetChildren(host, {owner});
  auto& pool = host->reuse_pool_;
  const auto sign = owner->PeekMaterializedRoot()->impl_id();
  auto weak_owner = owner->WeakFromThis();
  ASSERT_TRUE(pool.RegisterNewBinding(sign, owner));
  pool.Enqueue(sign);
  host->RemoveNodeFromChildSlot(uint32_t{0}, lepus::Value(owner));
  ASSERT_EQ(owner->GetLogicalParent(), nullptr);
  // Insertion/removal records also retain owner temporarily. Clear them to
  // isolate the pool's lifetime guarantee.
  host->list_update_info_recorder_.reset();
  owner = nullptr;
  ASSERT_NE(weak_owner.get(), nullptr);

  int runtime_token = 0;
  int mark_token = 0;
  constexpr uint64_t trace_tool = 67890;
  host->visitor(&runtime_token, &mark_token, trace_tool);

  EXPECT_EQ(leaf->visit_count_, 1);
  EXPECT_EQ(leaf->last_runtime_, &runtime_token);
  EXPECT_EQ(leaf->last_mark_func_, &mark_token);
  EXPECT_EQ(leaf->last_trace_tool_, trace_tool);
  EXPECT_EQ(leaf->GetLogicalParent(), middle.get());

  pool.Clear();
  EXPECT_EQ(weak_owner.get(), nullptr);
  EXPECT_EQ(middle->GetLogicalParent(), nullptr);
}

// Mixed operations on a large queue.
TEST_F(ListElementTemplateReusePoolTest,
       LargeBatchKeepsFifoAcrossMixedStateChanges) {
  // Purpose: Mixed reservations, cancellations, and invalidation preserve FIFO
  // in a large queue, without restoring invalid eligibility automatically.
  // Input: Enqueue 64 items and reserve the first 16. Cancel 9 then 3, clear
  // item 16's root, change item 17's type, query, restore state, then clear the
  // pool.
  // Expected: Select 3, 9, then 18..63. Restored invalid items remain
  // unavailable; item 16 becomes reusable after Clear and explicit
  // registration/enqueue.
  std::vector<Instance> items;
  for (int index = 0; index < 64; ++index) {
    auto item = CreateMaterializedItem(index);
    ASSERT_TRUE(pool_.RegisterNewBinding(
        item->PeekMaterializedRoot()->impl_id(), item));
    pool_.Enqueue(item->PeekMaterializedRoot()->impl_id());
    items.push_back(std::move(item));
  }
  auto target = CreateUnmaterializedItem(64);
  std::vector<Action> actions;
  for (int index = 0; index < 16; ++index) {
    actions.push_back(pool_.Dequeue(target));
    ExpectReuse(actions.back(), items[index]->PeekMaterializedRoot()->impl_id(),
                items[index]);
  }

  pool_.CancelDequeue(actions[9]);
  pool_.CancelDequeue(actions[3]);
  auto original_root = items[16]->PeekMaterializedRoot();
  items[16]->result_ = nullptr;
  items[17]->SetTemplateKey(base::String("Other"));

  ExpectReuse(pool_.Dequeue(target),
              items[3]->PeekMaterializedRoot()->impl_id(), items[3]);
  ExpectReuse(pool_.Dequeue(target),
              items[9]->PeekMaterializedRoot()->impl_id(), items[9]);
  for (int index = 18; index < 64; ++index) {
    SCOPED_TRACE(index);
    ExpectReuse(pool_.Dequeue(target),
                items[index]->PeekMaterializedRoot()->impl_id(), items[index]);
  }
  ExpectCreate(pool_.Dequeue(target));

  items[16]->result_ = original_root;
  items[17]->SetTemplateKey(base::String("Card"));
  ExpectCreate(pool_.Dequeue(target));
  EXPECT_EQ(target->PeekMaterializedRoot(), nullptr);

  // A fresh batch can register and enqueue normally without inheriting
  // eligibility from the previous batch.
  pool_.Clear();
  ASSERT_TRUE(pool_.RegisterNewBinding(
      items[16]->PeekMaterializedRoot()->impl_id(), items[16]));
  pool_.Enqueue(items[16]->PeekMaterializedRoot()->impl_id());
  ExpectReuse(pool_.Dequeue(target),
              items[16]->PeekMaterializedRoot()->impl_id(), items[16]);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
