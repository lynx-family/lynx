// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/utils/diff_algorithm.h"

#include <array>
#include <chrono>
#include <random>
#include <sstream>
#include <unordered_set>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace {
constexpr const char ALPHA_NUM[] =
    "0123456789"
    "!@#$%^&*"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
constexpr const char ALPHA[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
struct randomGenerator {
  std::random_device random_device_{};
  std::mt19937 generator{random_device_()};

  auto Char() { return ALPHA_NUM[generator() % (sizeof(ALPHA_NUM) - 1)]; }

  auto Letter() { return ALPHA[generator() % (sizeof(ALPHA) - 1)]; }

  int Num(int max) { return generator() % max; }

  auto LetterString(int len) {
    auto str = std::string(len, ' ');
    auto chargen = [this]() { return Letter(); };
    std::generate(str.begin(), str.end(), chargen);
    return str;
  }

  auto String(int len) {
    auto str = std::string(len, ' ');
    auto chargen = [this]() { return Char(); };
    std::generate(str.begin(), str.end(), chargen);
    return str;
  }

  auto Bool(int possibility) {
    auto num = Num(100);
    if (num < possibility) {
      return true;
    } else {
      return false;
    }
  }
};

struct Component {
  std::string name_;
  int data_;

  friend bool operator==(const Component& lhs, const Component& rhs) {
    return lhs.name_ == rhs.name_ && lhs.data_ == rhs.data_;
  }

  friend std::ostream& operator<<(std::ostream& ostream,
                                  const Component& component) {
    ostream << "(" << component.name_ << ", " << component.data_ << ") ";
    return ostream;
  }
};

std::pair<std::vector<Component>, std::vector<Component>> genLists(
    size_t max_name_len, size_t pool_size, size_t list_estimate_size) {
  randomGenerator random_generator{};
  constexpr const int DATA_MAX = 10;
  auto component_pool = std::vector<std::string>(pool_size);
  std::generate(component_pool.begin(), component_pool.end(),
                [&random_generator, max_name_len]() {
                  auto len = 0;
                  while (len == 0) {
                    len = random_generator.Num(max_name_len + 1);
                  }
                  return random_generator.LetterString(len);
                });

  auto list_old = std::vector<Component>(list_estimate_size);
  std::generate(
      list_old.begin(), list_old.end(), [&random_generator, &component_pool]() {
        auto index_in_pool = random_generator.Num(component_pool.size());
        return Component{component_pool[index_in_pool],
                         random_generator.Num(DATA_MAX)};
      });
  auto list_new = list_old;
  std::for_each(
      list_new.begin(), list_new.end(),
      [&random_generator, &component_pool](Component& component) {
        if (random_generator.Bool(20)) {
          component.name_ =
              component_pool[random_generator.Num(component_pool.size())];
        }
        if (random_generator.Bool(20)) {
          component.data_ = random_generator.Num(DATA_MAX);
        }
      });

  list_new.resize(random_generator.Num(list_estimate_size));
  list_old.resize(random_generator.Num(list_estimate_size));

  return {list_old, list_new};
}

auto printList(const std::vector<Component>& list, std::stringstream& sstream) {
  for (const auto& component : list) {
    sstream << component;
  }
}

void rebuildDiffResult(lynx::tasm::myers_diff::DiffResult& diff_result) {
  auto& removals = diff_result.removals_;
  auto& insertions = diff_result.insertions_;
  auto& move_from = diff_result.move_from_;
  auto& move_to = diff_result.move_to_;

  std::copy(move_from.begin(), move_from.end(), std::back_inserter(removals));
  std::copy(move_to.begin(), move_to.end(), std::back_inserter(insertions));

  std::sort(removals.begin(), removals.end());
  std::sort(insertions.begin(), insertions.end());
  move_from.clear();
  move_to.clear();
}

std::ostream& operator<<(std::ostream& ostream,
                         lynx::tasm::myers_diff::DiffResult& diff_result) {
  const auto& removals = diff_result.removals_;
  const auto& insertions = diff_result.insertions_;
  const auto& update_from = diff_result.update_from_;
  const auto& update_to = diff_result.update_to_;

  std::copy(removals.begin(), removals.end(),
            std::ostream_iterator<int>{ostream, ", "});
  std::copy(insertions.begin(), insertions.end(),
            std::ostream_iterator<int>{ostream, ", "});
  std::copy(update_from.begin(), update_from.end(),
            std::ostream_iterator<int>{ostream, ", "});
  std::copy(update_to.begin(), update_to.end(),
            std::ostream_iterator<int>{ostream, ", "});

  return ostream;
}

std::pair<bool, std::vector<Component>> rebuildList(
    const std::vector<Component>& list_old,
    const std::vector<Component>& list_new,
    const lynx::tasm::myers_diff::DiffResult& diff_result) {
  const auto& removals = diff_result.removals_;
  const auto& insertions = diff_result.insertions_;
  const auto& update_from = diff_result.update_from_;
  const auto& update_to = diff_result.update_to_;

  auto list = list_old;
  // the size of update from and update to must be the same
  if (update_from.size() != update_to.size()) {
    return {false, {}};
  }

  // first update
  for (size_t i{}; i < update_from.size(); ++i) {
    if (list_old[update_from[i]].name_ == list_new[update_to[i]].name_) {
      list[update_from[i]].data_ = list_new[update_to[i]].data_;
    } else {
      // the two components being updated must have same name
      return {false, {}};
    }
  }

  // then remove
  {
    auto need_remove = [&removals](int i) {
      return std::binary_search(removals.begin(), removals.end(), i);
    };
    size_t i = 0;
    for (auto iter = list.begin(); iter != list.end();) {
      if (need_remove(i)) {
        iter = list.erase(iter);
      } else {
        ++iter;
      }
      i++;
    }
  }

  // finally insert
  for (auto i : insertions) {
    list.insert(list.begin() + i, list_new[i]);
  }

  return {true, list};
}
template <typename Differ>
auto testListsImpl(const std::vector<Component>& list_old,
                   const std::vector<Component>& list_new, Differ differ) {
  std::stringstream sstream{};
  sstream << "Transform From: \n";
  printList(list_old, sstream);
  sstream << "\n To: \n";
  printList(list_new, sstream);
  sstream << "\n";

  auto diff_result = differ(
      list_old.begin(), list_old.end(), list_new.begin(), list_new.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.name_ == rhs.name_; },
      [](const auto& lhs, const auto& rhs) { return lhs == rhs; });

  rebuildDiffResult(diff_result);
  sstream << diff_result;

  const auto rebuild_result = rebuildList(list_old, list_new, diff_result);
  if (!rebuild_result.first) {
    std::cout << sstream.str();
    return false;
  }
  const auto& list = rebuild_result.second;

  auto mismatch_pair =
      std::mismatch(list.begin(), list.end(), list_new.begin());

  if (mismatch_pair.first != list.end() ||
      mismatch_pair.second != list_new.end()) {
    std::cout << sstream.str();
    return false;
  }

  return true;
}

