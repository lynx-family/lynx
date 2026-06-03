// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/decoupled_list_event_manager.h"

#include "core/list/decoupled_list_container_impl.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "testing/fiber_data_source.h"
#include "testing/mock_list_element.h"
#include "testing/mock_list_item_element.h"
#include "testing/radon_data_source.h"
#include "testing/utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {

using ::testing::_;
using ::testing::Return;

class ListEventManagerTest : public ::testing::Test {
 public:
  ListEventManagerTest() = default;
  ~ListEventManagerTest() override = default;

  std::unique_ptr<MockListElement> mock_list_element_{nullptr};
  std::unique_ptr<ListContainerImpl> list_container_impl_{nullptr};
  std::shared_ptr<pub::PubValueFactoryDefault> value_factory_{nullptr};
  ListLayoutManager* list_layout_manager_{nullptr};
  ListEventManager* list_event_manager_{nullptr};
  ListChildrenHelper* list_children_helper_{nullptr};
  ListAdapter* list_adapter_{nullptr};

  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    list_container_impl_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
    list_layout_manager_ = list_container_impl_->list_layout_manager();
    list_event_manager_ = list_container_impl_->list_event_manager();
    list_children_helper_ = list_container_impl_->list_children_helper();
    list_adapter_ = list_container_impl_->list_adapter();
  }

  void InitFiberDataSourceAndLayout(int layout_id,
                                    bool need_layout_complete_info,
                                    bool need_visible_item_info,
                                    int scroll_event_throttle_ms = 16) {
    testing::InsertAction insert_action{
        .insert_ops_ = {
            {.position_ = 0, "A_0", 100, false, false, false, false},
            {.position_ = 1, "B_1", 100, false, false, false, false},
            {.position_ = 2, "C_2", 100, false, false, false, false},
            {.position_ = 3, "D_3", 100, false, false, false, false},
            {.position_ = 4, "E_4", 100, false, false, false, false},
            {.position_ = 5, "F_5", 100, false, false, false, false},
            {.position_ = 6, "G_6", 100, false, false, false, false},
            {.position_ = 7, "H_7", 100, false, false, false, false},
            {.position_ = 8, "I_8", 100, false, false, false, false},
            {.position_ = 9, "J_9", 100, false, false, false, false},
        }};
    testing::FiberDataSource fiber_data_source{
        .insert_action_ = insert_action,
    };
    auto update_list_info_key =
        value_factory_->CreateString(kPropFiberUpdateListInfo);
    auto update_list_info_value =
        pub::ValueImplLepus(lepus::Value(fiber_data_source.Resolve()));
    list_container_impl_->ResolveAttribute(*update_list_info_key,
                                           update_list_info_value);
    auto layout_id_key = value_factory_->CreateString(kPropLayoutId);
    auto layout_id_value = value_factory_->CreateNumber(layout_id);
    list_container_impl_->ResolveAttribute(*layout_id_key, *layout_id_value);
    auto need_layout_complete_info_key =
        value_factory_->CreateString(kPropNeedLayoutCompleteInfo);
    auto need_layout_complete_info_value =
        value_factory_->CreateBool(need_layout_complete_info);
    list_container_impl_->ResolveAttribute(*need_layout_complete_info_key,
                                           *need_layout_complete_info_value);
    auto need_visible_item_info_key =
        value_factory_->CreateString(kPropNeedVisibleItemInfo);
    auto need_visible_item_info_value =
        value_factory_->CreateBool(need_visible_item_info);
    list_container_impl_->ResolveAttribute(*need_visible_item_info_key,
                                           *need_visible_item_info_value);
    auto scroll_event_throttle_ms_key =
        value_factory_->CreateString(kPropScrollEventThrottle);
    auto scroll_event_throttle_ms_value =
        value_factory_->CreateNumber(scroll_event_throttle_ms);
    list_container_impl_->ResolveAttribute(*scroll_event_throttle_ms_key,
                                           *scroll_event_throttle_ms_value);
    list_container_impl_->PropsUpdateFinish();
    list_layout_manager_->LayoutInvalidItemHolder(0);
    list_layout_manager_->content_size_ =
        list_layout_manager_->GetTargetContentSize();
    list_children_helper_->UpdateOnScreenChildren(
        list_layout_manager_->list_orientation_helper_.get(),
        list_layout_manager_->content_offset_);
    {
      // Note: because all item's real size is equal to estimated size, so we
      // can bind all visible item holders directly.
      list_container_impl_->StartInterceptListElementUpdated();
      int list_item_impl_id = mock_list_element_->GetImplId() + 1;
      list_children_helper_->ForEachChild(
          list_children_helper_->on_screen_children(),
          [this, &list_item_impl_id](ItemHolder* item_holder) {
            EXPECT_CALL(*mock_list_element_, ComponentAtIndex(_, _, _))
                .WillOnce(Return(true));
            list_adapter_->BindItemHolder(item_holder, item_holder->index());
            auto pipeline = std::make_shared<tasm::PipelineOptions>();
            pipeline->operation_id = item_holder->operation_id_;
            const auto& item_key = item_holder->item_key();
            mock_list_element_->AddListItemElement(
                item_key,
                std::make_unique<MockListItemElement>(list_item_impl_id++));
            list_adapter_->OnFinishBindItemHolder(
                mock_list_element_->GetListItemElement(item_key), pipeline);
            return false;
          });
      list_container_impl_->StopInterceptListElementUpdated();
    }
  }

  void CheckScrollInfo(const std::unique_ptr<pub::Value>& scroll_info_value,
                       float scroll_left, float scroll_top, float scroll_width,
                       float scroll_height, float list_width, float list_height,
                       float delta_x, float delta_y) {
    EXPECT_TRUE(scroll_info_value && scroll_info_value->IsMap());
    auto scroll_left_value =
        scroll_info_value->GetValueForKey(kScrollInfoScrollLeft);
    EXPECT_TRUE(
        scroll_left_value && scroll_left_value->IsNumber() &&
        base::FloatsEqual(static_cast<double>(scroll_left_value->Number()),
                          scroll_left));
    auto scroll_top_value =
        scroll_info_value->GetValueForKey(kScrollInfoScrollTop);
    EXPECT_TRUE(
        scroll_top_value && scroll_top_value->IsNumber() &&
        base::FloatsEqual(static_cast<double>(scroll_top_value->Number()),
                          scroll_top));
    auto scroll_width_value =
        scroll_info_value->GetValueForKey(kScrollInfoScrollWidth);
    EXPECT_TRUE(
        scroll_width_value && scroll_width_value->IsNumber() &&
        base::FloatsEqual(static_cast<double>(scroll_width_value->Number()),
                          scroll_width));
    auto scroll_height_value =
        scroll_info_value->GetValueForKey(kScrollInfoScrollHeight);
    EXPECT_TRUE(
        scroll_height_value && scroll_height_value->IsNumber() &&
        base::FloatsEqual(static_cast<double>(scroll_height_value->Number()),
                          scroll_height));
    auto list_width_value =
        scroll_info_value->GetValueForKey(kScrollInfoListWidth);
    EXPECT_TRUE(
        list_width_value && list_width_value->IsNumber() &&
        base::FloatsEqual(static_cast<double>(list_width_value->Number()),
                          list_width));
    auto list_height_value =
        scroll_info_value->GetValueForKey(kScrollInfoListHeight);
    EXPECT_TRUE(
        list_height_value && list_height_value->IsNumber() &&
        base::FloatsEqual(static_cast<double>(list_height_value->Number()),
                          list_height));
    auto scroll_delta_x_value =
        scroll_info_value->GetValueForKey(kScrollInfoDeltaX);
    EXPECT_TRUE(
        scroll_delta_x_value && scroll_delta_x_value->IsNumber() &&
        base::FloatsEqual(static_cast<double>(scroll_delta_x_value->Number()),
                          delta_x));
    auto scroll_delta_y_value =
        scroll_info_value->GetValueForKey(kScrollInfoDeltaY);
    EXPECT_TRUE(
        scroll_delta_y_value && scroll_delta_y_value->IsNumber() &&
        base::FloatsEqual(static_cast<double>(scroll_delta_y_value->Number()),
                          delta_y));
  }

  void CheckAllVisibleCellInfo(
      const std::unique_ptr<pub::Value>& visible_cell_info, float scroll_left,
      float scroll_top, bool for_scroll_info) {
    EXPECT_TRUE(visible_cell_info && visible_cell_info->IsArray());
    EXPECT_EQ(visible_cell_info->Length(),
              list_children_helper_->on_screen_children_.size());
    auto it = list_children_helper_->on_screen_children_.begin();
    visible_cell_info->ForeachArray(
        [this, scroll_left, scroll_top, for_scroll_info, &it](
            int64_t index, const pub::Value& cell_info) {
          ItemHolder* item_holder = *(it++);
          CheckCellInfo(cell_info, item_holder, scroll_left, scroll_top,
                        for_scroll_info);
        });
  }

  void CheckCellInfo(const pub::Value& cell_info, ItemHolder* item_holder,
                     float scroll_left, float scroll_top,
                     bool for_scroll_info) {
    EXPECT_TRUE(cell_info.IsMap());
    auto index_value = cell_info.GetValueForKey(kCellInfoIndex);
    EXPECT_TRUE(index_value && index_value->IsInt32() &&
                index_value->Int32() == item_holder->index());
    auto item_key_value = cell_info.GetValueForKey(kCellInfoItemKey);
    EXPECT_TRUE(item_key_value && item_key_value->IsString() &&
                item_key_value->str() == item_holder->item_key());
    ItemElementDelegate* list_item_delegate =
        list_adapter_->GetItemElementDelegate(item_holder);
    float layouts_unit_per_px =
        list_container_impl_->list_delegate()->GetLayoutsUnitPerPx();
    if (for_scroll_info) {
      auto id_value = cell_info.GetValueForKey(kCellInfoIdSelector);
      EXPECT_TRUE(id_value && id_value->IsString() &&
                  id_value->str() == list_item_delegate->GetIdSelector());
      auto top_value = cell_info.GetValueForKey(kCellInfoTop);
      EXPECT_TRUE(top_value && top_value->IsNumber() &&
                  base::FloatsEqual(
                      static_cast<double>(top_value->Number()),
                      (item_holder->top() - scroll_top) / layouts_unit_per_px));
      auto bottom_value = cell_info.GetValueForKey(kCellInfoBottom);
      EXPECT_TRUE(
          bottom_value && bottom_value->IsNumber() &&
          base::FloatsEqual(static_cast<double>(bottom_value->Number()),
                            (item_holder->top() + item_holder->height()) /
                                layouts_unit_per_px));
      auto left_value = cell_info.GetValueForKey(kCellInfoLeft);
      EXPECT_TRUE(left_value && left_value->IsNumber() &&
                  base::FloatsEqual(static_cast<double>(left_value->Number()),
                                    (item_holder->left() - scroll_left) /
                                        layouts_unit_per_px));
      auto right_value = cell_info.GetValueForKey(kCellInfoRight);
      EXPECT_TRUE(
          right_value && right_value->IsNumber() &&
          base::FloatsEqual(static_cast<double>(right_value->Number()),
                            (item_holder->left() + item_holder->width()) /
                                layouts_unit_per_px));
      auto position_value = cell_info.GetValueForKey(kCellInfoPosition);
      EXPECT_TRUE(position_value && position_value->IsInt32() &&
                  position_value->Int32() == item_holder->index());
    } else {
      auto origin_x_value = cell_info.GetValueForKey(kCellInfoOriginX);
      EXPECT_TRUE(
          origin_x_value && origin_x_value->IsNumber() &&
          base::FloatsEqual(static_cast<double>(origin_x_value->Number()),
                            item_holder->left() / layouts_unit_per_px));
      auto origin_y_value = cell_info.GetValueForKey(kCellInfoOriginY);
      EXPECT_TRUE(
          origin_y_value && origin_y_value->IsNumber() &&
          base::FloatsEqual(static_cast<double>(origin_y_value->Number()),
                            item_holder->top() / layouts_unit_per_px));
      auto width_value = cell_info.GetValueForKey(kCellInfoWidth);
      EXPECT_TRUE(
          width_value && width_value->IsNumber() &&
          base::FloatsEqual(static_cast<double>(width_value->Number()),
                            item_holder->width() / layouts_unit_per_px));
      auto height_value = cell_info.GetValueForKey(kCellInfoHeight);
      EXPECT_TRUE(
          height_value && height_value->IsNumber() &&
          base::FloatsEqual(static_cast<double>(height_value->Number()),
                            item_holder->height() / layouts_unit_per_px));
      auto is_binding_value = cell_info.GetValueForKey(kCellInfoIsBinding);
      EXPECT_TRUE(is_binding_value && is_binding_value->IsBool() &&
                  is_binding_value->Bool() ==
                      list_adapter_->IsBinding(item_holder));
      auto is_updated_value = cell_info.GetValueForKey(kCellInfoUpdated);
      EXPECT_TRUE(is_updated_value && is_updated_value->IsBool() &&
                  is_updated_value->Bool() ==
                      list_adapter_->IsUpdated(item_holder));
    }
  }
};

