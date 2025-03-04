// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/vector.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <numeric>
#include <stack>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"

#include "base/include/vector_helper.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

/**
 * @brief Insertion sort algorithm.
 *
 * @details This is a STABLE in-place O(n^2) algorithm. It is efficient
 * for ranges smaller than 10 elements.
 *
 * All iterators of Vector are random-access iterators which are also
 * valid bidirectional iterators.
 * @param first a bidirectional iterator.
 * @param last a bidirectional iterator.
 * @param compare a comparison functor.
 */
template <
    class BidirectionalIterator, class Compare,
    class T = typename std::iterator_traits<BidirectionalIterator>::value_type>
inline void InsertionSort(BidirectionalIterator first,
                          BidirectionalIterator last, Compare compare) {
  if (first == last) {
    return;
  }

  auto it = first;
  for (++it; it != last; ++it) {
    auto key = std::move(*it);
    auto insertPos = it;
    for (auto movePos = it; movePos != first && compare(key, *(--movePos));
         --insertPos) {
      *insertPos = std::move(*movePos);
    }
    *insertPos = std::move(key);
  }
}

// Polyfills from SarNative

struct Matrix3 {
  static const Matrix3 zero;
  static const Matrix3 identity;

  union {
    float elements[9]{1, 0, 0, 0, 1, 0, 0, 0, 1};
    struct {
      float col0[3];
      float col1[3];
      float col2[3];
    };

    struct {
      float m0;
      float m1;
      float m2;
      float m3;
      float m4;
      float m5;
      float m6;
      float m7;
      float m8;
    };
  };

  Matrix3() noexcept = default;

  Matrix3(float e00, float e01, float e02, float e10, float e11, float e12,
          float e20, float e21, float e22) {
    Set(e00, e01, e02, e10, e11, e12, e20, e21, e22);
  }

  explicit Matrix3(const float arr[9]) {
    std::memcpy(elements, arr, 9 * sizeof(float));
  }

  bool operator==(const Matrix3& value) const {
    return std::memcmp(elements, value.elements, 9 * sizeof(float)) == 0;
  }

  bool operator!=(const Matrix3& value) const { return !(*this == value); }

  float& operator[](const size_t index) { return elements[index]; }

  float operator[](const size_t index) const { return elements[index]; }

  Matrix3& Set(const float e00, const float e01, const float e02,
               const float e10, const float e11, const float e12,
               const float e20, const float e21, const float e22) {
    elements[0] = e00;
    elements[3] = e01;
    elements[6] = e02;
    elements[1] = e10;
    elements[4] = e11;
    elements[7] = e12;
    elements[2] = e20;
    elements[5] = e21;
    elements[8] = e22;
    return *this;
  }
};

const Matrix3 Matrix3::zero = {0, 0, 0, 0, 0, 0, 0, 0, 0};
const Matrix3 Matrix3::identity = {1, 0, 0, 0, 1, 0, 0, 0, 1};

template <class T>
void _checkVector([[maybe_unused]] const Vector<T>& array,
                  [[maybe_unused]] int line) {
  // Currently nothing to do.
}

#define CheckVector(array) _checkVector(array, __LINE__)

TEST(Vector, ByteArray) {
  struct Range {
    uint32_t start;
    uint32_t end;
  };
  Range range{10000, 20000};

  // Additional tests for ByteArray
  std::vector<uint8_t> vec;
  vec.push_back(0);
  vec.push_back(1);
  vec.push_back(0);

  std::vector<uint8_t> vec_final;
  {
    std::vector<uint8_t> start((uint8_t*)(&range.start),
                               (uint8_t*)(&range.start) + sizeof(uint32_t));
    std::vector<uint8_t> end((uint8_t*)(&range.end),
                             (uint8_t*)(&range.end) + sizeof(uint32_t));

    vec.insert(vec.end(), start.begin(), start.end());
    vec.insert(vec.end(), end.begin(), end.end());

    std::string s(vec.begin(), vec.end());
    auto s2 = std::make_unique<std::string>(s);
    auto u_char_arr = reinterpret_cast<unsigned const char*>(s2->c_str());
    vec_final = std::vector<uint8_t>(u_char_arr, u_char_arr + s.size());
  }

  // ByteArray version
  ByteArray array;
  array.push_back(0);
  array.push_back(1);
  array.push_back(0);

  ByteArray array_final;
  {
    ByteArray start((uint8_t*)(&range.start),
                    (uint8_t*)(&range.start) + sizeof(uint32_t));
    ByteArray end((uint8_t*)(&range.end),
                  (uint8_t*)(&range.end) + sizeof(uint32_t));

    array.append(start);
    array.append(end);

    std::string s(array.begin(), array.end());
    auto s2 = std::make_unique<std::string>(s);
    auto u_char_arr = reinterpret_cast<unsigned const char*>(s2->c_str());
    array_final = ByteArray(u_char_arr, u_char_arr + s.size());
  }

  // Check
  EXPECT_EQ(vec_final.size(), 11);
  EXPECT_EQ(vec_final.size(), array_final.size());
  for (size_t i = 0; i < vec_final.size(); i++) {
    EXPECT_EQ(vec_final[i], array_final[i]);
  }

  std::vector<uint8_t> vec_copy(array_final.begin(), array_final.end());
  EXPECT_EQ(vec_copy.size(), array_final.size());
  for (size_t i = 0; i < vec_copy.size(); i++) {
    EXPECT_EQ(vec_copy[i], array_final[i]);
  }
}

template <int N>
struct TinyTrivialStruct {
  char c[N];
};

TEST(Vector, TrivialTinyInt){
#define TRIVIAL_TINY_INT(T)       \
  {                               \
    Vector<T> array;              \
    T v = 100;                    \
    for (T i = 1; i < 100; i++) { \
      array.push_back(i);         \
    }                             \
    array.push_back(v);           \
    int sum = 0;                  \
    for (auto i : array) {        \
      sum += (int)i;              \
    }                             \
    EXPECT_EQ(sum, 5050);         \
  }

    TRIVIAL_TINY_INT(uint8_t) TRIVIAL_TINY_INT(uint16_t)
        TRIVIAL_TINY_INT(uint32_t) TRIVIAL_TINY_INT(uint64_t)}

TEST(Vector, TrivialTinyStruct){
#define TRIVIAL_TINY_STRUCT(N)                 \
  {                                            \
    Vector<TinyTrivialStruct<N>> array;        \
    TinyTrivialStruct<N> s;                    \
    for (int i = 0; i < N; i++) {              \
      s.c[i] = -i;                             \
    }                                          \
    array.push_back(s);                        \
    array.push_back(std::move(s));             \
    array.emplace_back(s);                     \
    for (int i = 0; i < N; i++) {              \
      for (int j = 0; j < array.size(); j++) { \
        EXPECT_EQ(array[j].c[i], -i);          \
      }                                        \
    }                                          \
  }

    TRIVIAL_TINY_STRUCT(1) TRIVIAL_TINY_STRUCT(2) TRIVIAL_TINY_STRUCT(3)
        TRIVIAL_TINY_STRUCT(4) TRIVIAL_TINY_STRUCT(5) TRIVIAL_TINY_STRUCT(6)
            TRIVIAL_TINY_STRUCT(7) TRIVIAL_TINY_STRUCT(8)}

TEST(Vector, FromStream) {
  std::string data = "Hello World!";
  std::stringstream stream(data);

  std::vector<uint8_t> vector((std::istreambuf_iterator<char>(stream)),
                              std::istreambuf_iterator<char>());

  ByteArray empty = ByteArrayFromStream(stream);
  EXPECT_TRUE(empty.empty());

  {
    stream.seekg(0);
    ByteArray full = ByteArrayFromStream(stream);

    EXPECT_EQ(vector.size(), full.size());
    for (size_t i = 0; i < vector.size(); i++) {
      EXPECT_EQ(vector[i], full[i]);
    }
  }

  {
    stream.seekg(1);
    ByteArray partial = ByteArrayFromStream(stream);

    EXPECT_EQ(vector.size() - 1, partial.size());
    for (size_t i = 0; i < partial.size(); i++) {
      EXPECT_EQ(vector[i + 1], partial[i]);
    }
  }
}

TEST(Vector, FromString) {
  std::string data = "Hello World!";

  std::vector<char> vector(data.begin(), data.end());
  ByteArray array = ByteArrayFromString(data);

  EXPECT_EQ(vector.size(), array.size());
  for (size_t i = 0; i < vector.size(); i++) {
    EXPECT_EQ(vector[i], array[i]);
  }
}

TEST(Vector, Pointer) {
  // Vector<T is pointer> shares the same push_back method.
  int a = 100;
  int b = 200;
  std::string sa = "300";
  std::string sb = "400";
  std::string sc = "500";

  Vector<const int*> ints;
  ints.push_back(&a);
  CheckVector(ints);
  ints.push_back(&b);
  CheckVector(ints);

  Vector<std::string*> strings;
  strings.push_back(&sa);
  CheckVector(strings);
  strings.push_back(&sb);
  CheckVector(strings);
  EXPECT_EQ(strings.emplace_back(&sc), &sc);
  CheckVector(strings);

  EXPECT_EQ(*ints[0], 100);
  EXPECT_EQ(*ints[1], 200);
  EXPECT_EQ(*strings[0], "300");
  EXPECT_EQ(*strings[1], "400");
  EXPECT_EQ(*strings[2], "500");

  Vector<char> chars;
  chars.push_back(0);
  CheckVector(chars);  // Make sure push_back(const void* e) not called
  chars.push_back(1);
  CheckVector(chars);
  EXPECT_EQ(chars[1], 1);

  Vector<const char*> c_strings;
  c_strings.push_back("abcd");
  c_strings.push_back("1234");
  EXPECT_EQ(std::strcmp(c_strings[0], "abcd"), 0);
  EXPECT_EQ(std::strcmp(c_strings[1], "1234"), 0);
}