auto testLists(const std::vector<Component>& list_old,
               const std::vector<Component>& list_new) {
  return testListsImpl(list_old, list_new,
                       [](auto&&... args) {
                         return lynx::tasm::myers_diff::MyersDiff(
                             std::forward<decltype(args)>(args)...);
                       }) &&
         testListsImpl(
             list_old, list_new,
             [](auto first1, auto last1, auto first2, auto last2, auto cmp1,
                auto cmp2) {
               auto diffResultBase =
                   lynx::tasm::myers_diff::MyersDiffWithoutUpdate(
                       first1, last1, first2, last2, cmp2);
               auto diffResult = lynx::tasm::myers_diff::DiffResult{};
               diffResult.removals_ = std::move(diffResultBase.removals_);
               diffResult.insertions_ = std::move(diffResultBase.insertions_);
               return diffResult;
             });
}

auto testRandomLists(size_t max_name_len, size_t pool_size,
                     size_t list_estimate_size) {
  auto lists = genLists(max_name_len, pool_size, list_estimate_size);
  return testListsImpl(lists.first, lists.second, [](auto&&... args) {
    return lynx::tasm::myers_diff::MyersDiff(
        std::forward<decltype(args)>(args)...);
  });
}

auto testRandomListsTime(size_t max_name_len, size_t pool_size,
                         size_t list_estimate_size) {
  auto lists = genLists(max_name_len, pool_size, list_estimate_size);
  return testListsImpl(lists.first, lists.second, [&lists](auto&&... args) {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto res = lynx::tasm::myers_diff::MyersDiff(
        std::forward<decltype(args)>(args)...);
    auto stop_time = std::chrono::high_resolution_clock::now();
    auto duration = stop_time - start_time;
    std::cout << "[          ] from list " << lists.first.size() << ", to list "
              << lists.second.size()
              << ", Diff Time: " << duration.count() / 1'000'000 << "ms\n";
    return res;
  });
}