TEST_F(ListEventManagerTest, HasBoundEvent) {
  mock_list_element_->events_ = {"scroll",
                                 "layoutcomplete",
                                 "scrolltoupper",
                                 "scrolltolower",
                                 "scrolltoupperedge",
                                 "scrolltoloweredge",
                                 "scrolltonormalstate",
                                 "listdebuginfo"};
  EXPECT_TRUE(mock_list_element_->HasBoundEvent(kEventScroll));
  EXPECT_TRUE(mock_list_element_->HasBoundEvent(kEventLayoutComplete));
  EXPECT_TRUE(mock_list_element_->HasBoundEvent(kEventScrollToUpper));
  EXPECT_TRUE(mock_list_element_->HasBoundEvent(kEventScrollToLower));
  EXPECT_TRUE(mock_list_element_->HasBoundEvent(kEventScrollToUpperEdge));
  EXPECT_TRUE(mock_list_element_->HasBoundEvent(kEventScrollToLowerEdge));
  EXPECT_TRUE(mock_list_element_->HasBoundEvent(kEventScrollToNormalState));
  EXPECT_TRUE(mock_list_element_->HasBoundEvent(kEventListDebugInfo));
}

TEST_F(ListEventManagerTest, SendLayoutCompleteEvent0) {
  // 1. No bind layout complete event.
  InitFiberDataSourceAndLayout(1, false, false);
  mock_list_element_->events_ = {};
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _)).Times(0);
  list_event_manager_->SendLayoutCompleteEvent();
}

