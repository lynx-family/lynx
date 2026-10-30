// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/lru_cache.h"

#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace {

template <typename Cache, typename = void>
struct HasWeightedPut : std::false_type {};

template <typename Cache>
struct HasWeightedPut<
    Cache, std::void_t<decltype(std::declval<Cache&>().Put(1, 1, size_t{1}))>>
    : std::true_type {};

TEST(LRUCacheTest, CostApiRequiresExplicitOptIn) {
  using CountCache = LRUCache<int, int>;
  using CostCache = CostBoundedLRUCache<int, int>;
  static_assert(
      !std::is_constructible<CountCache, size_t, size_t, size_t>::value,
      "Count-only caches must not accept cost budgets");
  static_assert(std::is_constructible<CostCache, size_t, size_t, size_t>::value,
                "Cost-bounded caches must accept cost budgets");
  static_assert(!HasWeightedPut<CountCache>::value,
                "Count-only caches must not accept weighted insertion");
  static_assert(HasWeightedPut<CostCache>::value,
                "Cost-bounded caches must accept weighted insertion");
}

TEST(LRUCacheTest, CountOnlyPolicyHasNoAccountingFields) {
  static_assert(std::is_empty<internal::LRUCacheCostState<false>>::value,
                "The disabled policy must not store accounting fields");
  static_assert(!std::is_empty<internal::LRUCacheCostState<true>>::value,
                "The enabled policy must store its cost state");
}

TEST(LRUCacheTest, PreservesLegacyApiAndDefaultCapacity) {
  using Cache = LRUCache<int, int>;
  static_assert(
      std::is_same<decltype(std::declval<Cache&>().Get(0)), int*>::value,
      "Get must keep returning a pointer");
  static_assert(
      std::is_same<decltype(std::declval<Cache&>().Put(0, 0)), void>::value,
      "The two-argument Put must keep returning void");
  Cache cache;
  EXPECT_EQ(cache.Get(-1), nullptr);
  for (int i = 0; i <= DEFAULT_CAPACITY; ++i) {
    cache.Put(i, i);
  }
  EXPECT_EQ(cache.Get(0), nullptr);
  ASSERT_NE(cache.Get(DEFAULT_CAPACITY), nullptr);
  EXPECT_EQ(*cache.Get(DEFAULT_CAPACITY), DEFAULT_CAPACITY);
  EXPECT_EQ(cache.size(), static_cast<size_t>(DEFAULT_CAPACITY));
  EXPECT_EQ(cache.total_units(), cache.size());
}

TEST(LRUCacheTest, GetTouchesEntryBeforeCapacityEviction) {
  LRUCache<int, int> cache(2);
  cache.Put(1, 10);
  cache.Put(2, 20);
  ASSERT_NE(cache.Get(1), nullptr);
  EXPECT_EQ(*cache.Get(1), 10);
  cache.Put(3, 30);
  EXPECT_EQ(cache.Get(2), nullptr);
  EXPECT_NE(cache.Get(1), nullptr);
  EXPECT_NE(cache.Get(3), nullptr);
  EXPECT_EQ(cache.size(), 2u);
}

TEST(LRUCacheTest, UpdateTouchesEntryAndPreservesItsAddress) {
  LRUCache<int, int> cache(2);
  cache.Put(1, 10);
  int* value = cache.Get(1);
  cache.Put(2, 20);
  cache.Put(1, 11);
  cache.Put(3, 30);
  EXPECT_EQ(cache.Get(2), nullptr);
  EXPECT_EQ(cache.Get(1), value);
  EXPECT_EQ(*value, 11);
  EXPECT_EQ(cache.total_units(), 2u);
}

