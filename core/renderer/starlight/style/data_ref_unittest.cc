// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/style/data_ref.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace starlight {

class StyleData : public fml::RefCountedThreadSafeStorage {
 public:
  void ReleaseSelf() const override { delete this; }
  static fml::RefPtr<StyleData> Create() {
    return fml::AdoptRef(new StyleData());
  }
  fml::RefPtr<StyleData> Copy() const {
    return fml::AdoptRef(new StyleData(*this));
  }

  StyleData() : value(1) {}

  StyleData(const StyleData& data) : value(data.value) {}

  ~StyleData() override { destroyed_count_++; };

  int value = 0;
  // Debug lifecycle
  static int destroyed_count() { return destroyed_count_; }
  static void reset_destroyed() { destroyed_count_ = 0; }

 private:
  static int destroyed_count_;
};

int StyleData::destroyed_count_ = 0;

TEST(DataRefTest, CreateAndDestroy) {
  StyleData::reset_destroyed();
  {
    DataRef<StyleData> data;
    data.Init();
    EXPECT_EQ(StyleData::destroyed_count(), 0);
    EXPECT_TRUE(data.Get()->HasOneRef());
  }
  EXPECT_EQ(StyleData::destroyed_count(), 1);
}

TEST(DataRefTest, CopyOnWrite) {
  DataRef<StyleData> init_data;
  init_data.Init();
  EXPECT_TRUE(init_data->HasOneRef());
  // No copy constructor is called
  DataRef<StyleData> new_data = init_data;
  EXPECT_EQ(new_data->value, init_data->value);
  // Now has two ref count
  EXPECT_FALSE(new_data->HasOneRef());
  init_data.Access()->value = 100;
  // Does not affect the new data
  EXPECT_NE(new_data->value, init_data->value);
  new_data.Access()->value = 2;
  EXPECT_TRUE(new_data->HasOneRef());
  EXPECT_EQ(new_data->value, 2);
}

TEST(DataRefTest, RefCount) {
  StyleData::reset_destroyed();
  {
    DataRef<StyleData> init_data;
    init_data.Init();
    EXPECT_TRUE(init_data->HasOneRef());
    // Create the new ref locally
    {
      DataRef<StyleData> new_data = init_data;
      EXPECT_EQ(new_data->value, init_data->value);
      EXPECT_FALSE(new_data->HasOneRef());
      // The ref count should be 2
      EXPECT_EQ(init_data->SubtleRefCountForDebug(), 2);
    }
    EXPECT_EQ(StyleData::destroyed_count(), 0);
    EXPECT_TRUE(init_data->HasOneRef());
    // Create the new ref locally and write
    {
      DataRef<StyleData> new_data = init_data;
      EXPECT_EQ(new_data->value, init_data->value);
      EXPECT_FALSE(new_data->HasOneRef());
      // The ref count should equal 2
      EXPECT_EQ(init_data->SubtleRefCountForDebug(), 2);
      // Copy the data
      new_data.Access()->value = 2;
      EXPECT_TRUE(new_data->HasOneRef());
      EXPECT_EQ(new_data->value, 2);
    }
    EXPECT_EQ(StyleData::destroyed_count(), 1);
    // Still has one ref count
    EXPECT_TRUE(init_data->HasOneRef());
  }
  // The init data should be destroyed
  EXPECT_EQ(StyleData::destroyed_count(), 2);
}

}  // namespace starlight
}  // namespace lynx