TEST(Vector, ConstructFill) {
  class NontrivialInt {
   public:
    NontrivialInt(int i = -1) {
      value_ = std::make_shared<std::string>(std::to_string(i));
    }

    operator int() const { return std::stoi(*value_); }
    bool operator==(const NontrivialInt& other) {
      return (int)(*this) == (int)other;
    }
    NontrivialInt& operator+=(int value) {
      value_ = std::make_shared<std::string>(
          std::to_string(std::stoi(*value_) + value));
      return *this;
    }

   private:
    std::shared_ptr<std::string> value_;
  };

  auto to_s = [](const Vector<NontrivialInt>& array) -> std::string {
    std::string result;
    for (auto& i : array) {
      result += std::to_string((int)i);
    }
    return result;
  };

  {
    NontrivialInt i5(5);
    InlineVector<NontrivialInt, 3> vec(4, i5);
    EXPECT_EQ(to_s(vec), "5555");
    EXPECT_FALSE(vec.is_static_buffer());

    InlineVector<NontrivialInt, 3> vec2(3, i5);
    EXPECT_EQ(to_s(vec2), "555");
    EXPECT_TRUE(vec2.is_static_buffer());

    InlineVector<NontrivialInt, 3> vec3(3);
    EXPECT_EQ(to_s(vec3), "-1-1-1");
    EXPECT_TRUE(vec3.is_static_buffer());

    InlineVector<NontrivialInt, 3> vec4(4);
    EXPECT_EQ(to_s(vec4), "-1-1-1-1");
    EXPECT_FALSE(vec4.is_static_buffer());
  }

  {
    InlineVector<bool, 3> vec(3);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_TRUE(vec.is_static_buffer());
    for (auto b : vec) {
      EXPECT_FALSE(b);
    }
  }

  {
    InlineVector<bool, 3> vec(5);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_FALSE(vec.is_static_buffer());
    for (auto b : vec) {
      EXPECT_FALSE(b);
    }
  }

  {
    InlineVector<bool, 3> vec(5, true);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_FALSE(vec.is_static_buffer());
    for (auto b : vec) {
      EXPECT_TRUE(b);
    }
  }

  {
    InlineVector<float, 3> vec(4, 3.14f);
    EXPECT_EQ(vec.size(), 4);
    for (auto f : vec) {
      EXPECT_EQ(f, 3.14f);
    }
    EXPECT_FALSE(vec.is_static_buffer());

    InlineVector<float, 3> vec2(3, 3.14f);
    EXPECT_EQ(vec2.size(), 3);
    for (auto f : vec2) {
      EXPECT_EQ(f, 3.14f);
    }
    EXPECT_TRUE(vec2.is_static_buffer());

    InlineVector<float, 3> vec3(3);
    EXPECT_EQ(vec3.size(), 3);
    for (auto f : vec3) {
      EXPECT_EQ(f, 0.0f);
    }
    EXPECT_TRUE(vec3.is_static_buffer());

    InlineVector<float, 3> vec4(4);
    EXPECT_EQ(vec4.size(), 4);
    for (auto f : vec4) {
      EXPECT_EQ(f, 0.0f);
    }
    EXPECT_FALSE(vec4.is_static_buffer());
  }

  {
    InlineVector<const void*, 3> vec(4, (const void*)(&Matrix3::zero));
    for (auto p : vec) {
      EXPECT_EQ(p, (const void*)(&Matrix3::zero));
    }
    EXPECT_EQ(vec.size(), 4);
    EXPECT_FALSE(vec.is_static_buffer());

    InlineVector<const void*, 3> vec2(3);
    for (auto p : vec2) {
      EXPECT_EQ(p, nullptr);
    }
    EXPECT_EQ(vec2.size(), 3);
    EXPECT_TRUE(vec2.is_static_buffer());
  }

  {
    InlineVector<NontrivialInt, 3> vec;
    for (int i = 0; i < 10; i++) {
      vec.emplace_back(i);
    }

    InlineVector<NontrivialInt, 3> vec2(vec.begin() + 2, vec.begin() + 5);
    EXPECT_EQ(to_s(vec2), "234");
    EXPECT_TRUE(vec2.is_static_buffer());
  }

  {
    InlineVector<int, 3> vec;
    for (int i = 0; i < 10; i++) {
      vec.emplace_back(i);
    }

    InlineVector<int, 3> vec2(vec.begin() + 2, vec.begin() + 5);
    EXPECT_EQ(vec2.size(), 3);
    EXPECT_EQ(vec2[0], 2);
    EXPECT_EQ(vec2[1], 3);
    EXPECT_EQ(vec2[2], 4);
    EXPECT_TRUE(vec2.is_static_buffer());
  }
}

TEST(Vector, InlineSwap) {
  auto to_s = [](const Vector<int>& array) -> std::string {
    std::string result;
    for (int i : array) {
      result += std::to_string(i);
    }
    return result;
  };

  {
    InlineVector<int, 5> array{0, 1, 2, 3, 4};
    Vector<int> array2{5, 6, 7, 8, 9};
    array2.swap(array);
    EXPECT_EQ(to_s(array), "56789");
    EXPECT_EQ(to_s(array2), "01234");
  }

  {
    InlineVector<int, 5> array{0, 1, 2, 3, 4};
    Vector<int> array2{5, 6, 7, 8, 9};
    array.swap(array2);
    EXPECT_EQ(to_s(array), "56789");
    EXPECT_EQ(to_s(array2), "01234");
  }

  {
    InlineVector<int, 5> array{0, 1, 2, 3, 4};
    Vector<int> array2{5, 6, 7, 8, 9};
    std::swap(array, array2);
    EXPECT_EQ(to_s(array), "56789");
    EXPECT_EQ(to_s(array2), "01234");
  }

  // Inline buffer overflow
  {
    InlineVector<int, 5> array{0, 1, 2, 3, 4, 5};
    Vector<int> array2{5, 6, 7, 8, 9};
    array2.swap(array);
    EXPECT_EQ(to_s(array), "56789");
    EXPECT_EQ(to_s(array2), "012345");
  }

  {
    InlineVector<int, 5> array{0, 1, 2, 3, 4, 5};
    Vector<int> array2{5, 6, 7, 8, 9};
    array.swap(array2);
    EXPECT_EQ(to_s(array), "56789");
    EXPECT_EQ(to_s(array2), "012345");
  }

  {
    InlineVector<int, 5> array{0, 1, 2, 3, 4, 5};
    Vector<int> array2{5, 6, 7, 8, 9};
    std::swap(array, array2);
    EXPECT_EQ(to_s(array), "56789");
    EXPECT_EQ(to_s(array2), "012345");
  }
}

TEST(Vector, Inline) {
  auto to_s = [](const Vector<int>& array) -> std::string {
    std::string result;
    for (int i : array) {
      result += std::to_string(i);
    }
    return result;
  };

  InlineVector<int, 100> array;
  const auto data0 = array.data();
  EXPECT_TRUE((uint8_t*)data0 - (uint8_t*)&array == sizeof(Vector<int>));
  for (size_t i = 1; i <= 80; i++) {
    array.push_back(i);
  }
  EXPECT_EQ(data0, array.data());
  CheckVector(array);

  array.clear();
  CheckVector(array);
  EXPECT_EQ(data0, array.data());
  for (size_t i = 1; i <= 80; i++) {
    array.push_back(i);
  }
  EXPECT_EQ(data0, array.data());
  CheckVector(array);

  EXPECT_EQ(array.size(), 80);
  EXPECT_FALSE(array.reserve(90));
  CheckVector(array);
  EXPECT_EQ(data0, array.data());  // Reserve but no reallocation.
  for (size_t i = 81; i <= 90; i++) {
    array.push_back(i);
  }
  EXPECT_EQ(array.size(), 90);
  CheckVector(array);
  EXPECT_EQ(data0, array.data());  // Still no reallocation.

  EXPECT_FALSE(array.resize<false>(100));
  CheckVector(array);              // Resize but still no reallocation.
  EXPECT_EQ(data0, array.data());  // Resize but no reallocation.
  for (size_t i = 90; i < 100; i++) {
    array[i] = i + 1;
  }
  EXPECT_EQ(array.size(), 100);
  CheckVector(array);

  array.push_back(101);
  CheckVector(array);  // Reallocation
  EXPECT_TRUE(data0 != array.data());
  int sum = 0;
  for (auto i : array) {
    sum += i;
  }
  EXPECT_EQ(sum, 5050 + 101);

  array.clear_and_shrink();
  EXPECT_TRUE(array.empty());
  EXPECT_EQ(data0, array.data());
  for (size_t i = 1; i <= 5; i++) {
    array.push_back(i);
  }
  EXPECT_EQ(array.size(), 5);
  EXPECT_EQ(to_s(array), "12345");

  // Test constructors and assignments
  Vector<int> sourceArray{0, 10, 20, 30, 40};
  CheckVector(sourceArray);

  {
    InlineVector<int, 10> array;
    CheckVector(array);
    InlineVector<int, 5> samllArray{100, 101, 102, 103, 104};
    EXPECT_TRUE((uint8_t*)samllArray.data() - (uint8_t*)&samllArray ==
                sizeof(Vector<int>));

    array = sourceArray;
    CheckVector(array);
    EXPECT_TRUE((uint8_t*)array.data() - (uint8_t*)&array ==
                sizeof(Vector<int>));
    EXPECT_EQ(array.capacity(), 10);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "010203040");

    array = {5, 4, 3, 2, 1};
    CheckVector(array);
    EXPECT_TRUE((uint8_t*)array.data() - (uint8_t*)&array ==
                sizeof(Vector<int>));
    EXPECT_EQ(array.capacity(), 10);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "54321");

    // move sourceArray to array, sourceArray.size() <= array.capacity(), no
    // reallocation
    array = std::move(sourceArray);
    CheckVector(array);
    EXPECT_TRUE((uint8_t*)array.data() - (uint8_t*)&array ==
                sizeof(Vector<int>));
    EXPECT_EQ(array.capacity(), 10);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "010203040");
    EXPECT_TRUE(sourceArray.empty());

    // copy assign samllArray to array, no reallocation
    array = samllArray;
    CheckVector(array);
    EXPECT_TRUE((uint8_t*)array.data() - (uint8_t*)&array ==
                sizeof(Vector<int>));
    EXPECT_EQ(array.capacity(), 10);
    EXPECT_EQ(array.size(), samllArray.size());
    EXPECT_EQ(to_s(array), "100101102103104");

    array = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    CheckVector(array);  // Will reallocate
    EXPECT_FALSE((uint8_t*)array.data() - (uint8_t*)&array ==
                 sizeof(Vector<int>));
    EXPECT_EQ(array.size(), 11);
    EXPECT_EQ(to_s(array), "1234567891011");

    InlineVector<int, 5> array2{1, 2, 3, 4, 5};
    CheckVector(array2);
    EXPECT_TRUE((uint8_t*)array2.data() - (uint8_t*)&array2 ==
                sizeof(Vector<int>));
    EXPECT_EQ(array2.capacity(), 5);
    EXPECT_EQ(array2.size(), 5);
    array2.push_back(6);
    CheckVector(array2);
    EXPECT_FALSE((uint8_t*)array2.data() - (uint8_t*)&array2 ==
                 sizeof(Vector<int>));
    EXPECT_EQ(to_s(array2), "123456");

    InlineVector<int, 10> array3(samllArray);
    CheckVector(array);
    EXPECT_TRUE((uint8_t*)array3.data() - (uint8_t*)&array3 ==
                sizeof(Vector<int>));
    EXPECT_EQ(array3.capacity(), 10);
    EXPECT_EQ(array3.size(), samllArray.size());
    EXPECT_EQ(to_s(array3), "100101102103104");
  }

  {
    InlineVector<int, 10> array0;
    CheckVector(array0);
    for (size_t i = 0; i < array0.capacity(); i++) {
      array0.push_back(i);
    }
    CheckVector(array0);
    EXPECT_TRUE((uint8_t*)array0.data() - (uint8_t*)&array0 ==
                sizeof(Vector<int>));

    InlineVector<int, 10> array1;
    array1 = array0;
    CheckVector(array1);
    EXPECT_TRUE((uint8_t*)array1.data() - (uint8_t*)&array1 ==
                sizeof(Vector<int>));
    EXPECT_EQ(to_s(array1), "0123456789");

    InlineVector<int, 10> array2;
    array2 = std::move(array0);
    CheckVector(array2);
    EXPECT_TRUE((uint8_t*)array2.data() - (uint8_t*)&array2 ==
                sizeof(Vector<int>));
    EXPECT_EQ(to_s(array2), "0123456789");
    EXPECT_TRUE(array0.empty());
  }

  {
    InlineVector<int, 5> array;
    array.resize<false>(5);
    EXPECT_TRUE(array.is_static_buffer());

    array.resize<true>(6);
    EXPECT_FALSE(array.is_static_buffer());
  }
}