TEST_F(ListEventManagerTest, SendLayoutCompleteEvent1) {
  // 2. Bind layout complete event.
  int layout_id = 1;
  InitFiberDataSourceAndLayout(layout_id, true, false);
  mock_list_element_->events_ = {"layoutcomplete"};
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventLayoutComplete);
        EXPECT_EQ(param_name, kEventParamDetail);
        EXPECT_TRUE(param->IsMap());
        // check layout id
        auto layout_id_value = param->GetValueForKey(kLayoutInfoLayoutId);
        EXPECT_TRUE(layout_id_value && layout_id_value->IsNumber() &&
                    static_cast<int>(layout_id_value->Number()) == layout_id);
        // check scroll info
        auto scroll_info_value = param->GetValueForKey(kLayoutInfoScrollInfo);
        float scroll_left = 0.f;
        float scroll_top = list_layout_manager_->content_offset();
        float scroll_width = mock_list_element_->GetWidth();
        float scroll_height = list_layout_manager_->content_size();
        float list_width = mock_list_element_->GetWidth();
        float list_height = mock_list_element_->GetHeight();
        float delta_x = 0.f;
        float delta_y = 0.f;
        CheckScrollInfo(scroll_info_value, scroll_left, scroll_top,
                        scroll_width, scroll_height, list_width, list_height,
                        delta_x, delta_y);
      });
  list_event_manager_->SendLayoutCompleteEvent();
}

