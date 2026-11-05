// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/list_update_info_recorder.h"

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/template_assembler.h"
#undef private
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
namespace {

using InstanceRefPtr = fml::RefPtr<ElementTemplateInstance>;

constexpr bool kDefaultFullSpan = false;
constexpr bool kDefaultStickyTop = false;
constexpr bool kDefaultStickyBottom = false;
constexpr double kDefaultEstimatedMainAxisSizePx = 0.0;

lepus::Value MakeObject(
    std::initializer_list<std::pair<const char*, lepus::Value>> properties) {
  auto object = lepus::Dictionary::Create();
  for (const auto& property : properties) {
    object->SetValue(base::String(property.first), property.second);
  }
  return lepus::Value(std::move(object));
}

lepus::Value MakeArray(std::initializer_list<lepus::Value> values) {
  auto array = lepus::CArray::Create();
  for (const auto& value : values) {
    array->emplace_back(value);
  }
  return lepus::Value(std::move(array));
}

lepus::Value MakeChildren(std::initializer_list<InstanceRefPtr> children) {
  auto array = lepus::CArray::Create();
  for (const auto& child : children) {
    array->emplace_back(lepus::Value(child));
  }
  return lepus::Value(std::move(array));
}

lepus::Value MakeListItemPlatformInfo(
    const char* key, bool full_span = kDefaultFullSpan,
    bool sticky_top = kDefaultStickyTop,
    bool sticky_bottom = kDefaultStickyBottom,
    double estimated_main_axis_size_px = kDefaultEstimatedMainAxisSizePx) {
  return MakeObject({{"item-key", lepus::Value(key)},
                     {"full-span", lepus::Value(full_span)},
                     {"sticky-top", lepus::Value(sticky_top)},
                     {"sticky-bottom", lepus::Value(sticky_bottom)},
                     {"estimated-main-axis-size-px",
                      lepus::Value(estimated_main_axis_size_px)}});
}

lepus::Value InsertAction(
    const char* key, int32_t position, const char* type = "Card",
    bool full_span = kDefaultFullSpan, bool sticky_top = kDefaultStickyTop,
    bool sticky_bottom = kDefaultStickyBottom,
    double estimated_main_axis_size_px = kDefaultEstimatedMainAxisSizePx) {
  return MakeObject({{"item-key", lepus::Value(key)},
                     {"full-span", lepus::Value(full_span)},
                     {"sticky-top", lepus::Value(sticky_top)},
                     {"sticky-bottom", lepus::Value(sticky_bottom)},
                     {"estimated-main-axis-size-px",
                      lepus::Value(estimated_main_axis_size_px)},
                     {"position", lepus::Value(position)},
                     {"type", lepus::Value(type)}});
}

lepus::Value UpdateAction(
    const char* key, int32_t position, const char* type = "Card",
    bool full_span = kDefaultFullSpan, bool sticky_top = kDefaultStickyTop,
    bool sticky_bottom = kDefaultStickyBottom,
    double estimated_main_axis_size_px = kDefaultEstimatedMainAxisSizePx) {
  return MakeObject({{"item-key", lepus::Value(key)},
                     {"full-span", lepus::Value(full_span)},
                     {"sticky-top", lepus::Value(sticky_top)},
                     {"sticky-bottom", lepus::Value(sticky_bottom)},
                     {"estimated-main-axis-size-px",
                      lepus::Value(estimated_main_axis_size_px)},
                     {"from", lepus::Value(position)},
                     {"to", lepus::Value(position)},
                     {"flush", lepus::Value(false)},
                     {"type", lepus::Value(type)}});
}

}  // namespace