TEST(Vector, InlineSafety) {
  static int64_t gAliveCount = 0;
  class NontrivialInt {
   public:
    NontrivialInt(int i = -1) {
      gAliveCount++;
      value_ = std::make_shared<std::string>(std::to_string(i));
    }
    NontrivialInt(NontrivialInt&& other) : value_(std::move(other.value_)) {
      gAliveCount++;
    }
    NontrivialInt(const NontrivialInt& other) : value_(other.value_) {
      gAliveCount++;
    }
    NontrivialInt& operator=(const NontrivialInt& other) = default;
    NontrivialInt& operator=(NontrivialInt&& other) = default;
    ~NontrivialInt() { gAliveCount--; }

    operator int() const { return std::stoi(*value_); }

   private:
    std::shared_ptr<std::string> value_;
  };

  auto to_s = [](const Vector<NontrivialInt>& array) -> std::string {
    std::string result;
    for (auto& i : array) {
      result += std::to_string((int)i);
    }
    return result;
  };

  {
    InlineVector<NontrivialInt, 5> array;
    array.reserve(1);
    array.reserve(5);
    EXPECT_TRUE(array.is_static_buffer());
    array.reserve(6);
    EXPECT_FALSE(array.is_static_buffer());
  }

  {
    InlineVector<NontrivialInt, 5> array;
    EXPECT_TRUE(array.is_static_buffer());
  }

  {
    InlineVector<int, 5> array{100, 101, 102, 103, 104};
    EXPECT_EQ(array.size(), 5);
    EXPECT_TRUE(array.is_static_buffer());

    InlineVector<int, 5> array2{100, 101, 102, 103, 104, 105};
    EXPECT_EQ(array2.size(), 6);
    EXPECT_FALSE(array2.is_static_buffer());

    array2 = {1, 2, 3, 4, 5};
    EXPECT_EQ(array2.size(), 5);
    EXPECT_FALSE(
        array2.is_static_buffer());  // Not using static buffer even though size
                                     // fits to self's static buffer.
  }

  {
    // Copy contructors
    Vector<NontrivialInt> source;
    for (size_t i = 0; i < 5; i++) {
      source.emplace_back(i);
    }
    EXPECT_EQ(to_s(source), "01234");
    EXPECT_EQ(gAliveCount, 5);

    InlineVector<NontrivialInt, 5> array(source);
    EXPECT_TRUE(array.is_static_buffer());
    EXPECT_EQ(to_s(array), "01234");
    EXPECT_EQ(gAliveCount, 10);

    source.emplace_back(5);
    EXPECT_EQ(to_s(source), "012345");
    EXPECT_EQ(gAliveCount, 11);

    InlineVector<NontrivialInt, 5> array2(source);
    EXPECT_FALSE(array2.is_static_buffer());
    EXPECT_EQ(to_s(array2), "012345");
    EXPECT_EQ(gAliveCount, 17);

    decltype(array2) array3(array2);
    EXPECT_FALSE(array3.is_static_buffer());
    EXPECT_EQ(to_s(array3), "012345");
    EXPECT_EQ(gAliveCount, 23);

    InlineVector<NontrivialInt, 6> array4(array3);
    EXPECT_TRUE(array4.is_static_buffer());
    EXPECT_EQ(to_s(array4), "012345");
    EXPECT_EQ(gAliveCount, 29);

    array4.erase(array4.begin());
    EXPECT_EQ(to_s(array4), "12345");
    EXPECT_EQ(gAliveCount, 28);
  }
  EXPECT_EQ(gAliveCount, 0);

  {
    // Copy assignment operator
    Vector<NontrivialInt> source;
    for (size_t i = 0; i < 5; i++) {
      source.emplace_back(i);
    }
    EXPECT_EQ(to_s(source), "01234");

    InlineVector<NontrivialInt, 5> array;
    array = source;
    EXPECT_TRUE(array.is_static_buffer());
    EXPECT_EQ(to_s(array), "01234");

    source.emplace_back(5);
    EXPECT_EQ(to_s(source), "012345");

    InlineVector<NontrivialInt, 5> array2;
    array2 = source;
    EXPECT_FALSE(array2.is_static_buffer());
    EXPECT_EQ(to_s(array2), "012345");

    decltype(array2) array3;
    array3 = array2;
    EXPECT_FALSE(array3.is_static_buffer());
    EXPECT_EQ(to_s(array3), "012345");

    InlineVector<NontrivialInt, 6> array4;
    array4 = array3;
    EXPECT_TRUE(array4.is_static_buffer());
    EXPECT_EQ(to_s(array4), "012345");
  }

  {
    // Move contructors
    Vector<NontrivialInt> source;
    for (size_t i = 0; i < 5; i++) {
      source.emplace_back(i);
    }
    EXPECT_EQ(to_s(source), "01234");
    EXPECT_EQ(gAliveCount, 5);

    InlineVector<NontrivialInt, 5> array(std::move(source));
    EXPECT_TRUE(array.is_static_buffer());
    EXPECT_EQ(to_s(array), "01234");

    EXPECT_TRUE(source.empty());
    EXPECT_EQ(gAliveCount, 5);

    for (size_t i = 0; i < 6; i++) {
      source.emplace_back(i);
    }
    EXPECT_EQ(to_s(source), "012345");
    EXPECT_EQ(gAliveCount, 11);

    const auto data0 = source.data();
    InlineVector<NontrivialInt, 5> array2(std::move(source));
    EXPECT_FALSE(array2.is_static_buffer());
    EXPECT_EQ(to_s(array2), "012345");
    EXPECT_EQ(data0, array2.data());  // buffer moved
    EXPECT_TRUE(source.empty());
    EXPECT_EQ(gAliveCount, 11);

    decltype(array2) array3(std::move(array2));
    EXPECT_FALSE(array3.is_static_buffer());
    EXPECT_EQ(to_s(array3), "012345");
    EXPECT_EQ(data0, array3.data());  // buffer moved
    EXPECT_TRUE(array2.empty());
    EXPECT_EQ(gAliveCount, 11);

    InlineVector<NontrivialInt, 6> array4(std::move(array3));
    EXPECT_TRUE(array4.is_static_buffer());
    EXPECT_EQ(to_s(array4), "012345");
    EXPECT_TRUE(array3.empty());
    EXPECT_EQ(data0, array3.data());  // buffer not moved from array3
    EXPECT_EQ(gAliveCount, 11);
  }
  EXPECT_EQ(gAliveCount, 0);

  {
    // Move assignment operator
    Vector<NontrivialInt> source;
    for (size_t i = 0; i < 5; i++) {
      source.emplace_back(i);
    }
    EXPECT_EQ(to_s(source), "01234");
    EXPECT_EQ(gAliveCount, 5);

    InlineVector<NontrivialInt, 5> array;
    array = std::move(source);
    EXPECT_TRUE(array.is_static_buffer());
    EXPECT_EQ(to_s(array), "01234");

    EXPECT_TRUE(source.empty());
    EXPECT_EQ(gAliveCount, 5);

    for (size_t i = 0; i < 6; i++) {
      source.emplace_back(i);
    }
    EXPECT_EQ(to_s(source), "012345");
    EXPECT_EQ(gAliveCount, 11);

    const auto data0 = source.data();
    InlineVector<NontrivialInt, 5> array2;
    array2 = std::move(source);
    EXPECT_FALSE(array2.is_static_buffer());
    EXPECT_EQ(to_s(array2), "012345");
    EXPECT_EQ(data0, array2.data());  // buffer moved
    EXPECT_TRUE(source.empty());
    EXPECT_EQ(gAliveCount, 11);

    decltype(array2) array3;
    array3 = std::move(array2);
    EXPECT_FALSE(array3.is_static_buffer());
    EXPECT_EQ(to_s(array3), "012345");
    EXPECT_EQ(data0, array3.data());  // buffer moved
    EXPECT_TRUE(array2.empty());
    EXPECT_EQ(gAliveCount, 11);

    InlineVector<NontrivialInt, 6> array4;
    array4 = std::move(array3);
    EXPECT_TRUE(array4.is_static_buffer());
    EXPECT_EQ(to_s(array4), "012345");
    EXPECT_TRUE(array3.empty());
    EXPECT_EQ(data0, array3.data());  // buffer not moved from array3
    EXPECT_EQ(gAliveCount, 11);
  }
  EXPECT_EQ(gAliveCount, 0);
}