TEST(LRUCacheTest, TotalBudgetCanEvictMultipleEntriesBelowCapacity) {
  CostBoundedLRUCache<int, int> cache(8, 6, 6);
  EXPECT_TRUE(cache.Put(1, 10, 2));
  EXPECT_TRUE(cache.Put(2, 20, 2));
  EXPECT_TRUE(cache.Put(3, 30, 2));
  EXPECT_TRUE(cache.Put(4, 40, 5));
  EXPECT_EQ(cache.Get(1), nullptr);
  EXPECT_EQ(cache.Get(2), nullptr);
  EXPECT_EQ(cache.Get(3), nullptr);
  ASSERT_NE(cache.Get(4), nullptr);
  EXPECT_EQ(*cache.Get(4), 40);
  EXPECT_EQ(cache.size(), 1u);
  EXPECT_EQ(cache.total_units(), 5u);
}

TEST(LRUCacheTest, RejectsOversizedEntriesWithoutEviction) {
  CostBoundedLRUCache<int, int> per_entry_limited(4, 10, 2);
  EXPECT_TRUE(per_entry_limited.Put(1, 10, 2));
  EXPECT_FALSE(per_entry_limited.Put(2, 20, 3));
  EXPECT_NE(per_entry_limited.Get(1), nullptr);
  EXPECT_EQ(per_entry_limited.size(), 1u);
  EXPECT_EQ(per_entry_limited.total_units(), 2u);

  CostBoundedLRUCache<int, int> total_limited(4, 2, 10);
  EXPECT_TRUE(total_limited.Put(1, 10, 2));
  EXPECT_FALSE(total_limited.Put(2, 20, 3));
  EXPECT_NE(total_limited.Get(1), nullptr);
  EXPECT_EQ(total_limited.total_units(), 2u);
}

TEST(LRUCacheTest, RejectedUpdatePreservesValueCostAndRecency) {
  CostBoundedLRUCache<int, int> cache(2, 4, 2);
  EXPECT_TRUE(cache.Put(1, 10, 2));
  EXPECT_TRUE(cache.Put(2, 20, 2));
  EXPECT_FALSE(cache.Put(1, 99, 3));
  EXPECT_EQ(cache.total_units(), 4u);
  EXPECT_TRUE(cache.Put(3, 30, 1));
  // A rejected update must not touch entry 1: it remains the LRU victim.
  EXPECT_EQ(cache.Get(1), nullptr);
  ASSERT_NE(cache.Get(2), nullptr);
  EXPECT_EQ(*cache.Get(2), 20);
  EXPECT_EQ(cache.total_units(), 3u);

  EXPECT_FALSE(cache.Put(2, 99, 3));
  EXPECT_EQ(*cache.Get(2), 20);
}

TEST(LRUCacheTest, UpdatesReplaceCostWhenGrowingAndShrinking) {
  CostBoundedLRUCache<int, int> cache(3, 5, 5);
  EXPECT_TRUE(cache.Put(1, 10, 2));
  EXPECT_TRUE(cache.Put(2, 20, 2));
  EXPECT_TRUE(cache.Put(1, 11, 4));
  EXPECT_EQ(cache.Get(2), nullptr);
  EXPECT_EQ(cache.total_units(), 4u);
  EXPECT_TRUE(cache.Put(1, 12, 1));
  EXPECT_EQ(cache.total_units(), 1u);
  EXPECT_TRUE(cache.Put(2, 20, 4));
  EXPECT_EQ(cache.total_units(), 5u);
  EXPECT_EQ(cache.size(), 2u);
}

TEST(LRUCacheTest, ZeroCapacityRejectsBothPutOverloads) {
  LRUCache<int, int> cache(0);
  cache.Put(1, 10);
  CostBoundedLRUCache<int, int> cost_cache(0, 0, 0);
  EXPECT_FALSE(cost_cache.Put(2, 20, 0));
  EXPECT_EQ(cost_cache.size(), 0u);
  EXPECT_EQ(cache.Get(1), nullptr);
  EXPECT_EQ(cache.size(), 0u);
  EXPECT_EQ(cache.total_units(), 0u);
}

