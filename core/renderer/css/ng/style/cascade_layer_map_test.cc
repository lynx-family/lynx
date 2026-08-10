// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/style/cascade_layer_map.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

TEST(CascadeLayerMapTest, EmptyByDefault) {
  CascadeLayerMap map;
  EXPECT_TRUE(map.IsEmpty());
  EXPECT_EQ(nullptr, map.GetRootLayer());
  EXPECT_EQ(CascadeLayer::kImplicitOuterLayerOrder, map.GetLayerOrder(nullptr));
}

TEST(CascadeLayerMapTest, MergeNullIsNoOp) {
  CascadeLayerMap map;
  map.MergeLayerTree(nullptr);
  EXPECT_TRUE(map.IsEmpty());
}

TEST(CascadeLayerMapTest, SingleFragmentSingleLayer) {
  // Fragment: @layer foo { ... }
  CascadeLayer root;
  auto* foo = root.GetOrAddSubLayer({"foo"});

  CascadeLayerMap map;
  map.MergeLayerTree(&root);
  map.ComputeLayerOrder();

  EXPECT_FALSE(map.IsEmpty());
  ASSERT_NE(nullptr, map.GetRootLayer());
  EXPECT_EQ(1U, map.GetRootLayer()->GetDirectSubLayers().size());
  uint16_t foo_order = map.GetLayerOrder(foo);
  uint16_t root_order = map.GetLayerOrder(&root);
  EXPECT_LT(foo_order, root_order);
}

TEST(CascadeLayerMapTest, SingleFragmentMultipleLayers_PostOrderDFS) {
  // Fragment: @layer a, b, c;
  // Post-order DFS: a(0), b(1), c(2), root(implicit)
  CascadeLayer root;
  auto* a = root.GetOrAddSubLayer({"a"});
  auto* b = root.GetOrAddSubLayer({"b"});
  auto* c = root.GetOrAddSubLayer({"c"});

  CascadeLayerMap map;
  map.MergeLayerTree(&root);
  map.ComputeLayerOrder();

  EXPECT_LT(map.GetLayerOrder(a), map.GetLayerOrder(b));
  EXPECT_LT(map.GetLayerOrder(b), map.GetLayerOrder(c));
  EXPECT_LT(map.GetLayerOrder(c), map.GetLayerOrder(&root));
  EXPECT_EQ(CascadeLayer::kImplicitOuterLayerOrder, map.GetLayerOrder(&root));
}

TEST(CascadeLayerMapTest, NestedLayers_PostOrderDFS) {
  // @layer a { @layer x, y; }
  // @layer b;
  // Post-order: a.x(0), a.y(1), a(2), b(3), root(implicit)
  CascadeLayer root;
  auto* a = root.GetOrAddSubLayer({"a"});
  auto* ax = a->GetOrAddSubLayer({"x"});
  auto* ay = a->GetOrAddSubLayer({"y"});
  auto* b = root.GetOrAddSubLayer({"b"});

  CascadeLayerMap map;
  map.MergeLayerTree(&root);
  map.ComputeLayerOrder();

  EXPECT_LT(map.GetLayerOrder(ax), map.GetLayerOrder(ay));
  EXPECT_LT(map.GetLayerOrder(ay), map.GetLayerOrder(a));
  EXPECT_LT(map.GetLayerOrder(a), map.GetLayerOrder(b));
  EXPECT_EQ(CascadeLayer::kImplicitOuterLayerOrder, map.GetLayerOrder(&root));
}

TEST(CascadeLayerMapTest, GetLayerPathReturnsCanonicalNamesOuterToInner) {
  CascadeLayer root1;
  auto* components1 = root1.GetOrAddSubLayer({"framework", "components"});

  CascadeLayer root2;
  auto* components2 = root2.GetOrAddSubLayer({"framework", "components"});

  CascadeLayerMap map;
  map.MergeLayerTree(&root1);
  map.MergeLayerTree(&root2);
  map.ComputeLayerOrder();

  const std::vector<std::string> expected{"framework", "components"};
  EXPECT_EQ(expected, map.GetLayerPath(components1));
  EXPECT_EQ(expected, map.GetLayerPath(components2));
  EXPECT_TRUE(map.GetLayerPath(&root1).empty());
  EXPECT_TRUE(map.GetLayerPath(nullptr).empty());

  CascadeLayer unrelated("unrelated");
  EXPECT_TRUE(map.GetLayerPath(&unrelated).empty());
}

TEST(CascadeLayerMapTest, GetLayerPathPreservesAnonymousLayer) {
  CascadeLayer root;
  auto* anonymous = root.GetOrAddSubLayer({"framework", ""});

  CascadeLayerMap map;
  map.MergeLayerTree(&root);
  map.ComputeLayerOrder();

  const std::vector<std::string> expected{"framework", ""};
  EXPECT_EQ(expected, map.GetLayerPath(anonymous));
}