TEST(Vector, InlineNontrivial) {
  auto to_s = [](const Vector<std::string>& array) -> std::string {
    std::string result;
    for (auto& i : array) {
      result += i;
    }
    return result;
  };

  InlineVector<std::string, 100> array;
  const auto data0 = array.data();
  EXPECT_TRUE((uint8_t*)data0 - (uint8_t*)&array ==
              sizeof(Vector<std::string>));
  for (size_t i = 1; i <= 80; i++) {
    array.push_back(std::to_string(i));
  }
  EXPECT_EQ(data0, array.data());
  CheckVector(array);

  array.clear();
  CheckVector(array);
  EXPECT_EQ(data0, array.data());
  for (size_t i = 1; i <= 80; i++) {
    array.push_back(std::to_string(i));
  }
  EXPECT_EQ(data0, array.data());
  CheckVector(array);

  EXPECT_EQ(array.size(), 80);
  EXPECT_FALSE(array.reserve(90));
  CheckVector(array);
  EXPECT_EQ(data0, array.data());  // Reserve but no reallocation.
  for (size_t i = 81; i <= 90; i++) {
    array.push_back(std::to_string(i));
  }
  EXPECT_EQ(array.size(), 90);
  CheckVector(array);
  EXPECT_EQ(data0, array.data());  // Still no reallocation.

  EXPECT_FALSE(array.resize(100));
  CheckVector(array);              // Resize but still no reallocation.
  EXPECT_EQ(data0, array.data());  // Resize but no reallocation.
  for (size_t i = 90; i < 100; i++) {
    array[i] = std::to_string(i + 1);
  }
  EXPECT_EQ(array.size(), 100);
  CheckVector(array);

  array.push_back(std::to_string(101));
  CheckVector(array);  // Reallocation
  EXPECT_TRUE(data0 != array.data());
  int sum = 0;
  for (auto i : array) {
    sum += std::stoi(i);
  }
  EXPECT_EQ(sum, 5050 + 101);

  array.clear_and_shrink();
  EXPECT_TRUE(array.empty());
  EXPECT_EQ(data0, array.data());
  for (size_t i = 1; i <= 5; i++) {
    array.push_back(std::to_string(i));
  }
  EXPECT_EQ(array.size(), 5);
  EXPECT_EQ(to_s(array), "12345");

  // Test constructors and assignments
  Vector<std::string> sourceArray{"0", "10", "20", "30", "40"};
  CheckVector(sourceArray);

  {
    InlineVector<std::string, 10> array;
    CheckVector(array);

    array = sourceArray;
    CheckVector(array);
    EXPECT_TRUE((uint8_t*)array.data() - (uint8_t*)&array ==
                sizeof(Vector<std::string>));
    EXPECT_EQ(array.capacity(), 10);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "010203040");

    array = {"5", "4", "3", "2", "1"};
    CheckVector(array);
    EXPECT_TRUE((uint8_t*)array.data() - (uint8_t*)&array ==
                sizeof(Vector<std::string>));
    EXPECT_EQ(array.capacity(), 10);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "54321");

    // move sourceArray to array, sourceArray.size() <= array.capacity(), no
    // reallocation
    array = std::move(sourceArray);
    CheckVector(array);
    EXPECT_TRUE((uint8_t*)array.data() - (uint8_t*)&array ==
                sizeof(Vector<std::string>));
    EXPECT_EQ(array.capacity(), 10);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "010203040");
    EXPECT_TRUE(sourceArray.empty());

    array = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};
    CheckVector(array);  // Will reallocate
    EXPECT_FALSE((uint8_t*)array.data() - (uint8_t*)&array ==
                 sizeof(Vector<std::string>));
    EXPECT_EQ(array.size(), 11);
    EXPECT_EQ(to_s(array), "1234567891011");

    InlineVector<std::string, 5> array2{"1", "2", "3", "4", "5"};
    CheckVector(array2);
    EXPECT_TRUE((uint8_t*)array2.data() - (uint8_t*)&array2 ==
                sizeof(Vector<int>));
    EXPECT_EQ(array2.capacity(), 5);
    EXPECT_EQ(array2.size(), 5);
    array2.push_back("6");
    CheckVector(array2);
    EXPECT_FALSE((uint8_t*)array2.data() - (uint8_t*)&array2 ==
                 sizeof(Vector<int>));
    EXPECT_EQ(to_s(array2), "123456");
  }

  {
    InlineVector<std::string, 10> array0;
    CheckVector(array0);
    for (size_t i = 0; i < array0.capacity(); i++) {
      array0.push_back(std::to_string(i));
    }
    CheckVector(array0);
    EXPECT_TRUE((uint8_t*)array0.data() - (uint8_t*)&array0 ==
                sizeof(Vector<std::string>));

    InlineVector<std::string, 10> array1;
    array1 = array0;
    CheckVector(array1);
    EXPECT_TRUE((uint8_t*)array1.data() - (uint8_t*)&array1 ==
                sizeof(Vector<std::string>));
    EXPECT_EQ(to_s(array1), "0123456789");

    InlineVector<std::string, 10> array2;
    array2 = std::move(array0);
    CheckVector(array2);
    EXPECT_TRUE((uint8_t*)array2.data() - (uint8_t*)&array2 ==
                sizeof(Vector<std::string>));
    EXPECT_EQ(to_s(array2), "0123456789");
    EXPECT_TRUE(array0.empty());
  }
}

