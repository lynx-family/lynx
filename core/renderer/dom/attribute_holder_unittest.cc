// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/events/gesture.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class AttributeHolderTest : public ::testing::Test {
 public:
  AttributeHolderTest() {}
  ~AttributeHolderTest() override {}

  void SetUp() override {}
};

TEST_F(AttributeHolderTest, ContainsSelector) {
  RadonNode node(nullptr, "my_tag", 0);
  node.SetIdSelector("my_id");
  node.SetStaticClass("class1");
  node.SetStaticClass("class2");
  node.SetDynamicAttribute("attr1", lepus::Value("value1"));
  node.SetDynamicAttribute("attr2", lepus::Value("value2"));
  node.SetDynamicAttribute("src",
                           lepus::Value("https://abc.def/@#==*=$%!.123-456z"));
  node.SetDataSet("data1", lepus::Value("value3"));
  node.SetDataSet("data2", lepus::Value("value4"));
  auto actual = node.ContainsSelector("#my_id");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("#my_id2");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector(".class1");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector(".class1.class2");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector(".class3");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector(".class2.class3");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("my_tag");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("my_tag2");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("$id");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[attr1]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[attr1][attr2]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector(".class1[attr1]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("#my_id.class1[attr1]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[attr1=value1]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[attr3]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[attr1=value2]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[attr1][attr2=value3]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[data-data1]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[attr1][data-data1]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[data-data1=value3][attr2=value2]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[data-data3]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[attr1].class2");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src=https://abc.def/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector(
      "[src=https://abc.def/@#==*=$%!.123-456z][attr1][data-data1]");
  EXPECT_EQ(actual, true);
  actual =
      node.ContainsSelector("[src=https://abc.def/@#==*=$%!.123-456z].class1");
  EXPECT_EQ(actual, true);
  actual =
      node.ContainsSelector("[src=https://abc.def/@#==*=$%!.123-456z]#my_id");
  EXPECT_EQ(actual, true);

  actual = node.ContainsSelector("[src*=https://abc.def/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src*=https://abc.def/@#]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src*=bc.def/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src*=abc.def/@#==*=$%!.123-]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src*=h]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src*=z]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src*=https://abc.def/@#==*=$%!.123-456z1]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[src*=1https://abc.def/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[src*=https://abc.def1/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[src*=https://abc.def/@#==*=$%!.13-456z]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[src*=x]");
  EXPECT_EQ(actual, false);

  actual = node.ContainsSelector("[src^=https://abc.def/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src^=https://abc.def/@#==*=$%!.123-456]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src^=h]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src^=ttps://abc.def/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[src^=z]");
  EXPECT_EQ(actual, false);

  actual = node.ContainsSelector("[src$=https://abc.def/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src$=ttps://abc.def/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src$=ef/@#==*=$%!.123-456z]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src$=z]");
  EXPECT_EQ(actual, true);
  actual = node.ContainsSelector("[src$=https://abc.def/@#==*=$%!.123-456]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[src$=a]");
  EXPECT_EQ(actual, false);
  actual = node.ContainsSelector("[src$=https]");
  EXPECT_EQ(actual, false);
}

// Test case to check the behavior of the Gesture Detector in the RadonNode
// class
TEST_F(AttributeHolderTest, CheckGestureDetector) {
  // Create a RadonNode with tag "my_tag" and node ID 0
  RadonNode node(nullptr, "my_tag", 0);

  // Create a GestureDetector with ID 1, default GestureType, an empty vector of
  // GestureCallback, and an empty map of event names to associated vector of
  // gesture IDs
  GestureDetector gestureDetector(
      1, GestureType::DEFAULT, std::vector<GestureCallback>(),
      std::unordered_map<std::string, std::vector<uint32_t>>());

  // check callback and relation map size
  EXPECT_EQ(gestureDetector.gesture_callbacks().size(), static_cast<size_t>(0));
  EXPECT_EQ(gestureDetector.relation_map().size(), static_cast<size_t>(0));

  // Set the created GestureDetector in the RadonNode with ID 2
  node.SetGestureDetector(2, gestureDetector);

  // Expectation: The size of gesture_detectors vector in the RadonNode should
  // be 1 after setting the GestureDetector
  EXPECT_EQ(node.gesture_detectors().size(), static_cast<size_t>(1));

  // Expectation: Loop through the gesture_detectors vector and ensure the ID of
  // the first GestureDetector is 2
  for (const auto& gesture : node.gesture_detectors()) {
    EXPECT_EQ(gesture.first, static_cast<size_t>(2));
  }

  // Remove the GestureDetector with ID 2 from the RadonNode
  node.RemoveGestureDetector(2);

  // Expectation: After removal, the size of gesture_detectors vector in the
  // RadonNode should be 0
  EXPECT_EQ(node.gesture_detectors().size(), static_cast<size_t>(0));
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