class ListUpdateInfoRecorderTest : public ::testing::Test {
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
    list_ = fml::AdoptRef<ListElement>(new ListElement(
        manager_, "list", lepus::Value(), lepus::Value(), lepus::Value()));
    list_->set_tasm(tasm_.get());
  }

  InstanceRefPtr CreateItem(
      const char* key, bool full_span = kDefaultFullSpan,
      bool sticky_top = kDefaultStickyTop,
      bool sticky_bottom = kDefaultStickyBottom,
      double estimated_main_axis_size_px = kDefaultEstimatedMainAxisSizePx) {
    auto item = fml::AdoptRef(new ElementTemplateInstance(manager_));
    item->SetTemplateKey(base::String("Card"));
    item->SetUid(lepus::Value(key));
    item->SetListItemPlatformInfo(
        MakeListItemPlatformInfo(key, full_span, sticky_top, sticky_bottom,
                                 estimated_main_axis_size_px));
    return item;
  }

  InstanceRefPtr CreateParent(std::initializer_list<InstanceRefPtr> children) {
    auto parent = fml::AdoptRef(new ElementTemplateInstance(manager_));
    // Establish logical parents without GetRoot() to avoid materializing items.
    parent->InitializeChildSlots(MakeArray({MakeChildren(children)}));
    return parent;
  }

  bool HasUpdateListInfo() const {
    const auto& attributes = list_->data_model()->attributes();
    return attributes.find(base::String("update-list-info")) !=
           attributes.end();
  }

  lepus::Value GetUpdateListInfo() const {
    const auto& attributes = list_->data_model()->attributes();
    auto it = attributes.find(base::String("update-list-info"));
    return it == attributes.end() ? lepus::Value() : it->second;
  }

  void ExpectUpdateListInfo(
      std::initializer_list<lepus::Value> insert_actions,
      std::initializer_list<int32_t> remove_positions,
      std::initializer_list<lepus::Value> update_actions) {
    auto remove_actions = lepus::CArray::Create();
    for (int32_t position : remove_positions) {
      remove_actions->emplace_back(position);
    }
    auto expected =
        MakeObject({{"insertAction", MakeArray(insert_actions)},
                    {"removeAction", lepus::Value(std::move(remove_actions))},
                    {"updateAction", MakeArray(update_actions)}});
    // Compare the complete dictionary, including empty unrelated action arrays,
    // to catch extra operations or fields.
    ASSERT_TRUE(HasUpdateListInfo());
    EXPECT_EQ(GetUpdateListInfo(), expected);
  }

  ::testing::NiceMock<test::MockTasmDelegate> tasm_delegate_;
  std::shared_ptr<TemplateAssembler> tasm_;
  ElementManager* manager_ = nullptr;
  fml::RefPtr<ListElement> list_;
};

TEST_F(ListUpdateInfoRecorderTest, NonArrayOldChildrenProduceEmptySnapshot) {
  // Purpose: Construct a recorder safely from non-array input.
  // Input: nil, number, string, and plain object; record no subsequent
  // operations. Expected: The old snapshot and all four operation collections
  // are empty.
  for (const auto& value : {lepus::Value(), lepus::Value(1),
                            lepus::Value("invalid"), MakeObject({})}) {
    ListUpdateInfoRecorder recorder(value);
    EXPECT_TRUE(recorder.old_children().empty());
    EXPECT_TRUE(recorder.insert_before().empty());
    EXPECT_TRUE(recorder.append_child().empty());
    EXPECT_TRUE(recorder.remove_child().empty());
    EXPECT_TRUE(recorder.platform_info_update().empty());
  }
}

TEST_F(ListUpdateInfoRecorderTest, EmptyArrayProducesEmptySnapshot) {
  // Purpose: Accept an empty array as the initial list.
  // Input: old_children=[].
  // Expected: Empty snapshot; Flush returns true without writing
  // update-list-info.
  ListUpdateInfoRecorder recorder(MakeChildren({}));
  EXPECT_TRUE(recorder.old_children().empty());
  EXPECT_TRUE(recorder.Flush(list_.get()));
  EXPECT_FALSE(HasUpdateListInfo());
}

TEST_F(ListUpdateInfoRecorderTest, InvalidChildrenDoNotRecordOperations) {
  // Purpose: Accept only ET instances for insertion.
  // Input: Try inserting nil, a number, an object, an array, and a ListElement.
  // Expected: No anchored insertions or appends; Flush writes no update
  // attribute.
  auto item_a = CreateItem("A");
  ListUpdateInfoRecorder recorder(MakeChildren({item_a}));
  for (const auto& value : {lepus::Value(), lepus::Value(1), MakeObject({}),
                            MakeArray({}), lepus::Value(list_)}) {
    recorder.OnInsertBefore(value, lepus::Value(item_a));
  }
  EXPECT_TRUE(recorder.insert_before().empty());
  EXPECT_TRUE(recorder.append_child().empty());
  EXPECT_TRUE(recorder.remove_child().empty());
  EXPECT_TRUE(recorder.platform_info_update().empty());
  EXPECT_TRUE(recorder.Flush(list_.get()));
  EXPECT_FALSE(HasUpdateListInfo());
}

TEST_F(ListUpdateInfoRecorderTest, InvalidChildrenDoNotRecordRemoveOperations) {
  // Purpose: Accept only ET instances for removal.
  // Input: old=[A]; try removing nil, a number, an object, an array, and a
  // ListElement. Expected: All operation collections stay empty; Flush writes
  // no update attribute.
  auto item_a = CreateItem("A");
  ListUpdateInfoRecorder recorder(MakeChildren({item_a}));
  for (const auto& value : {lepus::Value(), lepus::Value(1), MakeObject({}),
                            MakeArray({}), lepus::Value(list_)}) {
    recorder.OnRemoveChild(value);
  }
  EXPECT_TRUE(recorder.insert_before().empty());
  EXPECT_TRUE(recorder.append_child().empty());
  EXPECT_TRUE(recorder.remove_child().empty());
  EXPECT_TRUE(recorder.platform_info_update().empty());
  EXPECT_TRUE(recorder.Flush(list_.get()));
  EXPECT_FALSE(HasUpdateListInfo());
}