TEST(Vector, Trivial) {
  auto to_s = [](const Vector<int>& array) -> std::string {
    std::string result;
    for (int i : array) {
      result += std::to_string(i);
    }
    return result;
  };

  {
    Vector<int> array;
    EXPECT_EQ(array.size(), 0);
    EXPECT_TRUE(array.empty());
    CheckVector(array);
  }

  {
    Vector<int> array(5);
    EXPECT_EQ(array.size(), 5);
    EXPECT_FALSE(array.empty());
    for (size_t i = 0; i < array.size(); i++) {
      EXPECT_EQ(array[i], 0);
      EXPECT_EQ(array.at(i), 0);
    }
    EXPECT_EQ(array.front(), 0);
    EXPECT_EQ(array.back(), 0);
    CheckVector(array);
  }

  {
    int buffer[5] = {10, 11, 12, 13, 14};
    Vector<int> array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    EXPECT_EQ(array.size(), 5);
    for (size_t i = 0; i < array.size(); i++) {
      EXPECT_EQ(array[i], buffer[i]);
    }
    CheckVector(array);

    EXPECT_EQ(array.front(), 10);
    EXPECT_EQ(array.back(), 14);
    EXPECT_TRUE(std::memcmp(buffer, array.data(), sizeof(buffer)) == 0);
  }

  {
    Matrix3 buffer[3] = {Matrix3::zero, Matrix3::identity,
                         Matrix3(1, 2, 3, 4, 5, 6, 7, 8, 9)};
    Vector<Matrix3> array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    EXPECT_EQ(array.size(), 3);
    for (size_t i = 0; i < array.size(); i++) {
      EXPECT_EQ(array[i], buffer[i]);
    }
    EXPECT_EQ(array[2].elements[6], 3.f);
    CheckVector(array);

    // Copy
    Vector<Matrix3> array2(array);
    EXPECT_EQ(array2.size(), 3);
    for (size_t i = 0; i < array2.size(); i++) {
      EXPECT_EQ(array2[i], buffer[i]);
    }
    EXPECT_EQ(array2[2].elements[6], 3.f);
    CheckVector(array2);

    // Copy assign
    Vector<Matrix3> array3(5);
    EXPECT_EQ(array3.size(), 5);
    for (size_t i = 0; i < array3.size(); i++) {
      EXPECT_EQ(array3[i], Matrix3::identity);
    }
    array3 = array2;
    CheckVector(array3);
    EXPECT_EQ(array3.size(), 3);
    for (size_t i = 0; i < array3.size(); i++) {
      EXPECT_EQ(array3[i], buffer[i]);
    }
    EXPECT_EQ(array3[2].elements[6], 3.f);
    CheckVector(array3);
  }

  {
    Vector<Matrix3> array(5);
    EXPECT_EQ(array.size(), 5);
    for (size_t i = 0; i < array.size(); i++) {
      EXPECT_EQ(array[i], Matrix3::identity);
    }
    CheckVector(array);
  }

  {
    Vector<Matrix3> array({});
    EXPECT_TRUE(array.empty());
  }

  {
    // Construct from initializer list or iterators
    Vector<Matrix3> array(
        {Matrix3::zero, Matrix3::identity, Matrix3::zero, Matrix3::identity});
    EXPECT_EQ(array.size(), 4);
    EXPECT_EQ(array[0], Matrix3::zero);
    EXPECT_EQ(array[1], Matrix3::identity);
    EXPECT_EQ(array[2], Matrix3::zero);
    EXPECT_EQ(array[3], Matrix3::identity);
    CheckVector(array);

    Vector<Matrix3> array2(5);
    EXPECT_EQ(array2.size(), 5);
    for (size_t i = 0; i < array2.size(); i++) {
      EXPECT_EQ(array2[i], Matrix3::identity);
    }
    CheckVector(array2);
    array2 = {Matrix3::identity, Matrix3::zero, Matrix3::identity,
              Matrix3::zero};
    CheckVector(array2);
    EXPECT_EQ(array2.size(), 4);
    EXPECT_EQ(array2[0], Matrix3::identity);
    EXPECT_EQ(array2[1], Matrix3::zero);
    EXPECT_EQ(array2[2], Matrix3::identity);
    EXPECT_EQ(array2[3], Matrix3::zero);

    {
      int buffer[5] = {10, 11, 12, 13, 14};
      Vector<int> array(sizeof(buffer) / sizeof(buffer[0]), buffer);
      Vector<int> array2(array.begin(), array.end());
      CheckVector(array2);
      EXPECT_EQ(to_s(array2), "1011121314");
      Vector<int> array3(array.begin() + 1, array.end());
      CheckVector(array3);
      EXPECT_EQ(to_s(array3), "11121314");
      Vector<int> array4(array.begin() + 1, array.end() - 1);
      CheckVector(array4);
      EXPECT_EQ(to_s(array4), "111213");

      Vector<int> array5(array.begin() + 2, array.end() - 2);
      CheckVector(array5);
      EXPECT_EQ(to_s(array5), "12");
      Vector<int> array6(array.begin() + 3, array.end() - 2);
      CheckVector(array6);
      EXPECT_TRUE(array6.empty());
    }

    {
      ByteArray array;
      EXPECT_EQ(array.push_back(1), 1);
      EXPECT_EQ(array.push_back(2), 2);
      EXPECT_EQ(array.push_back(3), 3);
      EXPECT_EQ(array.push_back(4), 4);

      ByteArray array2(array.begin(), array.end());
      CheckVector(array2);
      EXPECT_EQ(array2.size(), 4);
      EXPECT_EQ(array2[0], 1);
      EXPECT_EQ(array2[1], 2);
      EXPECT_EQ(array2[2], 3);
      EXPECT_EQ(array2[3], 4);

      ByteArray array3(&array[0], (&array[array.size() - 1]) + 1);
      CheckVector(array3);
      EXPECT_EQ(array3.size(), 4);
      EXPECT_EQ(array3[0], 1);
      EXPECT_EQ(array3[1], 2);
      EXPECT_EQ(array3[2], 3);
      EXPECT_EQ(array3[3], 4);
    }
  }

  {
    // Move
    int buffer[5] = {10, 11, 12, 13, 14};
    int buffer2[10] = {100, 101, 102, 103, 104, 105, 106, 107, 108, 109};
    Vector<int> array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    CheckVector(array);
    EXPECT_EQ(array.size(), 5);

    Vector<int> array2(std::move(array));
    CheckVector(array2);
    EXPECT_TRUE(array.empty());
    EXPECT_EQ(array2.size(), 5);
    for (size_t i = 0; i < array2.size(); i++) {
      EXPECT_EQ(array2[i], buffer[i]);
    }

    Vector<int> array3(sizeof(buffer2) / sizeof(buffer2[0]), buffer2);
    CheckVector(array3);
    EXPECT_EQ(array3.size(), 10);
    array = std::move(array3);
    CheckVector(array);
    EXPECT_TRUE(array3.empty());
    EXPECT_EQ(array.size(), 10);
    for (size_t i = 0; i < array.size(); i++) {
      EXPECT_EQ(array[i], buffer2[i]);
    }
  }

  {
    // Basic push and pop
    Vector<int> array;
    for (int i = 1; i <= 100; i++) {
      array.push_back(i);
      CheckVector(array);
    }
    int sum = 0;
    for (size_t i = 0; i < array.size(); i++) {
      sum += array[i];
    }
    EXPECT_EQ(sum, 5050);

    int buffer[5] = {10, 11, 12, 13, 14};
    for (size_t i = 0; i < 5; i++) {
      array.push_back(buffer[i]);
    }
    CheckVector(array);
    EXPECT_EQ(array.size(), 105);
    sum = 0;
    for (size_t i = 0; i < array.size(); i++) {
      sum += array[i];
    }
    EXPECT_EQ(sum, 5050 + 10 + 11 + 12 + 13 + 14);

    for (int i = 0; i < 5; i++) {
      array.pop_back();
      CheckVector(array);
    }
    EXPECT_EQ(array.size(), 100);
    sum = 0;
    for (int i : array) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);

    array.grow() = 9999;
    EXPECT_EQ(array.size(), 101);
    EXPECT_EQ(array.back(), 9999);

    array.grow(200);
    EXPECT_EQ(array.size(), 200);
  }

  {
    // Iterators
    std::string output;
    int buffer[5] = {10, 11, 12, 13, 14};
    Vector<int> array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    for (auto it = array.begin(); it != array.end(); it++) {
      output += std::to_string(*it);
    }
    for (auto it = array.cbegin(); it != array.cend(); it++) {
      output += std::to_string(*it);
    }
    for (int i : array) {
      output += std::to_string(i);
    }
    EXPECT_EQ(output, "101112131410111213141011121314");

    output = "";
    for (auto it = array.rbegin(); it != array.rend(); it++) {
      output += std::to_string(*it);
    }
    for (auto it = array.crbegin(); it != array.crend(); it++) {
      output += std::to_string(*it);
    }
    EXPECT_EQ(output, "14131211101413121110");

    // Writable iterator
    output = "";
    for (auto& i : array) {
      i += 1;
    }
    for (auto it = array.begin(); it != array.end(); it++) {
      *it += 1;
    }
    for (int i : array) {
      output += std::to_string(i);
    }
    EXPECT_EQ(output, "1213141516");
  }

  {
    // Erase and insert
    int buffer[5] = {10, 11, 12, 13, 14};
    Vector<int> array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    for (int i = 0; i < 5; i++) {
      auto it = array.erase(array.begin());
      CheckVector(array);
      EXPECT_EQ(it, array.begin());
      if (i == 2) {
        EXPECT_EQ(to_s(array), "1314");
      }
    }

    EXPECT_TRUE(array.empty());
    for (int i = 4; i >= 0; i--) {
      array.insert(array.begin(), i);
      CheckVector(array);
    }
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "01234");

    auto it = array.erase(array.begin() + 1, array.begin() + 3);
    CheckVector(array);
    EXPECT_EQ(to_s(array), "034");
    EXPECT_EQ(*it, 3);

    it = array.erase(array.end() - 1);
    CheckVector(array);
    EXPECT_EQ(to_s(array), "03");
    EXPECT_EQ(it, array.end());

    it = array.erase(array.begin(), array.end());
    CheckVector(array);
    EXPECT_TRUE(array.empty());
    EXPECT_EQ(it, array.end());

    array.insert(array.begin(), 50);
    CheckVector(array);
    array.insert(array.end(), 51);
    CheckVector(array);
    array.insert(array.end(), 52);
    CheckVector(array);
    array.insert(array.begin() + 1, 49);
    CheckVector(array);
    array.insert(array.begin(), 48);
    CheckVector(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "4850495152");

    Vector<int> array2;
    for (int i = 1; i <= 100; i++) {
      array2.insert(array2.begin() + i - 1, i);
    }
    CheckVector(array2);
    int sum = 0;
    for (int i : array2) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);

    Vector<int> array3;
    for (int i = 1; i <= 100; i++) {
      array3.insert(array3.begin(), i);
    }
    CheckVector(array3);
    sum = 0;
    for (int i : array3) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);
  }

  {
    // Reserve
    Vector<int> array;
    EXPECT_TRUE(array.reserve(100));
    CheckVector(array);
    auto data_p = array.data();
    for (int i = 1; i <= 100; i++) {
      array.emplace_back(i);
      CheckVector(array);
    }
    EXPECT_EQ(data_p, array.data());
  }

  {
    // Resize
    Vector<float> farray;
    farray.resize<true>(10);
    EXPECT_EQ(farray.size(), 10);
    for (auto f : farray) {
      EXPECT_EQ(f, 0.0f);
    }
    farray.resize<true>(1);
    EXPECT_EQ(farray.size(), 1);
    EXPECT_EQ(farray[0], 0.0f);
    farray.resize<true>(5, 3.14f);
    EXPECT_EQ(farray.size(), 5);
    EXPECT_EQ(farray[0], 0.0f);
    for (size_t i = 1; i < farray.size(); i++) {
      EXPECT_EQ(farray[i], 3.14f);
    }

    Vector<Matrix3> marray;
    marray.resize<true>(10);
    EXPECT_EQ(marray.size(), 10);
    for (auto f : marray) {
      EXPECT_EQ(f, Matrix3::identity);
    }
    marray.resize<true>(1);
    EXPECT_EQ(marray.size(), 1);
    EXPECT_EQ(marray[0], Matrix3::identity);
    marray.resize<true>(5, Matrix3::zero);
    EXPECT_EQ(marray.size(), 5);
    EXPECT_EQ(marray[0], Matrix3::identity);
    for (size_t i = 1; i < marray.size(); i++) {
      EXPECT_EQ(marray[i], Matrix3::zero);
    }

    Vector<int> array;
    EXPECT_FALSE(array.resize<false>(0));
    EXPECT_TRUE(array.empty());
    EXPECT_EQ(array.capacity(), 0);
    EXPECT_FALSE(array.resize<true>(0, 5));
    EXPECT_TRUE(array.empty());
    EXPECT_EQ(array.capacity(), 0);

    EXPECT_TRUE(array.resize<true>(50, 5));
    CheckVector(array);
    EXPECT_EQ(array.size(), 50);
    for (int i : array) {
      EXPECT_EQ(i, 5);
    }

    EXPECT_TRUE(array.resize<true>(100, 6));
    CheckVector(array);
    EXPECT_EQ(array.size(), 100);
    for (size_t i = 0; i < 50; i++) {
      EXPECT_EQ(array[i], 5);
    }
    for (size_t i = 50; i < 100; i++) {
      EXPECT_EQ(array[i], 6);
    }

    EXPECT_FALSE(array.resize<false>(10));
    CheckVector(array);
    EXPECT_EQ(array.size(), 10);
    for (size_t i = 0; i < 10; i++) {
      EXPECT_EQ(array[i], 5);
    }

    array.clear_and_shrink();
    EXPECT_TRUE(array.empty());
    array.resize<true>(5, 5);
    EXPECT_EQ(to_s(array), "55555");
  }

  {
    // Algorithm
    int buffer[5] = {12, 11, 15, 14, 10};
    Vector<int> array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    std::sort(array.begin(), array.end(), [](int& a, int& b) { return a < b; });
    CheckVector(array);
    EXPECT_EQ(to_s(array), "1011121415");

    Vector<int> array2(sizeof(buffer) / sizeof(buffer[0]), buffer);
    InsertionSort(array2.begin(), array2.end(),
                  [](int& a, int& b) { return a < b; });
    CheckVector(array2);
    EXPECT_EQ(to_s(array2), "1011121415");

    Vector<int> array3(sizeof(buffer) / sizeof(buffer[0]), buffer);
    InsertionSort(array3.begin() + 1, array3.end(),
                  [](int& a, int& b) { return a < b; });
    CheckVector(array3);
    EXPECT_EQ(to_s(array3), "1210111415");

    Vector<int> array4(sizeof(buffer) / sizeof(buffer[0]), buffer);
    InsertionSort(array4.begin() + 1, array4.end() - 1,
                  [](int& a, int& b) { return a < b; });
    CheckVector(array4);
    EXPECT_EQ(to_s(array4), "1211141510");
  }

  {
    // Fill and append
    int buffer[5] = {10, 11, 12, 13, 14};
    int buffer2[5] = {20, 21, 22, 23, 24};
    Vector<int> array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    array.fill(buffer2, sizeof(buffer2));
    CheckVector(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "2021222324");

    // fill buffer again but from index 3
    array.fill(buffer, sizeof(buffer), 3);
    CheckVector(array);
    EXPECT_EQ(array.size(), 8);
    EXPECT_EQ(to_s(array), "2021221011121314");

    array.fill(nullptr, sizeof(int) * 2, 1);
    CheckVector(array);
    EXPECT_EQ(to_s(array), "2000");

    array.append(buffer2, sizeof(int) * 3);
    CheckVector(array);
    EXPECT_EQ(to_s(array), "2000202122");

    Vector<int> array2(sizeof(buffer) / sizeof(buffer[0]), buffer);
    array.append(array2);
    CheckVector(array);
    EXPECT_EQ(to_s(array), "20002021221011121314");

    Vector<int> array3;
    array3.append(nullptr, 0);
    EXPECT_TRUE(array3.empty());
    array3.append(buffer, 0);
    EXPECT_TRUE(array3.empty());
    array3.append(buffer2, sizeof(int) * 3);
    CheckVector(array3);
    EXPECT_EQ(to_s(array3), "202122");
    array3.append(nullptr, 0);
    CheckVector(array3);
    EXPECT_EQ(to_s(array3), "202122");
    array3.append(buffer, 0);
    CheckVector(array3);
    EXPECT_EQ(to_s(array3), "202122");
  }

  {
    // swap
    int buffer[5] = {10, 11, 12, 13, 14};
    int buffer2[5] = {20, 21, 22, 23, 24};
    Vector<int> array1(sizeof(buffer) / sizeof(buffer[0]), buffer);
    Vector<int> array2(sizeof(buffer2) / sizeof(buffer2[0]), buffer2);
    array1.swap(array2);
    EXPECT_EQ(to_s(array1), "2021222324");
    CheckVector(array1);
    EXPECT_EQ(to_s(array2), "1011121314");
    CheckVector(array2);
  }

  {
    // Templateless methods.
    int buffer[5] = {10, 11, 12, 13, 14};
    Vector<int> array(sizeof(buffer) / sizeof(buffer[0]), buffer);

    VectorTemplateless::PushBackBatch(&array, sizeof(int), buffer, 5);
    CheckVector(array);
    EXPECT_EQ(to_s(array), "10111213141011121314");
  }
}