TEST_F(ListEventManagerTest, SendCustomScrollEvent) {
  int layout_id = 1;
  int scroll_event_threshold = 20;
  float distance = 10.f;
  InitFiberDataSourceAndLayout(layout_id, false, true, scroll_event_threshold);
  mock_list_element_->events_ = {"scroll"};
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScroll);
        EXPECT_EQ(param_name, kEventParamDetail);
        EXPECT_TRUE(param->IsMap());
        // check event source
        auto event_source_value = param->GetValueForKey(kScrollInfoEventSource);
        EXPECT_TRUE(event_source_value && event_source_value->IsNumber() &&
                    static_cast<int>(event_source_value->Number()) ==
                        static_cast<int>(EventSource::kScroll));
        // check scroll info
        float scroll_left = 0.f;
        float scroll_top = list_layout_manager_->content_offset();
        float scroll_width = mock_list_element_->GetWidth();
        float scroll_height = list_layout_manager_->content_size();
        float list_width = mock_list_element_->GetWidth();
        float list_height = mock_list_element_->GetHeight();
        float delta_x = 0.f;
        float delta_y = distance;
        CheckScrollInfo(param, scroll_left, scroll_top, scroll_width,
                        scroll_height, list_width, list_height, delta_x,
                        delta_y);
        // check visible cell info
        auto visible_cell_info =
            param->GetValueForKey(kScrollInfoAttachedCells);
        CheckAllVisibleCellInfo(visible_cell_info, scroll_left, scroll_top,
                                true);
      });
  list_event_manager_->SendCustomScrollEvent(kEventScroll, distance,
                                             EventSource::kScroll);
}