TEST_F(ListUpdateInfoRecorderTest, InvalidChildrenDoNotRecordUpdateOperations) {
  // Purpose: Accept only ET instances for metadata updates.
  // Input: old=[A]; set valid metadata on five invalid child values.
  // Expected: All operation collections stay empty; Flush writes no update
  // attribute.
  auto item_a = CreateItem("A");
  ListUpdateInfoRecorder recorder(MakeChildren({item_a}));
  auto platform_info = MakeListItemPlatformInfo("A");
  for (const auto& value : {lepus::Value(), lepus::Value(1), MakeObject({}),
                            MakeArray({}), lepus::Value(list_)}) {
    recorder.OnSetAttribute(value, platform_info);
  }
  EXPECT_TRUE(recorder.insert_before().empty());
  EXPECT_TRUE(recorder.append_child().empty());
  EXPECT_TRUE(recorder.remove_child().empty());
  EXPECT_TRUE(recorder.platform_info_update().empty());
  EXPECT_TRUE(recorder.Flush(list_.get()));
  EXPECT_FALSE(HasUpdateListInfo());
}

TEST_F(ListUpdateInfoRecorderTest, MissingOrInvalidAnchorRecordsAppendInOrder) {
  // Purpose: Append in call order when the anchor cannot be resolved.
  // Input: old=[]; insert A with a nil anchor, then B with a numeric anchor.
  // Expected: append_child=[A,B]; insert_before and remove_child stay empty.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  ListUpdateInfoRecorder recorder(MakeChildren({}));
  recorder.OnInsertBefore(lepus::Value(a), lepus::Value());
  recorder.OnInsertBefore(lepus::Value(b), lepus::Value(1));
  ASSERT_EQ(recorder.append_child().size(), 2u);
  EXPECT_EQ(recorder.append_child()[0], a);
  EXPECT_EQ(recorder.append_child()[1], b);
  EXPECT_TRUE(recorder.insert_before().empty());
  EXPECT_TRUE(recorder.remove_child().empty());
}

TEST_F(ListUpdateInfoRecorderTest,
       InsertBeforeGroupsByInstanceAndKeepsCallOrder) {
  // Purpose: Group by anchor instance, preserving call order within each group.
  // Input: old=[A,B] with one template; insert X before A, Z before B, then Y
  // before A. Expected: A's group is [X,Y], B's group is [Z]; each retains its
  // anchor reference.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto x = CreateItem("X");
  auto y = CreateItem("Y");
  auto z = CreateItem("Z");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b}));
  recorder.OnInsertBefore(lepus::Value(x), lepus::Value(a));
  recorder.OnInsertBefore(lepus::Value(z), lepus::Value(b));
  recorder.OnInsertBefore(lepus::Value(y), lepus::Value(a));
  ASSERT_EQ(recorder.insert_before().size(), 2u);
  const auto& group_a = recorder.insert_before().at(a.get());
  const auto& group_b = recorder.insert_before().at(b.get());
  EXPECT_EQ(group_a.ref_child_, a);
  EXPECT_EQ(group_b.ref_child_, b);
  ASSERT_EQ(group_a.children_.size(), 2u);
  ASSERT_EQ(group_b.children_.size(), 1u);
  EXPECT_EQ(group_a.children_[0], x);
  EXPECT_EQ(group_a.children_[1], y);
  EXPECT_EQ(group_b.children_[0], z);
  EXPECT_TRUE(recorder.append_child().empty());
  EXPECT_TRUE(recorder.remove_child().empty());
}

TEST_F(ListUpdateInfoRecorderTest, RemoveDeduplicatesWithoutDetachingChild) {
  // Purpose: Deduplicate removals without detaching logical children.
  // Input: A parent contains [A,B]; call OnRemoveChild(A) twice.
  // Expected: remove_child contains only A; logical parents and slots are
  // unchanged.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto parent = CreateParent({a, b});
  ASSERT_EQ(a->GetLogicalParent(), parent.get());
  ASSERT_EQ(b->GetLogicalParent(), parent.get());
  ListUpdateInfoRecorder recorder(MakeChildren({a, b}));
  recorder.OnRemoveChild(lepus::Value(a));
  recorder.OnRemoveChild(lepus::Value(a));
  EXPECT_EQ(recorder.remove_child().size(), 1u);
  EXPECT_EQ(recorder.remove_child().count(a), 1u);
  EXPECT_EQ(recorder.remove_child().count(b), 0u);
  EXPECT_EQ(a->GetLogicalParent(), parent.get());
  EXPECT_EQ(b->GetLogicalParent(), parent.get());
  EXPECT_EQ(parent->GetChildSlots(), MakeArray({MakeChildren({a, b})}));
}