TEST(LRUCacheTest, ZeroBudgetsAllowOnlyZeroCostEntries) {
  CostBoundedLRUCache<int, int> cache(2, 0, 0);
  cache.Put(1, 10);
  EXPECT_EQ(cache.Get(1), nullptr);
  EXPECT_TRUE(cache.Put(1, 10, 0));
  EXPECT_TRUE(cache.Put(2, 20, 0));
  EXPECT_TRUE(cache.Put(3, 30, 0));
  EXPECT_EQ(cache.Get(1), nullptr);
  EXPECT_EQ(cache.size(), 2u);
  EXPECT_EQ(cache.total_units(), 0u);
}

TEST(LRUCacheTest, ClearResetsAccountingAndAllowsReuse) {
  CostBoundedLRUCache<int, int> cache(2, 3, 3);
  EXPECT_TRUE(cache.Put(1, 10, 3));
  cache.Clear();
  EXPECT_EQ(cache.Get(1), nullptr);
  EXPECT_EQ(cache.size(), 0u);
  EXPECT_EQ(cache.total_units(), 0u);
  EXPECT_TRUE(cache.Put(2, 20, 3));
  EXPECT_EQ(cache.total_units(), 3u);
}

struct CacheKey {
  int id;
};

struct CacheKeyHash {
  // Deliberately collide so equality must distinguish different keys.
  size_t operator()(const CacheKey&) const { return 0; }
};

struct CacheKeyEqual {
  bool operator()(const CacheKey& lhs, const CacheKey& rhs) const {
    return lhs.id == rhs.id;
  }
};

TEST(LRUCacheTest, SupportsCustomHashAndEquality) {
  CostBoundedLRUCache<CacheKey, int, CacheKeyHash, CacheKeyEqual> cache(2, 3,
                                                                        3);
  EXPECT_TRUE(cache.Put({1}, 10, 1));
  EXPECT_TRUE(cache.Put({2}, 20, 1));
  EXPECT_TRUE(cache.Put({1}, 11, 2));
  ASSERT_NE(cache.Get({1}), nullptr);
  EXPECT_EQ(*cache.Get({1}), 11);
  EXPECT_TRUE(cache.Put({3}, 30, 1));
  EXPECT_EQ(cache.Get({2}), nullptr);
  EXPECT_EQ(cache.size(), 2u);
  EXPECT_EQ(cache.total_units(), 3u);
}

TEST(LRUCacheTest, SupportsMoveOnlyValues) {
  LRUCache<int, std::unique_ptr<int>> count_cache(1);
  count_cache.Put(1, std::make_unique<int>(10));
  count_cache.Put(2, std::make_unique<int>(20));
  EXPECT_EQ(count_cache.Get(1), nullptr);
  ASSERT_NE(count_cache.Get(2), nullptr);
  EXPECT_EQ(**count_cache.Get(2), 20);
  CostBoundedLRUCache<int, std::unique_ptr<int>> cache(2, 3, 3);
  cache.Put(1, std::make_unique<int>(10));
  EXPECT_TRUE(cache.Put(2, std::make_unique<int>(20), 2));
  EXPECT_TRUE(cache.Put(1, std::make_unique<int>(11), 2));
  EXPECT_EQ(cache.Get(2), nullptr);
  ASSERT_NE(cache.Get(1), nullptr);
  EXPECT_EQ(**cache.Get(1), 11);
}

TEST(LRUCacheTest, BudgetAccountingDoesNotOverflow) {
  const size_t max_units = std::numeric_limits<size_t>::max();
  CostBoundedLRUCache<int, int> cache(3, max_units, max_units);
  EXPECT_TRUE(cache.Put(1, 10, max_units - 1));
  EXPECT_TRUE(cache.Put(2, 20, 1));
  EXPECT_EQ(cache.total_units(), max_units);
  EXPECT_TRUE(cache.Put(3, 30, 1));
  EXPECT_EQ(cache.Get(1), nullptr);
  EXPECT_EQ(cache.total_units(), 2u);
  EXPECT_TRUE(cache.Put(2, 21, max_units));
  EXPECT_EQ(cache.Get(3), nullptr);
  EXPECT_EQ(cache.size(), 1u);
  EXPECT_EQ(cache.total_units(), max_units);
}

}  // namespace
}  // namespace base
}  // namespace lynx