TEST_F(ListEventManagerTest, SendAnchorDebugInfoIfNeeded) {
  int layout_id = 1;
  InitFiberDataSourceAndLayout(layout_id, true, false);
  mock_list_element_->is_debug_mode_ = true;
  mock_list_element_->events_ = {kEventListDebugInfo};
  // Construct anchor info.
  ListAnchorManager::AnchorInfo anchor_info;
  int anchor_index = 1;
  ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(anchor_index);
  anchor_info.valid_ = true;
  anchor_info.index_ = anchor_index;
  anchor_info.start_offset_ = 100.f;
  anchor_info.start_alignment_delta_ = 10.f;
  anchor_info.item_holder_ = item_holder;
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventListDebugInfo);
        EXPECT_EQ(param_name, kEventParamDetail);
        EXPECT_TRUE(param->IsMap());
        auto anchor_info_map = param->GetValueForKey(kDebugInfoAnchorInfo);
        EXPECT_TRUE(anchor_info_map && anchor_info_map->IsMap());
        auto index_value =
            anchor_info_map->GetValueForKey(kDebugInfoAnchorIndex);
        EXPECT_TRUE(index_value && index_value->IsInt32() &&
                    index_value->Int32() == anchor_index);
        auto start_offset =
            anchor_info_map->GetValueForKey(kDebugInfoAnchorStartOffset);
        EXPECT_TRUE(
            start_offset && start_offset->IsNumber() &&
            base::FloatsEqual(static_cast<double>(start_offset->Number()),
                              anchor_info.start_offset_));
        auto delta_value = anchor_info_map->GetValueForKey(
            kDebugInfoAnchorStartAlignmentDelta);
        EXPECT_TRUE(
            delta_value && delta_value->IsNumber() &&
            base::FloatsEqual(static_cast<double>(delta_value->Number()),
                              anchor_info.start_alignment_delta_));
        auto dirty_value =
            anchor_info_map->GetValueForKey(kDebugInfoAnchorDirty);
        EXPECT_TRUE(dirty_value && dirty_value->IsBool() &&
                    dirty_value->Bool() == list_adapter_->IsDirty(item_holder));
        auto binding_value =
            anchor_info_map->GetValueForKey(kDebugInfoAnchorBinding);
        EXPECT_TRUE(binding_value && binding_value->IsBool() &&
                    binding_value->Bool() ==
                        list_adapter_->IsBinding(item_holder));
      });
  list_event_manager_->SendAnchorDebugInfoIfNeeded(anchor_info);
}