TEST_F(ListUpdateInfoRecorderTest, PlatformInfoKeepsLatestIndependentSnapshot) {
  // Purpose: Replace item metadata with an independent deep snapshot.
  // Input: Record A's obsolete metadata, then height=20/nested.value=2;
  //        mutate the source objects afterward.
  // Expected: One update with height=20 and nested.value=2, without obsolete
  // fields.
  auto a = CreateItem("A");
  auto nested = MakeObject({{"value", lepus::Value(2)}});
  auto latest = MakeObject({{"height", lepus::Value(20)}, {"nested", nested}});
  ListUpdateInfoRecorder recorder(MakeChildren({a}));
  recorder.OnSetAttribute(lepus::Value(a),
                          MakeObject({{"obsolete", lepus::Value(true)}}));
  recorder.OnSetAttribute(lepus::Value(a), latest);
  latest.SetProperty(base::String("height"), lepus::Value(30));
  nested.SetProperty(base::String("value"), lepus::Value(3));

  ASSERT_EQ(recorder.platform_info_update().size(), 1u);
  EXPECT_EQ(recorder.platform_info_update().at(a),
            MakeObject({{"height", lepus::Value(20)},
                        {"nested", MakeObject({{"value", lepus::Value(2)}})}}));
}

TEST_F(ListUpdateInfoRecorderTest,
       NonObjectPlatformInfoReplacesPreviousRecord) {
  // Purpose: Normalize non-object metadata to an empty object, replacing old
  // data. Input: Record valid metadata for A, then nil, a number, or an array
  // in each round. Expected: The update map contains only A with an empty
  // dictionary.
  auto a = CreateItem("A");
  ListUpdateInfoRecorder recorder(MakeChildren({a}));
  for (const auto& value : {lepus::Value(), lepus::Value(1), MakeArray({})}) {
    recorder.OnSetAttribute(lepus::Value(a), MakeListItemPlatformInfo("A"));
    recorder.OnSetAttribute(lepus::Value(a), value);

    ASSERT_EQ(recorder.platform_info_update().size(), 1u);
    EXPECT_EQ(recorder.platform_info_update().at(a), MakeObject({}));
  }
}

TEST_F(ListUpdateInfoRecorderTest, FlushRemovesOldItemAndAppendsNewItem) {
  // Purpose: Flush the removal of an old item and insertion of a new item.
  // Input: old=[A]; remove A and append B.
  // Expected: Flush=true; insert=[B@0], remove=[0], update=[].
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto view = manager_->CreateFiberView();
  ListUpdateInfoRecorder recorder(MakeChildren({a}));
  recorder.OnRemoveChild(lepus::Value(a));
  recorder.OnInsertBefore(lepus::Value(b), lepus::Value());
  EXPECT_EQ(recorder.remove_child().count(a), 1u);
  ASSERT_EQ(recorder.append_child().size(), 1u);
  EXPECT_EQ(recorder.append_child()[0], b);
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({InsertAction("B", 0)}, {0}, {});
}

TEST_F(ListUpdateInfoRecorderTest, FlushAppendsToEmptyList) {
  // Purpose: Flush consecutive appends with full insertion data in call order.
  // Input: old=[]; append A then B, both using Card with their own item-key.
  // Expected: Flush=true; insert=[A@0,B@1], remove=[], update=[].
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  ListUpdateInfoRecorder recorder(MakeChildren({}));
  recorder.OnInsertBefore(lepus::Value(a), lepus::Value());
  recorder.OnInsertBefore(lepus::Value(b), lepus::Value());
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({InsertAction("A", 0), InsertAction("B", 1)}, {}, {});
}

TEST_F(ListUpdateInfoRecorderTest, FlushPreservesCustomPlatformInfo) {
  // Each flag is enabled independently to detect swapped or ignored parameters.
  auto full_span_item = CreateItem("full-span", true, false, false, 100);
  auto sticky_top_item = CreateItem("sticky-top", false, true, false, 100);
  auto sticky_bottom_item =
      CreateItem("sticky-bottom", false, false, true, 100);

  ListUpdateInfoRecorder insert_recorder(MakeChildren({}));
  for (const auto& item :
       {full_span_item, sticky_top_item, sticky_bottom_item}) {
    insert_recorder.OnInsertBefore(lepus::Value(item), lepus::Value());
  }
  ASSERT_TRUE(insert_recorder.Flush(list_.get()));
  ExpectUpdateListInfo(
      {InsertAction("full-span", 0, "Card", true, false, false, 100),
       InsertAction("sticky-top", 1, "Card", false, true, false, 100),
       InsertAction("sticky-bottom", 2, "Card", false, false, true, 100)},
      {}, {});

  ListUpdateInfoRecorder update_recorder(
      MakeChildren({full_span_item, sticky_top_item, sticky_bottom_item}));
  for (const auto& item :
       {full_span_item, sticky_top_item, sticky_bottom_item}) {
    update_recorder.OnSetAttribute(lepus::Value(item),
                                   item->GetListItemPlatformInfoFromOptions());
  }
  ASSERT_TRUE(update_recorder.Flush(list_.get()));
  ExpectUpdateListInfo(
      {}, {},
      {UpdateAction("full-span", 0, "Card", true, false, false, 100),
       UpdateAction("sticky-top", 1, "Card", false, true, false, 100),
       UpdateAction("sticky-bottom", 2, "Card", false, false, true, 100)});
}