TEST(Vector, Nontrivial) {
  class NontrivialInt {
   public:
    NontrivialInt(int i = -1) {
      value_ = std::make_shared<std::string>(std::to_string(i));
    }

    operator int() const { return std::stoi(*value_); }
    bool operator==(const NontrivialInt& other) {
      return (int)(*this) == (int)other;
    }
    NontrivialInt& operator+=(int value) {
      value_ = std::make_shared<std::string>(
          std::to_string(std::stoi(*value_) + value));
      return *this;
    }

   private:
    std::shared_ptr<std::string> value_;
  };

  auto to_nt_int_array = [](size_t count,
                            int buffer[]) -> Vector<NontrivialInt> {
    Vector<NontrivialInt> result;
    for (size_t i = 0; i < count; i++) {
      result.emplace_back(buffer[i]);
    }
    return result;
  };

  auto to_s = [](const Vector<NontrivialInt>& array) -> std::string {
    std::string result;
    for (auto& i : array) {
      result += std::to_string((int)i);
    }
    return result;
  };

  NontrivialInt ni10000(10000);
  NontrivialInt ni10001(10001);
  NontrivialInt ni10002(10002);
  NontrivialInt ni10003(10003);

  {
    Vector<NontrivialInt> array;
    EXPECT_EQ(array.size(), 0);
    EXPECT_TRUE(array.empty());
    CheckVector(array);
  }

  {
    Vector<NontrivialInt> array(5);
    EXPECT_EQ(array.size(), 5);
    EXPECT_FALSE(array.empty());
    for (size_t i = 0; i < array.size(); i++) {
      EXPECT_EQ(array[i], -1);
      EXPECT_EQ(array.at(i), -1);
    }
    EXPECT_EQ(array.front(), -1);
    EXPECT_EQ(array.back(), -1);
    CheckVector(array);
  }

  {
    Vector<Matrix3> array({});
    EXPECT_TRUE(array.empty());
  }

  {
    // Construct from initializer list or iterators
    Vector<NontrivialInt> array({ni10000, ni10001, ni10002, ni10003});
    EXPECT_EQ(array.size(), 4);
    EXPECT_EQ(array[0], ni10000);
    EXPECT_EQ(array[1], ni10001);
    EXPECT_EQ(array[2], ni10002);
    EXPECT_EQ(array[3], ni10003);
    CheckVector(array);

    Vector<NontrivialInt> array2(5);
    EXPECT_EQ(array2.size(), 5);
    CheckVector(array2);
    for (size_t i = 0; i < array2.size(); i++) {
      EXPECT_EQ(array2[i], -1);
    }
    array2 = {ni10000, ni10001, ni10002, ni10003};
    CheckVector(array2);
    EXPECT_EQ(array2.size(), 4);
    EXPECT_EQ(array2[0], ni10000);
    EXPECT_EQ(array2[1], ni10001);
    EXPECT_EQ(array2[2], ni10002);
    EXPECT_EQ(array2[3], ni10003);

    Vector<NontrivialInt> array3(array2);
    CheckVector(array3);
    EXPECT_EQ(array3.size(), 4);
    EXPECT_EQ(array3[0], ni10000);
    EXPECT_EQ(array3[1], ni10001);
    EXPECT_EQ(array3[2], ni10002);
    EXPECT_EQ(array3[3], ni10003);

    {
      int buffer[5] = {10, 11, 12, 13, 14};
      Vector<NontrivialInt> array =
          to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
      Vector<NontrivialInt> array2(array.begin(), array.end());
      CheckVector(array2);
      EXPECT_EQ(to_s(array2), "1011121314");
      Vector<NontrivialInt> array3(array.begin() + 1, array.end());
      CheckVector(array3);
      EXPECT_EQ(to_s(array3), "11121314");
      Vector<NontrivialInt> array4(array.begin() + 1, array.end() - 1);
      CheckVector(array4);
      EXPECT_EQ(to_s(array4), "111213");

      Vector<NontrivialInt> array5(array.begin() + 2, array.end() - 2);
      CheckVector(array5);
      EXPECT_EQ(to_s(array5), "12");
      Vector<NontrivialInt> array6(array.begin() + 3, array.end() - 2);
      CheckVector(array6);
      EXPECT_TRUE(array6.empty());
    }
  }

  {
    // Move
    int buffer[5] = {10, 11, 12, 13, 14};
    int buffer2[10] = {100, 101, 102, 103, 104, 105, 106, 107, 108, 109};
    Vector<NontrivialInt> array =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    CheckVector(array);
    EXPECT_EQ(array.size(), 5);

    Vector<NontrivialInt> array2(std::move(array));
    CheckVector(array2);
    EXPECT_TRUE(array.empty());
    EXPECT_EQ(array2.size(), 5);
    for (size_t i = 0; i < array2.size(); i++) {
      EXPECT_EQ(array2[i], buffer[i]);
    }

    Vector<NontrivialInt> array3 =
        to_nt_int_array(sizeof(buffer2) / sizeof(buffer2[0]), buffer2);
    CheckVector(array3);
    EXPECT_EQ(array3.size(), 10);
    array = std::move(array3);
    CheckVector(array);
    EXPECT_TRUE(array3.empty());
    EXPECT_EQ(array.size(), 10);
    for (size_t i = 0; i < array.size(); i++) {
      EXPECT_EQ(array[i], buffer2[i]);
    }
  }

  {
    // Basic push and pop
    Vector<NontrivialInt> array;
    for (int i = 1; i <= 100; i++) {
      EXPECT_EQ(array.push_back(i), i);
      CheckVector(array);
    }
    int sum = 0;
    for (size_t i = 0; i < array.size(); i++) {
      sum += array[i];
    }
    EXPECT_EQ(sum, 5050);

    int buffer[5] = {10, 11, 12, 13, 14};
    for (int i = 0; i < 5; i++) {
      array.push_back(buffer[i]);
    }
    EXPECT_EQ(array.size(), 105);
    sum = 0;
    for (size_t i = 0; i < array.size(); i++) {
      sum += array[i];
    }
    EXPECT_EQ(sum, 5050 + 10 + 11 + 12 + 13 + 14);

    for (int i = 0; i < 5; i++) {
      array.pop_back();
      CheckVector(array);
    }
    EXPECT_EQ(array.size(), 100);
    sum = 0;
    for (int i : array) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);

    EXPECT_EQ(array.emplace_back(999), 999);

    array.grow() = 9999;
    EXPECT_EQ(array.size(), 102);
    EXPECT_EQ(array.back(), 9999);

    array.grow(200);
    EXPECT_EQ(array.size(), 200);
    EXPECT_EQ(array.back(), -1);
  }

  {
    // Iterators
    std::string output;
    int buffer[5] = {10, 11, 12, 13, 14};
    Vector<NontrivialInt> array =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    for (auto it = array.begin(); it != array.end(); it++) {
      output += std::to_string(*it);
    }
    for (auto it = array.cbegin(); it != array.cend(); it++) {
      output += std::to_string(*it);
    }
    for (int i : array) {
      output += std::to_string(i);
    }
    EXPECT_EQ(output, "101112131410111213141011121314");

    output = "";
    for (auto it = array.rbegin(); it != array.rend(); it++) {
      output += std::to_string(*it);
    }
    for (auto it = array.crbegin(); it != array.crend(); it++) {
      output += std::to_string(*it);
    }
    EXPECT_EQ(output, "14131211101413121110");

    // Writable iterator
    output = "";
    for (auto& i : array) {
      i += 1;
    }
    for (auto it = array.begin(); it != array.end(); it++) {
      *it += 1;
    }
    for (int i : array) {
      output += std::to_string(i);
    }
    EXPECT_EQ(output, "1213141516");
  }

  {
    // Erase and insert
    int buffer[5] = {10, 11, 12, 13, 14};
    Vector<NontrivialInt> array =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    for (int i = 0; i < 5; i++) {
      auto it = array.erase(array.begin());
      CheckVector(array);
      EXPECT_EQ(it, array.begin());
      if (i == 2) {
        EXPECT_EQ(to_s(array), "1314");
      }
    }

    EXPECT_TRUE(array.empty());
    for (int i = 4; i >= 0; i--) {
      array.insert(array.begin(), i);
      CheckVector(array);
    }
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "01234");

    auto it = array.erase(array.begin() + 1, array.begin() + 3);
    CheckVector(array);
    EXPECT_EQ(to_s(array), "034");
    EXPECT_EQ(*it, 3);

    it = array.erase(array.end() - 1);
    CheckVector(array);
    EXPECT_EQ(to_s(array), "03");
    EXPECT_EQ(it, array.end());

    it = array.erase(array.begin(), array.end());
    CheckVector(array);
    EXPECT_TRUE(array.empty());
    EXPECT_EQ(it, array.end());

    array.insert(array.begin(), 50);
    CheckVector(array);
    array.insert(array.end(), 51);
    CheckVector(array);
    array.insert(array.end(), 52);
    CheckVector(array);
    array.insert(array.begin() + 1, 49);
    CheckVector(array);
    array.insert(array.begin(), 48);
    CheckVector(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "4850495152");

    Vector<NontrivialInt> array2;
    for (int i = 1; i <= 100; i++) {
      array2.insert(array2.begin() + i - 1, i);
    }
    CheckVector(array2);
    int sum = 0;
    for (int i : array2) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);

    Vector<NontrivialInt> array3;
    for (int i = 1; i <= 100; i++) {
      array3.insert(array3.begin(), i);
    }
    CheckVector(array3);
    sum = 0;
    for (int i : array3) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);
  }

  {
    // Reserve
    Vector<NontrivialInt> array;
    EXPECT_TRUE(array.reserve(100));
    CheckVector(array);
    auto data_p = array.data();
    for (int i = 1; i <= 100; i++) {
      array.emplace_back(i);
      CheckVector(array);
    }
    EXPECT_EQ(data_p, array.data());
  }

  {
    // Resize
    Vector<NontrivialInt> array;
    EXPECT_TRUE(array.resize(50, 5));
    CheckVector(array);
    EXPECT_EQ(array.size(), 50);
    for (int i : array) {
      EXPECT_EQ(i, 5);
    }

    EXPECT_TRUE(array.resize(100, 6));
    CheckVector(array);
    EXPECT_EQ(array.size(), 100);
    for (size_t i = 0; i < 50; i++) {
      EXPECT_EQ(array[i], 5);
    }
    for (size_t i = 50; i < 100; i++) {
      EXPECT_EQ(array[i], 6);
    }
    EXPECT_FALSE(array.resize(10));
    CheckVector(array);
    EXPECT_EQ(to_s(array), "5555555555");
    EXPECT_FALSE(array.resize(0));
    CheckVector(array);
    EXPECT_TRUE(array.empty());

    array.push_back(1);
    EXPECT_EQ(to_s(array), "1");
    array.clear_and_shrink();
    EXPECT_TRUE(array.empty());
    array.resize(5, 5);
    EXPECT_EQ(to_s(array), "55555");
  }

  {
    // Algorithm
    int buffer[5] = {12, 11, 15, 14, 10};
    Vector<NontrivialInt> array =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    std::sort(
        array.begin(), array.end(),
        [](NontrivialInt& a, NontrivialInt& b) { return (int)a < (int)b; });
    CheckVector(array);
    EXPECT_EQ(to_s(array), "1011121415");

    Vector<NontrivialInt> array2 =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    InsertionSort(
        array2.begin(), array2.end(),
        [](NontrivialInt& a, NontrivialInt& b) { return (int)a < (int)b; });
    CheckVector(array2);
    EXPECT_EQ(to_s(array2), "1011121415");

    Vector<NontrivialInt> array3 =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    InsertionSort(
        array3.begin() + 1, array3.end(),
        [](NontrivialInt& a, NontrivialInt& b) { return (int)a < (int)b; });
    CheckVector(array3);
    EXPECT_EQ(to_s(array3), "1210111415");

    Vector<NontrivialInt> array4 =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    InsertionSort(
        array4.begin() + 1, array4.end() - 1,
        [](NontrivialInt& a, NontrivialInt& b) { return (int)a < (int)b; });
    CheckVector(array4);
    EXPECT_EQ(to_s(array4), "1211141510");
  }

  {
    // swap
    int buffer[5] = {12, 11, 15, 14, 10};
    int buffer2[5] = {22, 21, 25, 24, 20};
    Vector<NontrivialInt> array1 =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    Vector<NontrivialInt> array2 =
        to_nt_int_array(sizeof(buffer2) / sizeof(buffer2[0]), buffer2);
    array1.swap(array2);
    EXPECT_EQ(to_s(array1), "2221252420");
    CheckVector(array1);
    EXPECT_EQ(to_s(array2), "1211151410");
    CheckVector(array2);
  }
}