TEST_F(ListEventManagerTest, SendExposureEvent) {
  int layout_id = 1;
  InitFiberDataSourceAndLayout(layout_id, true, false);
  ItemHolder* appear_item_holder = list_adapter_->GetItemHolderForIndex(0);
  ItemHolder* disappear_item_holder = list_adapter_->GetItemHolderForIndex(1);
  ItemElementDelegate* appear_item_element =
      list_adapter_->GetItemElementDelegate(appear_item_holder);
  ItemElementDelegate* disappear_item_element =
      list_adapter_->GetItemElementDelegate(disappear_item_holder);
  TO_MOCK_LIST_ITEM_ELEMENT(appear_item_element)->events_ = {
      kEventNodeAppear, kEventNodeDisappear};
  TO_MOCK_LIST_ITEM_ELEMENT(disappear_item_element)->events_ = {
      kEventNodeAppear, kEventNodeDisappear};
  EXPECT_CALL(*TO_MOCK_LIST_ITEM_ELEMENT(appear_item_element),
              SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventNodeAppear);
        EXPECT_EQ(param_name, kEventParamDetail);
        EXPECT_TRUE(param->IsMap());
        auto index_value = param->GetValueForKey(kCellInfoIndex);
        EXPECT_TRUE(index_value && index_value->IsInt32() &&
                    index_value->Int32() == appear_item_holder->index());
        auto item_key_value = param->GetValueForKey(kCellInfoItemKey);
        EXPECT_TRUE(item_key_value && item_key_value->IsString() &&
                    item_key_value->str() == appear_item_holder->item_key());
      });
  list_event_manager_->SendExposureEvent(kEventNodeAppear, appear_item_holder);
  EXPECT_CALL(*TO_MOCK_LIST_ITEM_ELEMENT(disappear_item_element),
              SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventNodeDisappear);
        EXPECT_EQ(param_name, kEventParamDetail);
        EXPECT_TRUE(param->IsMap());
        auto index_value = param->GetValueForKey(kCellInfoIndex);
        EXPECT_TRUE(index_value && index_value->IsInt32() &&
                    index_value->Int32() == disappear_item_holder->index());
        auto item_key_value = param->GetValueForKey(kCellInfoItemKey);
        EXPECT_TRUE(item_key_value && item_key_value->IsString() &&
                    item_key_value->str() == disappear_item_holder->item_key());
      });
  list_event_manager_->SendExposureEvent(kEventNodeDisappear,
                                         disappear_item_holder);
}