TEST_F(ListUpdateInfoRecorderTest, FlushAppendsAfterRetainedOldItems) {
  // Purpose: Append after the retained old items in a nonempty list.
  // Input: old=[A,B]; append C.
  // Expected: Flush=true; insert=[C@2], remove=[], update=[].
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b}));
  recorder.OnInsertBefore(lepus::Value(c), lepus::Value());
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({InsertAction("C", 2)}, {}, {});
}

TEST_F(ListUpdateInfoRecorderTest, FlushInsertsWithoutPlatformInfo) {
  // Purpose: Allow insertion with missing or non-object platform metadata.
  // Input: old=[]; append A without options, then B with numeric
  // __listItemPlatformInfo. Expected: Insertions contain only position=0/1 and
  // type=Card; no removes/updates.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  a->SetOptions(lepus::Value());
  b->SetOptions(MakeObject({{"__listItemPlatformInfo", lepus::Value(1)}}));
  ListUpdateInfoRecorder recorder(MakeChildren({}));
  recorder.OnInsertBefore(lepus::Value(a), lepus::Value());
  recorder.OnInsertBefore(lepus::Value(b), lepus::Value());
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({MakeObject({{"position", lepus::Value(0)},
                                    {"type", lepus::Value("Card")}}),
                        MakeObject({{"position", lepus::Value(1)},
                                    {"type", lepus::Value("Card")}})},
                       {}, {});
}

TEST_F(ListUpdateInfoRecorderTest, FlushInsertsBeforeAnchorInCallOrder) {
  // Purpose: Preserve call order and consecutive positions before one anchor.
  // Input: old=[A,B]; insert X then Y before B.
  // Expected: New order [A,X,Y,B]; insert=[X@1,Y@2]; no other actions.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto x = CreateItem("X");
  auto y = CreateItem("Y");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b}));
  recorder.OnInsertBefore(lepus::Value(x), lepus::Value(b));
  recorder.OnInsertBefore(lepus::Value(y), lepus::Value(b));
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({InsertAction("X", 1), InsertAction("Y", 2)}, {}, {});
}

TEST_F(ListUpdateInfoRecorderTest, FlushVisitsAnchorsInOldListOrder) {
  // Purpose: Emit anchor groups in old-list order, independent of call/hash
  // order. Input: old=[A,B,C] sharing one template; insert X before C, Y before
  // A, Z before B. Expected: New order [Y,A,Z,B,X,C]; insert=[Y@0,Z@2,X@4]; no
  // other actions.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  auto x = CreateItem("X");
  auto y = CreateItem("Y");
  auto z = CreateItem("Z");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b, c}));
  recorder.OnInsertBefore(lepus::Value(x), lepus::Value(c));
  recorder.OnInsertBefore(lepus::Value(y), lepus::Value(a));
  recorder.OnInsertBefore(lepus::Value(z), lepus::Value(b));
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo(
      {InsertAction("Y", 0), InsertAction("Z", 2), InsertAction("X", 4)}, {},
      {});
}

TEST_F(ListUpdateInfoRecorderTest, FlushInsertsBeforeRemovedAnchorAndAppends) {
  // Purpose: Retain insertions before a removed anchor and compute append
  // positions. Input: old=[A,B,C]; append D, insert X/Y/Z before B, then remove
  // B.
  //        X/D use Card; Y/Z use OtherCard.
  // Expected: New order [A,X,Y,Z,C,D]; insert=[X@1,Y@2,Z@3,D@5], remove=[1],
  //           update=[]; each insertion carries its own template type.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  auto d = CreateItem("D");
  auto x = CreateItem("X");
  auto y = CreateItem("Y");
  auto z = CreateItem("Z");
  y->SetTemplateKey(base::String("OtherCard"));
  z->SetTemplateKey(base::String("OtherCard"));
  ListUpdateInfoRecorder recorder(MakeChildren({a, b, c}));
  recorder.OnInsertBefore(lepus::Value(d), lepus::Value());
  recorder.OnInsertBefore(lepus::Value(x), lepus::Value(b));
  recorder.OnInsertBefore(lepus::Value(y), lepus::Value(b));
  recorder.OnInsertBefore(lepus::Value(z), lepus::Value(b));
  recorder.OnRemoveChild(lepus::Value(b));
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo(
      {InsertAction("X", 1), InsertAction("Y", 2, "OtherCard"),
       InsertAction("Z", 3, "OtherCard"), InsertAction("D", 5)},
      {1}, {});
}