TEST(Vector, Nontrivial2) {
  static int LiveInstance = 0;
  class NontrivialInt {
   public:
    NontrivialInt(int i = -1) {
      ++LiveInstance;
      value_ = std::to_string(i);
    }
    NontrivialInt(const NontrivialInt& other) : value_(other.value_) {
      ++LiveInstance;
    }
    NontrivialInt(NontrivialInt&& other) : value_(std::move(other.value_)) {
      ++LiveInstance;
    }
    NontrivialInt& operator=(const NontrivialInt& other) {
      if (this != &other) {
        value_ = other.value_;
      }
      return *this;
    }
    NontrivialInt& operator=(NontrivialInt&& other) {
      if (this != &other) {
        value_ = std::move(other.value_);
      }
      return *this;
    }
    ~NontrivialInt() { --LiveInstance; }

    operator int() const { return std::stoi(value_); }

   private:
    std::string value_;
  };

  {
    Vector<NontrivialInt> array;
    for (int i = 0; i < 100; i++) {
      array.push_back(NontrivialInt(i));
    }
    EXPECT_EQ(LiveInstance, 100);
    array.resize(200, NontrivialInt(9999));
    EXPECT_EQ(LiveInstance, 200);
    for (int i = 100; i < 200; i++) {
      EXPECT_EQ(array[i], 9999);
    }
    array.erase(array.begin() + 5, array.begin() + 10);
    EXPECT_EQ(LiveInstance, 195);
    array.pop_back();
    EXPECT_EQ(LiveInstance, 194);
    EXPECT_FALSE(array.resize(100));
    EXPECT_EQ(LiveInstance, 100);
    array.clear();
    EXPECT_EQ(LiveInstance, 0);
  }

  {
    int i = 1;
    std::string output;
    Vector<Vector<NontrivialInt>> arrays;
    for (int level = 0; level < 100; level++) {
      arrays.push_back(Vector<NontrivialInt>());
      for (int num = 0; num < level + 1; num++) {
        output += std::to_string(i);
        arrays[level].push_back(NontrivialInt(i++));
      }
    }
    EXPECT_EQ(LiveInstance, (1 + 100) * 100 / 2);
    std::string output2;
    for (int level = 0; level < 100; level++) {
      for (int num = 0; num < level + 1; num++) {
        output2 += std::to_string((arrays[level][num]).operator int());
      }
    }
    EXPECT_EQ(output, output2);
  }
  EXPECT_EQ(LiveInstance, 0);

  // Put in Inline array and test deallocation.
  {
    InlineVector<NontrivialInt, 10> array{1, 2, 3, 4, 5};
    EXPECT_EQ(LiveInstance, 5);
  }
  EXPECT_EQ(LiveInstance, 0);
}

TEST(Vector, PairElement) {
  static_assert(Vector<int>::is_trivial, "");
  static_assert(Vector<std::pair<float, float>>::is_trivial, "");
  static_assert(Vector<std::pair<std::pair<long, long>, int>>::is_trivial, "");
  static_assert(
      Vector<
          std::pair<std::pair<long, long>, std::pair<char, char>>>::is_trivial,
      "");
  static_assert(!Vector<std::string>::is_trivial, "");
  static_assert(!Vector<std::pair<std::string, int>>::is_trivial, "");
  static_assert(!Vector<std::pair<std::pair<long, long>,
                                  std::pair<std::string, char>>>::is_trivial,
                "");

  {
    Vector<std::pair<int, int>> array;
    static_assert(decltype(array)::is_trivial, "");

    array.resize<true>(100, {50, 50});
    for (size_t i = 0; i < 100; i++) {
      EXPECT_EQ(array[i].first, 50);
      EXPECT_EQ(array[i].second, 50);
    }

    for (size_t i = 0; i < 100; i++) {
      array[i].first = i;
      array[i].second = i;
    }

    array.erase(array.begin(), array.begin() + 50);
    EXPECT_EQ(array.size(), 50);
    for (size_t i = 0; i < 50; i++) {
      EXPECT_EQ(array[i].first, i + 50);
      EXPECT_EQ(array[i].second, i + 50);
    }
  }

  {
    Vector<std::pair<std::pair<int, int>, std::pair<int, int>>> array;
    static_assert(decltype(array)::is_trivial, "");

    std::pair<std::pair<int, int>, std::pair<int, int>> buffer[5] = {
        {{0, 0}, {0, 0}},
        {{1, 1}, {1, 1}},
        {{2, 2}, {2, 2}},
        {{3, 3}, {3, 3}},
        {{4, 4}, {4, 4}}};
    VectorTemplateless::PushBackBatch(
        &array, sizeof(std::pair<std::pair<int, int>, std::pair<int, int>>),
        buffer, 5);
    EXPECT_EQ(array.size(), 5);
    for (int i = 0; i < 5; i++) {
      EXPECT_EQ(array[i].first.first, i);
      EXPECT_EQ(array[i].first.second, i);
      EXPECT_EQ(array[i].second.first, i);
      EXPECT_EQ(array[i].second.second, i);
    }
  }
}

TEST(Vector, DestructOrder) {
  // To be consistent with std::vector. Elements are destructed from back.
  static std::string sDestructionOrder;

  class NontrivialInt {
   public:
    NontrivialInt(int i = -1) {
      value_ = std::make_shared<std::string>(std::to_string(i));
    }
    ~NontrivialInt() {
      if (value_) {
        sDestructionOrder += *value_;
      }
    }
    operator int() const { return std::stoi(*value_); }

   private:
    std::shared_ptr<std::string> value_;
  };

  {
    Vector<NontrivialInt> v;
    for (int i = 0; i < 5; i++) {
      v.emplace_back(i);
    }
    sDestructionOrder = "";
  }
  EXPECT_TRUE(sDestructionOrder == "43210");

  {
    Vector<NontrivialInt> v;
    for (int i = 0; i < 5; i++) {
      v.emplace_back(i);
    }
    sDestructionOrder = "";
    v.clear();
  }
  EXPECT_TRUE(sDestructionOrder == "43210");

  {
    Vector<NontrivialInt> v;
    for (int i = 0; i < 5; i++) {
      v.emplace_back(i);
    }
    sDestructionOrder = "";
    v.erase(v.begin() + 1, v.begin() + 3);
    EXPECT_TRUE(sDestructionOrder == "43");
  }
}

TEST(Vector, Slice) {
  Vector<uint32_t> array;
  for (int i = 0; i < 100; i++) {
    array.push_back(i);
  }
  EXPECT_EQ(array.size(), 100);

  EXPECT_TRUE(VectorTemplateless::Erase(&array, 4, 0, 0));
  EXPECT_EQ(array.size(), 100);

  EXPECT_TRUE(VectorTemplateless::Erase(&array, 4, 99, 0));
  EXPECT_EQ(array.size(), 100);
  for (int i = 0; i < 100; i++) {
    // Data not changed.
    EXPECT_EQ(array[i], i);
  }

  // DeleteCount == 0 is allowed but index 100 is out of range, so return false.
  EXPECT_FALSE(VectorTemplateless::Erase(&array, 4, 100, 0));
  EXPECT_EQ(array.size(), 100);

  EXPECT_TRUE(VectorTemplateless::Erase(&array, 4, 0, 50));

  EXPECT_EQ(array.size(), 50);
  EXPECT_EQ(array[0], 50);

  EXPECT_TRUE(VectorTemplateless::Erase(&array, 4, 10, 10));

  EXPECT_EQ(array.size(), 40);
  EXPECT_EQ(array[0], 50);
  EXPECT_EQ(array[10], 70);

  EXPECT_FALSE(VectorTemplateless::Erase(&array, 4, 10, 100));
  EXPECT_EQ(array.size(), 40);

  EXPECT_TRUE(VectorTemplateless::Erase(&array, 4, 0, 40));
  EXPECT_EQ(array.size(), 0);
}

