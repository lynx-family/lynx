// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_card_cache_data_manager.h"

#include "core/renderer/data/template_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

class LynxCardCacheDataManagerTest : public ::testing::Test {
 protected:
  LynxCardCacheDataManagerTest() = default;
  ~LynxCardCacheDataManagerTest() override = default;
};

TEST_F(LynxCardCacheDataManagerTest, GetCardCacheData) {
  LynxCardCacheDataManager data_mgr;
  LynxCardCacheDataManager data_mgr_tmp;
  ASSERT_TRUE(data_mgr.GetCardCacheData().empty());
  ASSERT_TRUE(data_mgr_tmp.GetCardCacheData().empty());

  data_mgr.AddCardCacheData({lepus::Value("test data 1"), true, ""},
                            CacheDataType::UPDATE);
  data_mgr_tmp.AddCardCacheData({lepus::Value("test data 1"), false, ""},
                                CacheDataType::UPDATE);

  ASSERT_EQ(data_mgr.GetCardCacheData(), data_mgr_tmp.GetCardCacheData());

  data_mgr.AddCardCacheData({lepus::Value("test data 2"), true, ""},
                            CacheDataType::UPDATE);

  data_mgr_tmp.AddCardCacheData({lepus::Value("test data 2"), false, ""},
                                CacheDataType::UPDATE);

  ASSERT_EQ(data_mgr.GetCardCacheData(), data_mgr_tmp.GetCardCacheData());

  data_mgr.AddCardCacheData({lepus::Value("test data 3"), true, ""},
                            CacheDataType::UPDATE);
  data_mgr_tmp.AddCardCacheData({lepus::Value("test data 3"), false, "zzzzz"},
                                CacheDataType::UPDATE);
  ASSERT_EQ(data_mgr.GetCardCacheData(), data_mgr_tmp.GetCardCacheData());
}

TEST_F(LynxCardCacheDataManagerTest, ObtainCardCacheData) {
  LynxCardCacheDataManager data_mgr;
  LynxCardCacheDataManager data_mgr_tmp;
  ASSERT_TRUE(data_mgr.ObtainCardCacheData().empty());
  ASSERT_TRUE(data_mgr_tmp.ObtainCardCacheData().empty());

  data_mgr.AddCardCacheData({lepus::Value("test data 1"), true, ""},
                            CacheDataType::UPDATE);
  data_mgr_tmp.AddCardCacheData({lepus::Value("test data 1"), false, ""},
                                CacheDataType::UPDATE);

  ASSERT_EQ(data_mgr.ObtainCardCacheData(), data_mgr_tmp.ObtainCardCacheData());

  data_mgr.AddCardCacheData({lepus::Value("test data 2"), true, ""},
                            CacheDataType::UPDATE);
  data_mgr_tmp.AddCardCacheData({lepus::Value("test data 2"), false, "zzzz"},
                                CacheDataType::UPDATE);

  ASSERT_EQ(data_mgr.ObtainCardCacheData(), data_mgr_tmp.ObtainCardCacheData());

  data_mgr.AddCardCacheData({lepus::Value("test data 3"), true, ""},
                            CacheDataType::UPDATE);
  data_mgr_tmp.AddCardCacheData({lepus::Value("test data 3"), false, "zzzz"},
                                CacheDataType::UPDATE);

  ASSERT_EQ(data_mgr.ObtainCardCacheData(), data_mgr_tmp.ObtainCardCacheData());
}

TEST_F(LynxCardCacheDataManagerTest, CacheDataOp) {
  CacheDataOp op({lepus::Value("test data 3"), false, "zzzz"},
                 CacheDataType::UPDATE);
  const auto& op_temp = CacheDataOp::DeepClone(op);

  EXPECT_EQ(op, op_temp);
  EXPECT_EQ(op.ProcessorName(), op_temp.ProcessorName());
}

}  // namespace shell
}  // namespace lynx