TEST_F(ListUpdateInfoRecorderTest, FlushRemovalsUseOldIndicesAndDeduplicate) {
  // Purpose: Deduplicate removals and emit old indices in old-list order.
  // Input: old=[A,B,C,D]; remove D, A, then D again.
  // Expected: New order [B,C]; remove=[0,3]; no inserts/updates.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  auto d = CreateItem("D");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b, c, d}));
  recorder.OnRemoveChild(lepus::Value(d));
  recorder.OnRemoveChild(lepus::Value(a));
  recorder.OnRemoveChild(lepus::Value(d));
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({}, {0, 3}, {});
}

TEST_F(ListUpdateInfoRecorderTest,
       FlushPreservesMetadataAndOverridesReservedFields) {
  // Purpose: Preserve platform metadata while overriding reserved control
  // fields. Input: old=[A,B]; update B's item-key/full-span with incorrect
  // from/to/flush/type. Expected: item-key=B-latest, full-span=true, from=to=1,
  // flush=false, type=Card.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto platform_info = MakeObject({{"item-key", lepus::Value("B-latest")},
                                   {"full-span", lepus::Value(true)},
                                   {"from", lepus::Value(100)},
                                   {"to", lepus::Value(200)},
                                   {"flush", lepus::Value(true)},
                                   {"type", lepus::Value("wrong")}});
  ListUpdateInfoRecorder recorder(MakeChildren({a, b}));
  recorder.OnSetAttribute(lepus::Value(b), platform_info);

  ASSERT_TRUE(recorder.Flush(list_.get()));
  auto expected = MakeObject({{"item-key", lepus::Value("B-latest")},
                              {"full-span", lepus::Value(true)},
                              {"from", lepus::Value(1)},
                              {"to", lepus::Value(1)},
                              {"flush", lepus::Value(false)},
                              {"type", lepus::Value("Card")}});
  ExpectUpdateListInfo({}, {}, {expected});
}

TEST_F(ListUpdateInfoRecorderTest, FlushMultipleMetadataUpdatesInOldListOrder) {
  // Purpose: Emit item updates in old-list order, regardless of recording
  // order. Input: old=[A,B,C]; update C's full-span=false, then A's estimated
  // main-axis size. Expected: update=[A@0,C@2] with their metadata; no
  // inserts/removes.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  auto a_info =
      MakeObject({{"item-key", lepus::Value("A")},
                  {"estimated-main-axis-size-px", lepus::Value(150)}});
  auto c_info = MakeObject(
      {{"item-key", lepus::Value("C")}, {"full-span", lepus::Value(false)}});
  ListUpdateInfoRecorder recorder(MakeChildren({a, b, c}));
  recorder.OnSetAttribute(lepus::Value(c), c_info);
  recorder.OnSetAttribute(lepus::Value(a), a_info);

  ASSERT_TRUE(recorder.Flush(list_.get()));
  auto a_update =
      MakeObject({{"item-key", lepus::Value("A")},
                  {"estimated-main-axis-size-px", lepus::Value(150)},
                  {"from", lepus::Value(0)},
                  {"to", lepus::Value(0)},
                  {"flush", lepus::Value(false)},
                  {"type", lepus::Value("Card")}});
  auto c_update = MakeObject({{"item-key", lepus::Value("C")},
                              {"full-span", lepus::Value(false)},
                              {"from", lepus::Value(2)},
                              {"to", lepus::Value(2)},
                              {"flush", lepus::Value(false)},
                              {"type", lepus::Value("Card")}});
  ExpectUpdateListInfo({}, {}, {a_update, c_update});
}

TEST_F(ListUpdateInfoRecorderTest, FlushNormalizesNonObjectUpdateMetadata) {
  // Purpose: Emit required update control fields even with empty metadata.
  // Input: old=[A]; update A with nil.
  // Expected: update=[{from:0,to:0,flush:false,type:Card}]; no item-key or
  // other actions.
  auto a = CreateItem("A");
  ListUpdateInfoRecorder recorder(MakeChildren({a}));
  recorder.OnSetAttribute(lepus::Value(a), lepus::Value());
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({}, {},
                       {MakeObject({{"from", lepus::Value(0)},
                                    {"to", lepus::Value(0)},
                                    {"flush", lepus::Value(false)},
                                    {"type", lepus::Value("Card")}})});
}

