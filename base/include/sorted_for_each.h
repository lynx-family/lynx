// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_SORTED_FOR_EACH_H_
#define BASE_INCLUDE_SORTED_FOR_EACH_H_

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

namespace lynx::base {

// `sorted_for_each` work just like `std::for_each`, but in a sorted order.
//
// It applies the given function object f to the result of dereferencing every
// iterator in the range [first, last), in order.
//
// InputIt:
// The iterators in the range [first, last) should keep being valid during the
// whole traversing process, since they are stored and sorted.
// The InputIt must meet the requirements of LegacyInputIterator.
// see: https://en.cppreference.com/w/cpp/named_req/InputIterator
//
// Sort:
// The iterators are sorted by `std::stable_sort`, it sorts the elements with
// the dereferenced iterators in the range [first, last) in non-descending
// order. The order of equivalent elements is guaranteed to be preserved. If the
// compare function is not passed, will use `std::less<>` as default.
// see: https://en.cppreference.com/w/cpp/algorithm/stable_sort
//
// Function object f:
// If the iterator type is mutable, f may modify the elements of the range
// through the dereferenced iterator.
// If f returns a result, the result is ignored.
// The signature of the function should be equivalent to the following:
//   `void fun(const InputIt::value_type &a);`
// The signature does not need to have `const &`.
// The type Type must be such that an object of type `InputIt` can be
// dereferenced and then implicitly converted to `Type`.
template <class InputIt, class UnaryFunc, class Compare>
constexpr UnaryFunc sorted_for_each(InputIt first, InputIt last, UnaryFunc f,
                                    Compare comp) {
  // iterators from [first, last) is stored and sorted in a `std::vector`
  // so they should keep being valid
  std::vector<InputIt> iterators;
  for (; first != last; first++) {
    iterators.push_back(first);
  }
  std::stable_sort(iterators.begin(), iterators.end(),
                   [&comp](InputIt a, InputIt b) { return comp(*a, *b); });

  for (auto& it : iterators) {
    // f is called with the dereferenced iterators
    f(*it);
  }
  // return the f as `std::for_each` does
  return std::move(f);
}

// Overrides of `sorted_for_each`, Compare function defaults to `std::less<>`
template <class InputIt, class UnaryFunc>
constexpr UnaryFunc sorted_for_each(InputIt first, InputIt last, UnaryFunc f) {
  return sorted_for_each(first, last, std::forward<UnaryFunc>(f),
                         std::less<>());
}

template <class Container, class UnaryFunc, class Compare>
constexpr UnaryFunc SortedForEach(const Container& container, UnaryFunc f,
                                  Compare comp) {
  return sorted_for_each(container.begin(), container.end(),
                         std::forward<UnaryFunc>(f), comp);
}

template <class Container, class UnaryFunc>
constexpr UnaryFunc SortedForEach(const Container& container, UnaryFunc f) {
  return sorted_for_each(container.begin(), container.end(),
                         std::forward<UnaryFunc>(f), std::less<>());
}

}  // namespace lynx::base

#endif  // BASE_INCLUDE_SORTED_FOR_EACH_H_