TEST(DiffAlgorithmTest, MyersDiffListPushFrontTest) {
  // case 1
  {
    const auto list_old = std::vector<Component>{{"A", 0}, {"A", 0}, {"A", 0}};
    const auto list_new =
        std::vector<Component>{{"A", 1}, {"A", 0}, {"A", 0}, {"A", 0}};
    auto res = testLists(list_old, list_new);
    ASSERT_TRUE(res);
  }
}

TEST(DiffAlgorithmTest, MyersDiffListPushBackTest) {
  // case 1
  {
    const auto list_old = std::vector<Component>{{"A", 0}, {"A", 0}, {"A", 0}};
    const auto list_new =
        std::vector<Component>{{"A", 0}, {"A", 0}, {"A", 0}, {"A", 1}};
    auto res = testLists(list_old, list_new);
    ASSERT_TRUE(res);
  }
}

TEST(DiffAlgorithmTest, MyersDiffWrongInsertionBadCaseTest) {
  // case 1
  {
    const auto list_old = std::vector<Component>{
        {"h", 0}, {"n", 0}, {"G", 0}, {"S", 0}, {"U", 0}};
    const auto list_new =
        std::vector<Component>{{"@", 0}, {"n", 0}, {"G", 0}, {"s", 0}};
    auto res = testLists(list_old, list_new);
    ASSERT_TRUE(res);
  }
  // case 2
  {
    const auto list_old = std::vector<Component>{
        {"l", 0}, {"d", 0}, {"m", 0}, {"3", 0}, {"2", 0}};
    const auto list_new =
        std::vector<Component>{{"f", 0}, {"d", 0}, {"m", 0}, {"3", 0}};
    auto res = testLists(list_old, list_new);
    ASSERT_TRUE(res);
  }
}

TEST(DiffAlgorithmTest, MyersDiffWrongUpdateBadCaseTest) {
  // case 1
  {
    const auto list_old = std::vector<Component>{{"E", 0}, {"I", 0}};
    const auto list_new = std::vector<Component>{{"E", 0}, {"S", 0}, {"I", 0}};
    auto res = testLists(list_old, list_new);
    ASSERT_TRUE(res);
  }
  // case 2
  {
    const auto list_old = std::vector<Component>{{"C", 0}, {"B", 0}};
    const auto list_new = std::vector<Component>{{"A", 0}, {"B", 0}, {"C", 0}};
    auto res = testLists(list_old, list_new);
    ASSERT_TRUE(res);
  }
}

TEST(DiffAlgorithmTest, MyersDiffMissUpdateBadCaseTest) {
  // case 1
  {
    const auto list_old = std::vector<Component>{{"S", 8}, {"N", 9}};
    const auto list_new = std::vector<Component>{{"N", 8}};
    auto res = testLists(list_old, list_new);
    ASSERT_TRUE(res);
  }
  // case 2
  {
    const auto list_old = std::vector<Component>{{"X", 6}, {"J", 4}};
    const auto list_new = std::vector<Component>{{"J", 4}, {"X", 2}};
    auto res = testLists(list_old, list_new);
    ASSERT_TRUE(res);
  }
}

TEST(DiffAlgorithmTest, MyersDiffDuplicateMoveBadCaseTest) {
  const auto list_old = std::vector<Component>{{"F", 3}, {"W", 8}, {"W", 8}};
  const auto list_new = std::vector<Component>{{"C", 3}, {"W", 8}, {"F", 8}};
  auto res = testLists(list_old, list_new);
  ASSERT_TRUE(res);
}

TEST(DiffAlgorithmTest, MyersDiffRandomTest) {
  auto res = true;
  for (int i = 0; i < 10; ++i) {
    res &= testRandomLists(5, 10, 1000);
  }
  ASSERT_TRUE(res);
}

TEST(DiffAlgorithmTest, MyersDiffRandomTestTime) {
  auto res = true;
  for (int i = 0; i < 10; ++i) {
    res &= testRandomListsTime(1, 10, 1000);
  }
  ASSERT_TRUE(res);
}

}  // namespace
}  // namespace base
}  // namespace lynx