TEST_F(ListUpdateInfoRecorderTest,
       FlushUsesTemplateIdentityForInsertAndUpdate) {
  // Purpose: Use template identity consistently for inserts/updates, excluding
  // UID/key. Input: A typed view, default-bundle Card, empty-bundle Card, and
  // external-bundle Card. Expected: Both action types use view, Card, Card, and
  // external:Card, respectively.
  auto a = CreateItem("a");
  auto b = CreateItem("b");
  auto c = CreateItem("c");
  auto d = CreateItem("d");
  a->SetTypedTag(base::String("view"));
  c->SetBundleUrl(base::String(""));
  d->SetBundleUrl(base::String("external"));
  auto children = MakeChildren({a, b, c, d});
  ListUpdateInfoRecorder insert_recorder(MakeChildren({}));
  for (const auto& item : {a, b, c, d}) {
    insert_recorder.OnInsertBefore(lepus::Value(item), lepus::Value());
  }
  ASSERT_TRUE(insert_recorder.Flush(list_.get()));
  ExpectUpdateListInfo(
      {InsertAction("a", 0, "view"), InsertAction("b", 1), InsertAction("c", 2),
       InsertAction("d", 3, "external:Card")},
      {}, {});

  ListUpdateInfoRecorder update_recorder(children);
  for (const auto& item : {a, b, c, d}) {
    update_recorder.OnSetAttribute(lepus::Value(item),
                                   item->GetListItemPlatformInfoFromOptions());
  }
  ASSERT_TRUE(update_recorder.Flush(list_.get()));
  ExpectUpdateListInfo(
      {}, {},
      {UpdateAction("a", 0, "view"), UpdateAction("b", 1), UpdateAction("c", 2),
       UpdateAction("d", 3, "external:Card")});
}

TEST_F(ListUpdateInfoRecorderTest, NewItemInsertionReadsLatestOptions) {
  // Purpose: Read new items' latest options at Flush without a separate
  // updateAction. Input: old=[]; append A, then set
  // item-key=A-latest/height=100 and incorrect
  //        position/type in its options.
  // Expected: insert=[A-latest@0] with height=100; no removes/updates.
  auto a = CreateItem("A");
  ListUpdateInfoRecorder recorder(MakeChildren({}));
  recorder.OnInsertBefore(lepus::Value(a), lepus::Value());
  auto latest = MakeObject({{"item-key", lepus::Value("A-latest")},
                            {"height", lepus::Value(100)},
                            {"position", lepus::Value(99)},
                            {"type", lepus::Value("wrong")}});
  // Insertions read item options; recorder.OnSetAttribute alone does not update
  // those options.
  a->SetListItemPlatformInfo(latest);
  recorder.OnSetAttribute(lepus::Value(a), latest);

  ASSERT_TRUE(recorder.Flush(list_.get()));
  auto expected = MakeObject({{"item-key", lepus::Value("A-latest")},
                              {"height", lepus::Value(100)},
                              {"position", lepus::Value(0)},
                              {"type", lepus::Value("Card")}});
  ExpectUpdateListInfo({expected}, {}, {});
  EXPECT_EQ(a->GetListItemPlatformInfoFromOptions(), latest);
}

TEST_F(ListUpdateInfoRecorderTest, RemovedItemDoesNotGenerateUpdateAction) {
  // Purpose: Suppress metadata updates for old items removed in the same batch.
  // Input: old=[A,B]; update A, then remove A.
  // Expected: remove=[0]; no inserts/updates that could target retained B.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b}));
  recorder.OnSetAttribute(lepus::Value(a),
                          MakeListItemPlatformInfo("A-latest"));
  recorder.OnRemoveChild(lepus::Value(a));

  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({}, {0}, {});
}

TEST_F(ListUpdateInfoRecorderTest,
       UpdatePositionIncludesInsertionsBeforeAnchor) {
  // Purpose: Include insertions before an updated anchor in its from/to
  // position. Input: old=[A,B]; insert X and Y before B, then update B.
  // Expected: insert=[X@1,Y@2], update=[B from=to=3,flush=false], remove=[].
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto x = CreateItem("X");
  auto y = CreateItem("Y");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b}));
  recorder.OnInsertBefore(lepus::Value(x), lepus::Value(b));
  recorder.OnInsertBefore(lepus::Value(y), lepus::Value(b));
  recorder.OnSetAttribute(lepus::Value(b), MakeListItemPlatformInfo("B"));

  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({InsertAction("X", 1), InsertAction("Y", 2)}, {},
                       {UpdateAction("B", 3)});
}

TEST_F(ListUpdateInfoRecorderTest, UpdatePositionAccountsForEarlierRemovals) {
  // Purpose: Shift update positions after earlier old items are removed.
  // Input: old=[A,B,C]; remove A and update C.
  // Expected: remove=[0], update=[C from=to=1,flush=false], insert=[].
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b, c}));
  recorder.OnRemoveChild(lepus::Value(a));
  recorder.OnSetAttribute(lepus::Value(c), MakeListItemPlatformInfo("C"));

  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({}, {0}, {UpdateAction("C", 1)});
}

TEST_F(ListUpdateInfoRecorderTest, MoveToEndUsesRemovalAndLatestInsertionInfo) {
  // Purpose: Express a parented item's move/update as removal plus insertion.
  // Input: old=[A,B]; update A's full-span, then append A to the end.
  // Expected: New order [B,A], remove=[0], update=[], and insert=[A@1] with
  // full-span=true.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto parent = CreateParent({a, b});
  ASSERT_EQ(a->GetLogicalParent(), parent.get());
  ListUpdateInfoRecorder recorder(MakeChildren({a, b}));
  auto latest = MakeListItemPlatformInfo("A", /*full_span=*/true);
  a->SetListItemPlatformInfo(latest);
  recorder.OnSetAttribute(lepus::Value(a), latest);
  recorder.OnInsertBefore(lepus::Value(a), lepus::Value());

  // OnInsertBefore uses the logical parent to record removal from the old
  // position; no explicit OnRemoveChild call is needed.
  EXPECT_EQ(recorder.remove_child().count(a), 1u);
  ASSERT_TRUE(recorder.Flush(list_.get()));
  auto expected = InsertAction("A", 1, "Card", /*full_span=*/true);
  ExpectUpdateListInfo({expected}, {0}, {});
}