TEST_F(ListEventManagerTest, RecordDiffResultIfNeeded) {
  int layout_id = 1;
  InitFiberDataSourceAndLayout(layout_id, true, false);
  const auto& layout_complete_info = list_event_manager_->layout_complete_info_;
  EXPECT_TRUE(layout_complete_info && layout_complete_info->IsMap());
  auto diff_result_value =
      layout_complete_info->GetValueForKey(kLayoutInfoDiffResult);
  EXPECT_TRUE(diff_result_value && diff_result_value->IsMap());
  auto insert_value = diff_result_value->GetValueForKey(kDiffInfoInsertion);
  EXPECT_TRUE(insert_value && insert_value->IsArray());
  auto remove_value = diff_result_value->GetValueForKey(kDiffInfoRemoval);
  EXPECT_TRUE(remove_value && remove_value->IsArray());
  auto update_from_value =
      diff_result_value->GetValueForKey(kDiffInfoUpdateFrom);
  EXPECT_TRUE(update_from_value && update_from_value->IsArray());
  auto update_to_value = diff_result_value->GetValueForKey(kDiffInfoUpdateTo);
  EXPECT_TRUE(update_to_value && update_to_value->IsArray());
  auto move_from_value = diff_result_value->GetValueForKey(kDiffInfoMoveFrom);
  EXPECT_TRUE(move_from_value && move_from_value->IsArray());
  auto move_to_value = diff_result_value->GetValueForKey(kDiffInfoMoveTo);
  EXPECT_TRUE(move_to_value && move_to_value->IsArray());
}

TEST_F(ListEventManagerTest, SendDiffDebugEventIfNeeded) {
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventListDebugInfo);
        EXPECT_EQ(param_name, kEventParamDetail);
        EXPECT_TRUE(param->IsMap());
        auto diff_result_value = param->GetValueForKey(kDebugInfoDiffResult);
        EXPECT_TRUE(diff_result_value && diff_result_value->IsMap());
      });
  int layout_id = 1;
  mock_list_element_->is_debug_mode_ = true;
  mock_list_element_->events_ = {kEventListDebugInfo};
  InitFiberDataSourceAndLayout(layout_id, true, false);
}

TEST_F(ListEventManagerTest, DetectScrollToThresholdAndSend0) {
  int layout_id = 1;
  // let list_size > content_size
  mock_list_element_->height_ = 5000;
  mock_list_element_->events_ = {kEventScrollToUpper, kEventScrollToLower,
                                 kEventScrollToUpperEdge,
                                 kEventScrollToLowerEdge};
  InitFiberDataSourceAndLayout(layout_id, true, false);
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToUpper);
      })
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToLower);
      })
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToUpperEdge);
      })
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToLowerEdge);
      });
  list_event_manager_->DetectScrollToThresholdAndSend(0.f, 0.f,
                                                      EventSource::kScroll);
  EXPECT_TRUE(list_event_manager_->previous_scroll_pos_state_ ==
              ScrollPositionState::kBothEdge);
}