TEST(CascadeLayerMapTest, MultipleFragments_SameNameMerge) {
  // Fragment 1: @layer foo { @layer bar; }
  CascadeLayer root1;
  auto* foo1 = root1.GetOrAddSubLayer({"foo"});
  auto* bar1 = foo1->GetOrAddSubLayer({"bar"});

  // Fragment 2: @layer foo { @layer baz; }
  CascadeLayer root2;
  auto* foo2 = root2.GetOrAddSubLayer({"foo"});
  auto* baz2 = foo2->GetOrAddSubLayer({"baz"});

  CascadeLayerMap map;
  map.MergeLayerTree(&root1);
  map.MergeLayerTree(&root2);
  map.ComputeLayerOrder();

  // foo merged: has bar and baz as children
  // Post-order: bar(0), baz(1), foo(2), root(implicit)
  EXPECT_LT(map.GetLayerOrder(bar1), map.GetLayerOrder(baz2));
  EXPECT_LT(map.GetLayerOrder(baz2), map.GetLayerOrder(foo1));
  // foo1 and foo2 should map to the same canonical node (same order)
  EXPECT_EQ(map.GetLayerOrder(foo1), map.GetLayerOrder(foo2));
  EXPECT_EQ(CascadeLayer::kImplicitOuterLayerOrder, map.GetLayerOrder(&root1));
  EXPECT_EQ(CascadeLayer::kImplicitOuterLayerOrder, map.GetLayerOrder(&root2));
}

TEST(CascadeLayerMapTest, MultipleFragments_DifferentNames) {
  // Fragment 1: @layer a;
  CascadeLayer root1;
  auto* a = root1.GetOrAddSubLayer({"a"});

  // Fragment 2: @layer b;
  CascadeLayer root2;
  auto* b = root2.GetOrAddSubLayer({"b"});

  CascadeLayerMap map;
  map.MergeLayerTree(&root1);
  map.MergeLayerTree(&root2);
  map.ComputeLayerOrder();

  // Post-order: a(0), b(1), root(implicit)
  EXPECT_LT(map.GetLayerOrder(a), map.GetLayerOrder(b));
}

TEST(CascadeLayerMapTest, AnonymousLayers_NeverMerge) {
  // Two anonymous layers in the same fragment should be distinct nodes.
  CascadeLayer root;
  auto* anon1 = root.GetOrAddSubLayer({""});
  auto* anon2 = root.GetOrAddSubLayer({""});

  CascadeLayerMap map;
  map.MergeLayerTree(&root);
  map.ComputeLayerOrder();

  EXPECT_NE(map.GetLayerOrder(anon1), map.GetLayerOrder(anon2));
  EXPECT_LT(map.GetLayerOrder(anon1), map.GetLayerOrder(anon2));
}

TEST(CascadeLayerMapTest, AnonymousLayers_AcrossFragments_NeverMerge) {
  // Anonymous layers across fragments should never merge.
  CascadeLayer root1;
  auto* anon1 = root1.GetOrAddSubLayer({""});

  CascadeLayer root2;
  auto* anon2 = root2.GetOrAddSubLayer({""});

  CascadeLayerMap map;
  map.MergeLayerTree(&root1);
  map.MergeLayerTree(&root2);
  map.ComputeLayerOrder();

  EXPECT_NE(map.GetLayerOrder(anon1), map.GetLayerOrder(anon2));
}

TEST(CascadeLayerMapTest, GetLayerOrder_UnknownLayer_ReturnsImplicit) {
  CascadeLayer root;
  root.GetOrAddSubLayer({"foo"});

  CascadeLayerMap map;
  map.MergeLayerTree(&root);
  map.ComputeLayerOrder();

  // A layer pointer not registered in the map should return implicit order.
  CascadeLayer unrelated("unrelated");
  EXPECT_EQ(CascadeLayer::kImplicitOuterLayerOrder,
            map.GetLayerOrder(&unrelated));
}

TEST(CascadeLayerMapTest, DottedLayerName) {
  // @layer framework.base;
  // @layer framework.utilities;
  CascadeLayer root;
  auto* base = root.GetOrAddSubLayer({"framework", "base"});
  auto* utils = root.GetOrAddSubLayer({"framework", "utilities"});

  CascadeLayerMap map;
  map.MergeLayerTree(&root);
  map.ComputeLayerOrder();

  // Post-order: base(0), utilities(1), framework(2), root(implicit)
  EXPECT_LT(map.GetLayerOrder(base), map.GetLayerOrder(utils));
}

}  // namespace css
}  // namespace lynx