TEST_F(ListUpdateInfoRecorderTest, MoveBeforeAnchorUsesOldRemovalIndex) {
  // Purpose: Use the new insertion position and old removal position for a
  // move. Input: Parented old=[A,B,C]; insert C before A. Expected: New order
  // [C,A,B]; insert=[C@0], remove=[2], update=[].
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  auto parent = CreateParent({a, b, c});
  ASSERT_EQ(c->GetLogicalParent(), parent.get());
  ListUpdateInfoRecorder recorder(MakeChildren({a, b, c}));
  recorder.OnInsertBefore(lepus::Value(c), lepus::Value(a));

  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({InsertAction("C", 0)}, {2}, {});
}

TEST_F(ListUpdateInfoRecorderTest, FlushMovesItemForwardBeforeRetainedAnchor) {
  // Purpose: Account for removal when moving an item before a retained later
  // anchor. Input: Parented old=[A,B,C,D]; insert B before D; C uses a
  // different template. Expected: New order [A,C,B,D]; insert=[B@2],
  // remove=[1], update=[].
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  auto d = CreateItem("D");
  c->SetTemplateKey(base::String("OtherCard"));
  auto parent = CreateParent({a, b, c, d});
  ASSERT_EQ(b->GetLogicalParent(), parent.get());
  ListUpdateInfoRecorder recorder(MakeChildren({a, b, c, d}));
  recorder.OnInsertBefore(lepus::Value(b), lepus::Value(d));

  // The logical parent makes insertion also record B's removal from the old
  // list.
  EXPECT_EQ(recorder.remove_child().count(b), 1u);
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({InsertAction("B", 2)}, {1}, {});
}

TEST_F(ListUpdateInfoRecorderTest,
       FlushReplacesAllItemsFromRecordedOperations) {
  // Purpose: Replace a same-template group with full removes/inserts and no
  // updates. Input: old=[A,B,C,D]; explicitly remove all, then append
  // [A1,B1,C1,D1]. Expected: remove=[0,1,2,3], insert=[A1@0,B1@1,C1@2,D1@3],
  // update=[]. The caller decides to replace items; the recorder does not
  // compare item-keys.
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  auto d = CreateItem("D");
  auto a1 = CreateItem("A1");
  auto b1 = CreateItem("B1");
  auto c1 = CreateItem("C1");
  auto d1 = CreateItem("D1");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b, c, d}));
  for (const auto& item : {a, b, c, d}) {
    recorder.OnRemoveChild(lepus::Value(item));
  }
  for (const auto& item : {a1, b1, c1, d1}) {
    recorder.OnInsertBefore(lepus::Value(item), lepus::Value());
  }

  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({InsertAction("A1", 0), InsertAction("B1", 1),
                        InsertAction("C1", 2), InsertAction("D1", 3)},
                       {0, 1, 2, 3}, {});
}

TEST_F(ListUpdateInfoRecorderTest, MixedOperationsUseCorrectOldAndNewIndices) {
  // Purpose: Keep action indices consistent across mixed operations in one
  // batch. Input: old=[A,B,C,D]; remove A/D, insert X before B, update B/C,
  // then append Y. Expected: New order [X,B,C,Y]; insert=[X@0,Y@3],
  // remove=[0,3], update=[B@1,C@2].
  auto a = CreateItem("A");
  auto b = CreateItem("B");
  auto c = CreateItem("C");
  auto d = CreateItem("D");
  auto x = CreateItem("X");
  auto y = CreateItem("Y");
  ListUpdateInfoRecorder recorder(MakeChildren({a, b, c, d}));
  recorder.OnRemoveChild(lepus::Value(d));
  recorder.OnInsertBefore(lepus::Value(x), lepus::Value(b));
  recorder.OnSetAttribute(lepus::Value(c), MakeListItemPlatformInfo("C"));
  recorder.OnRemoveChild(lepus::Value(a));
  recorder.OnSetAttribute(lepus::Value(b), MakeListItemPlatformInfo("B"));
  recorder.OnInsertBefore(lepus::Value(y), lepus::Value());

  // Removals use old indices 0/3; updates for B/C use new positions 1/2.
  ASSERT_TRUE(recorder.Flush(list_.get()));
  ExpectUpdateListInfo({InsertAction("X", 0), InsertAction("Y", 3)}, {0, 3},
                       {UpdateAction("B", 1), UpdateAction("C", 2)});
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