TEST_F(ListEventManagerTest, DetectScrollToThresholdAndSend1) {
  int layout_id = 1;
  mock_list_element_->events_ = {kEventScrollToUpper, kEventScrollToLower,
                                 kEventScrollToUpperEdge,
                                 kEventScrollToLowerEdge};
  InitFiberDataSourceAndLayout(layout_id, true, false);

  // list_size: 500, content_size: 1000, content_offset: 0, original_offset: 0
  mock_list_element_->height_ = 500;
  list_layout_manager_->content_offset_ = 0.f;
  float original_offset = 0.f;
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToUpper);
      })
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToUpperEdge);
      });
  list_event_manager_->DetectScrollToThresholdAndSend(0.f, original_offset,
                                                      EventSource::kScroll);
  EXPECT_TRUE(list_event_manager_->previous_scroll_pos_state_ ==
              ScrollPositionState::kUpper);

  // list_size: 500, content_size: 1000, content_offset: 500, original_offset:
  // 500
  mock_list_element_->height_ = 500.f;
  list_layout_manager_->content_offset_ = 500.f;
  original_offset = 500.f;
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToLower);
      })
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToLowerEdge);
      });
  list_event_manager_->DetectScrollToThresholdAndSend(0.f, original_offset,
                                                      EventSource::kScroll);
  EXPECT_TRUE(list_event_manager_->previous_scroll_pos_state_ ==
              ScrollPositionState::kLower);

  // list_size: 500, content_size: 1000, content_offset: 0, original_offset: -20
  // at bounce area
  mock_list_element_->height_ = 500.f;
  list_layout_manager_->content_offset_ = 0.f;
  original_offset = -20.f;
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToUpper);
      });
  list_event_manager_->DetectScrollToThresholdAndSend(0.f, original_offset,
                                                      EventSource::kScroll);
  EXPECT_TRUE(list_event_manager_->previous_scroll_pos_state_ ==
              ScrollPositionState::kUpper);
  // bounce back to 0
  list_layout_manager_->content_offset_ = 0.f;
  original_offset = 0.f;
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToUpperEdge);
      });
  list_event_manager_->DetectScrollToThresholdAndSend(0.f, original_offset,
                                                      EventSource::kScroll);
  EXPECT_TRUE(list_event_manager_->previous_scroll_pos_state_ ==
              ScrollPositionState::kUpper);

  // list_size: 500, content_size: 1000, content_offset: 500, original_offset:
  // 520 at bounce area
  mock_list_element_->height_ = 500.f;
  list_layout_manager_->content_offset_ = 500.f;
  original_offset = 520.f;
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToLower);
      });
  list_event_manager_->DetectScrollToThresholdAndSend(0.f, original_offset,
                                                      EventSource::kScroll);
  EXPECT_TRUE(list_event_manager_->previous_scroll_pos_state_ ==
              ScrollPositionState::kLower);
  // bounce back to 500
  list_layout_manager_->content_offset_ = 500.f;
  original_offset = 500.f;
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _))
      .WillOnce([&](const std::string& event_name,
                    const std::string& param_name,
                    std::unique_ptr<pub::Value> param) {
        EXPECT_EQ(event_name, kEventScrollToLowerEdge);
      });
  list_event_manager_->DetectScrollToThresholdAndSend(0.f, original_offset,
                                                      EventSource::kScroll);
  EXPECT_TRUE(list_event_manager_->previous_scroll_pos_state_ ==
              ScrollPositionState::kLower);
}

TEST_F(ListEventManagerTest, DetectScrollToThresholdAndSend2) {
  // Disabling the switch suppresses scrolltoupper/scrolltolower for diff/layout
  // sources when the list reaches the corresponding threshold.
  int layout_id = 1;
  mock_list_element_->events_ = {kEventScrollToUpper, kEventScrollToLower};
  InitFiberDataSourceAndLayout(layout_id, true, false);
  list_event_manager_->SetEnableScrollToThresholdEventOnDiffLayout(false);

  // Only scrolltoupper/scrolltolower are bound in this test. Any custom event
  // means the switch failed to suppress threshold events.
  EXPECT_CALL(*mock_list_element_, SendCustomEvent(_, _, _)).Times(0);

  // At the top edge, diff should not send scrolltoupper.
  mock_list_element_->height_ = 500.f;
  list_layout_manager_->content_offset_ = 0.f;
  list_event_manager_->DetectScrollToThresholdAndSend(0.f, 0.f,
                                                      EventSource::kDiff);

  // At the bottom edge, layout should not send scrolltolower.
  list_layout_manager_->content_offset_ = 500.f;
  list_event_manager_->DetectScrollToThresholdAndSend(0.f, 500.f,
                                                      EventSource::kLayout);
  // Diff/layout threshold checks should not update the scroll-state machine.
  EXPECT_TRUE(list_event_manager_->previous_scroll_pos_state_ ==
              ScrollPositionState::kMiddle);
}

}  // namespace list
}  // namespace lynx