TEST(Vector, Compare) {
  class NontrivialInt {
   public:
    NontrivialInt(int i = -1) {
      value_ = std::make_shared<std::string>(std::to_string(i));
    }

    operator int() const { return std::stoi(*value_); }
    bool operator==(const NontrivialInt& other) {
      return (int)(*this) == (int)other;
    }
    bool operator<(const NontrivialInt& other) {
      return (int)(*this) < (int)other;
    }
    NontrivialInt& operator+=(int value) {
      value_ = std::make_shared<std::string>(
          std::to_string(std::stoi(*value_) + value));
      return *this;
    }

   private:
    std::shared_ptr<std::string> value_;
  };

  auto to_nt_int_array = [](size_t count,
                            int buffer[]) -> Vector<NontrivialInt> {
    Vector<NontrivialInt> result;
    for (size_t i = 0; i < count; i++) {
      result.emplace_back(buffer[i]);
    }
    return result;
  };

  {
    int buffer1[] = {1, 2, 3, 4, 5};
    int buffer2[] = {5, 4, 3, 2, 1};
    Vector<NontrivialInt> vec1 =
        to_nt_int_array(sizeof(buffer1) / sizeof(buffer1[0]), buffer1);
    Vector<NontrivialInt> vec2 =
        to_nt_int_array(sizeof(buffer2) / sizeof(buffer2[0]), buffer2);
    EXPECT_FALSE(vec1 == vec2);
    EXPECT_TRUE(vec1 != vec2);
    std::reverse(vec1.begin(), vec1.end());
    EXPECT_TRUE(vec1 == vec2);
    EXPECT_FALSE(vec1 != vec2);
  }

  {
    int buffer1[] = {1, 2, 3, 4, 5};
    int buffer2[] = {1, 2, 2, 4, 5};
    Vector<NontrivialInt> vec1 =
        to_nt_int_array(sizeof(buffer1) / sizeof(buffer1[0]), buffer1);
    Vector<NontrivialInt> vec2 =
        to_nt_int_array(sizeof(buffer2) / sizeof(buffer2[0]), buffer2);
    EXPECT_TRUE(vec1 > vec2);
  }

  {
    int buffer1[] = {1, 2, 3, 4, 5};
    int buffer2[] = {1, 2, 3, 4};
    Vector<NontrivialInt> vec1 =
        to_nt_int_array(sizeof(buffer1) / sizeof(buffer1[0]), buffer1);
    Vector<NontrivialInt> vec2 =
        to_nt_int_array(sizeof(buffer2) / sizeof(buffer2[0]), buffer2);
    EXPECT_TRUE(vec1 > vec2);
  }

  {
    int buffer1[] = {1};
    Vector<NontrivialInt> vec1 =
        to_nt_int_array(sizeof(buffer1) / sizeof(buffer1[0]), buffer1);
    Vector<NontrivialInt> vec2;
    EXPECT_TRUE(vec1 > vec2);
  }
}

TEST(Vector, StackContainer) {
  std::stack<int, InlineVector<int, 5>> stack;
  stack.push(1);
  stack.push(2);
  EXPECT_EQ(stack.size(), 2);
  EXPECT_EQ(stack.top(), 2);
  stack.pop();
  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack.top(), 1);
  stack.push(3);
  stack.push(4);
  EXPECT_EQ(stack.size(), 3);
  EXPECT_EQ(stack.top(), 4);
  std::string content;
  while (!stack.empty()) {
    content += std::to_string(stack.top());
    stack.pop();
  }
  EXPECT_TRUE(stack.empty());
  EXPECT_EQ(content, "431");
}

TEST(Vector, Algorithms) {
  auto to_s = [](const Vector<int>& array) -> std::string {
    std::string result;
    for (int i : array) {
      result += std::to_string(i);
    }
    return result;
  };

  {
    Vector<int> vec;
    vec.resize<false>(10);
    std::iota(vec.begin(), vec.end(), 0);
    EXPECT_EQ(to_s(vec), "0123456789");
  }

  {
    Vector<int> vec = {1, 2, 3, 4, 5};
    std::string cat;
    std::for_each(vec.begin(), vec.end(),
                  [&](int i) { cat += std::to_string(i); });
    EXPECT_TRUE(cat == "12345");

    std::for_each(vec.rbegin(), vec.rend(),
                  [&](int i) { cat += std::to_string(i); });
    EXPECT_TRUE(cat == "1234554321");
  }

  {
    Vector<int> vec = {1, 2, 3, 4, 5, 4, 3, 2, 1};
    std::sort(vec.begin(), vec.end());
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 1));
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 2));
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 3));
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 4));
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 5));
    EXPECT_FALSE(std::binary_search(vec.begin(), vec.end(), 6));
  }

  {
    Vector<int> vec = {5, 7, 4, 2, 8, 6, 1, 9, 0, 3};
    std::sort(vec.begin(), vec.end());
    EXPECT_EQ(to_s(vec), "0123456789");
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    EXPECT_EQ(to_s(vec), "9876543210");
  }

  {
    Vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::reverse(vec.begin(), vec.end());
    EXPECT_EQ(to_s(vec), "987654321");
  }

  {
    Vector<int> vec1 = {1, 2, 3, 4, 5};
    Vector<int> vec2 = {100, 200};
    std::copy(vec1.begin(), vec1.end(), std::back_inserter(vec2));
    EXPECT_EQ(to_s(vec2), "10020012345");
  }

  {
    Vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8};
    vec.erase(std::remove_if(vec.begin(), vec.end(),
                             [](auto i) { return i % 2 == 0; }),
              vec.end());
    EXPECT_EQ(to_s(vec), "1357");
  }

  {
    Vector<int> vec = {1, 2, 3, 3, 9, 10, 3, 4, 5, 8};
    auto it = std::remove(vec.begin(), vec.end(), 15);
    EXPECT_EQ(to_s(vec), "12339103458");
    vec.erase(it, vec.end());  // Nothing erased
    EXPECT_EQ(to_s(vec), "12339103458");
  }

  {
    Vector<int> vec = {1, 1, 1, 1, 1};
    auto it = std::remove(vec.begin(), vec.end(), 1);
    vec.erase(it, vec.end());
    EXPECT_TRUE(vec.empty());
  }

  {
    Vector<int> vec = {1, 2, 3, 3, 9, 10, 3, 4, 5, 8};
    auto it = std::remove(vec.begin(), vec.begin() + 5, 3);
    vec.erase(it, vec.begin() + 5);
    EXPECT_EQ(to_s(vec), "129103458");
  }

  {
    Vector<int> vec = {1, 2, 3, 3, 9, 10, 3, 4, 5, 8};
    auto it = std::remove(vec.begin(), vec.end(), 3);
    EXPECT_EQ(to_s(vec), "12910458458");
    auto d = std::distance(vec.begin(), it);
    EXPECT_EQ(d, 7);
    vec.erase(it, vec.end());
    EXPECT_EQ(to_s(vec), "12910458");
  }
}

TEST(Vector, AlgorithmsNontrivial) {
  class NontrivialInt {
   public:
    NontrivialInt(int i = -1) {
      value_ = std::make_shared<std::string>(std::to_string(i));
    }

    operator int() const { return value_ ? std::stoi(*value_) : -1; }
    bool operator==(const NontrivialInt& other) {
      return (int)(*this) == (int)other;
    }
    bool operator==(int other) { return (int)(*this) == other; }
    NontrivialInt& operator+=(int value) {
      value_ = std::make_shared<std::string>(
          std::to_string(std::stoi(*value_) + value));
      return *this;
    }

   private:
    std::shared_ptr<std::string> value_;
  };

  auto to_nt_int_array = [](size_t count,
                            int buffer[]) -> Vector<NontrivialInt> {
    Vector<NontrivialInt> result;
    for (size_t i = 0; i < count; i++) {
      result.emplace_back(buffer[i]);
    }
    return result;
  };

  auto to_s = [](const Vector<NontrivialInt>& array) -> std::string {
    std::string result;
    for (auto& i : array) {
      result += std::to_string((int)i);
    }
    return result;
  };

  {
    int buffer[] = {1, 2, 3, 4, 5};
    Vector<NontrivialInt> vec =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    std::string cat;
    std::for_each(vec.begin(), vec.end(), [&](const NontrivialInt& i) {
      cat += std::to_string((int)i);
    });
    EXPECT_TRUE(cat == "12345");

    std::for_each(vec.rbegin(), vec.rend(), [&](const NontrivialInt& i) {
      cat += std::to_string((int)i);
    });
    EXPECT_TRUE(cat == "1234554321");
  }

  {
    int buffer[] = {1, 2, 3, 4, 5, 4, 3, 2, 1};
    Vector<NontrivialInt> vec =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    std::sort(vec.begin(), vec.end());
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 1));
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 2));
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 3));
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 4));
    EXPECT_TRUE(std::binary_search(vec.begin(), vec.end(), 5));
    EXPECT_FALSE(std::binary_search(vec.begin(), vec.end(), 6));
  }

  {
    int buffer[] = {5, 7, 4, 2, 8, 6, 1, 9, 0, 3};
    Vector<NontrivialInt> vec =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    std::sort(vec.begin(), vec.end());
    EXPECT_EQ(to_s(vec), "0123456789");
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    EXPECT_EQ(to_s(vec), "9876543210");
  }

  {
    int buffer[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    Vector<NontrivialInt> vec =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    std::reverse(vec.begin(), vec.end());
    EXPECT_EQ(to_s(vec), "987654321");
  }

  {
    int buffer1[] = {1, 2, 3, 4, 5};
    int buffer2[] = {100, 200};
    Vector<NontrivialInt> vec1 =
        to_nt_int_array(sizeof(buffer1) / sizeof(buffer1[0]), buffer1);
    Vector<NontrivialInt> vec2 =
        to_nt_int_array(sizeof(buffer2) / sizeof(buffer2[0]), buffer2);
    std::copy(vec1.begin(), vec1.end(), std::back_inserter(vec2));
    EXPECT_EQ(to_s(vec2), "10020012345");
  }

  {
    int buffer[] = {1, 2, 3, 4, 5, 6, 7, 8};
    Vector<NontrivialInt> vec =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
                       [](const NontrivialInt& i) { return (int)i % 2 == 0; }),
        vec.end());
    EXPECT_EQ(to_s(vec), "1357");
  }

  {
    int buffer[] = {1, 2, 3, 3, 9, 10, 3, 4, 5, 8};
    Vector<NontrivialInt> vec =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    auto it = std::remove(vec.begin(), vec.end(), 15);
    EXPECT_EQ(to_s(vec), "12339103458");
    vec.erase(it, vec.end());  // Nothing erased
    EXPECT_EQ(to_s(vec), "12339103458");
  }

  {
    int buffer[] = {1, 1, 1, 1, 1};
    Vector<NontrivialInt> vec =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    auto it = std::remove(vec.begin(), vec.end(), 1);
    vec.erase(it, vec.end());
    EXPECT_TRUE(vec.empty());
  }

  {
    int buffer[] = {1, 2, 3, 3, 9, 10, 3, 4, 5, 8};
    Vector<NontrivialInt> vec =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    auto it = std::remove(vec.begin(), vec.begin() + 5, 3);
    vec.erase(it, vec.begin() + 5);
    EXPECT_EQ(to_s(vec), "129103458");
  }

  {
    int buffer[] = {1, 2, 3, 3, 9, 10, 3, 4, 5, 8};
    Vector<NontrivialInt> vec =
        to_nt_int_array(sizeof(buffer) / sizeof(buffer[0]), buffer);
    auto it = std::remove(vec.begin(), vec.end(), 3);
    EXPECT_EQ(to_s(vec), "12910458-1-1-1");  // moved to tail and is invalid.
    auto d = std::distance(vec.begin(), it);
    EXPECT_EQ(d, 7);
    vec.erase(it, vec.end());
    EXPECT_EQ(to_s(vec), "12910458");
  }
}

}  // namespace base
}  // namespace lynx

#pragma clang diagnostic pop
