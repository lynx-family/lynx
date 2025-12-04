// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/vector.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <numeric>
#include <random>
#include <stack>
#include <string>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"

#include "base/include/value/base_string.h"
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

using VectorTemplateless_0 = VectorTemplateless<0, false>;

TEST(Vector, InlineTypeNoFullValueInitialization) {
  // This test make sure that inlined types will not be fully
  // value initialized with zero bytes.
  {
    using Type = InlineVector<std::string, 100>;
    unsigned char buffer[sizeof(Type)];
    std::memset(buffer, 0xAA, sizeof(buffer));
    new (buffer) Type();
    EXPECT_EQ(buffer[sizeof(buffer) - 1], 0xAA);  // byte not set to zero
  }

  {
    using Type = InlineOrderedFlatMap<std::string, std::string, 100>;
    unsigned char buffer[sizeof(Type)];
    std::memset(buffer, 0xAA, sizeof(buffer));
    new (buffer) Type();
    EXPECT_EQ(buffer[sizeof(buffer) - 1], 0xAA);  // byte not set to zero
  }

  {
    using Type = InlineOrderedFlatSet<std::string, 100>;
    unsigned char buffer[sizeof(Type)];
    std::memset(buffer, 0xAA, sizeof(buffer));
    new (buffer) Type();
    EXPECT_EQ(buffer[sizeof(buffer) - 1], 0xAA);  // byte not set to zero
  }

  {
    using Type = InlineLinearFlatMap<std::string, std::string, 100>;
    unsigned char buffer[sizeof(Type)];
    std::memset(buffer, 0xAA, sizeof(buffer));
    new (buffer) Type();
    EXPECT_EQ(buffer[sizeof(buffer) - 1], 0xAA);  // byte not set to zero
  }

  {
    using Type = InlineLinearFlatSet<std::string, 100>;
    unsigned char buffer[sizeof(Type)];
    std::memset(buffer, 0xAA, sizeof(buffer));
    new (buffer) Type();
    EXPECT_EQ(buffer[sizeof(buffer) - 1], 0xAA);  // byte not set to zero
  }
}

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

  static_assert(Vector<int>::is_trivial);
  static_assert(Vector<int>::is_trivially_destructible);
  static_assert(Vector<int>::is_trivially_destructible_after_move);
  static_assert(Vector<int>::is_trivially_relocatable);
  static_assert(Vector<std::pair<int, int>>::is_trivial);
  static_assert(Vector<std::pair<int, int>>::is_trivially_destructible);
  static_assert(
      Vector<std::pair<int, int>>::is_trivially_destructible_after_move);
  static_assert(Vector<std::pair<int, int>>::is_trivially_relocatable);

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
    // Erase and emplace
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
      array.emplace(array.begin(), i);
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

    array.emplace(array.begin(), 50);
    CheckVector(array);
    array.emplace(array.end(), 51);
    CheckVector(array);
    array.emplace(array.end(), 52);
    CheckVector(array);
    array.emplace(array.begin() + 1, 49);
    CheckVector(array);
    array.emplace(array.begin(), 48);
    CheckVector(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "4850495152");

    Vector<int> array2;
    for (int i = 1; i <= 100; i++) {
      array2.emplace(array2.begin() + i - 1, i);
    }
    CheckVector(array2);
    int sum = 0;
    for (int i : array2) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);

    Vector<int> array3;
    for (int i = 1; i <= 100; i++) {
      array3.emplace(array3.begin(), i);
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

    VectorTemplateless_0::PushBackBatch(&array, sizeof(int), buffer, 5);
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

  static_assert(!Vector<NontrivialInt>::is_trivial);
  static_assert(!Vector<NontrivialInt>::is_trivially_destructible);
  static_assert(!Vector<NontrivialInt>::is_trivially_destructible_after_move);
  static_assert(!Vector<NontrivialInt>::is_trivially_relocatable);
  static_assert(!Vector<std::pair<int, NontrivialInt>>::is_trivial);
  static_assert(
      !Vector<std::pair<int, NontrivialInt>>::is_trivially_destructible);
  static_assert(
      !Vector<
          std::pair<int, NontrivialInt>>::is_trivially_destructible_after_move);
  static_assert(
      !Vector<std::pair<int, NontrivialInt>>::is_trivially_relocatable);

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
    // Erase and emplace
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
      array.emplace(array.begin(), i);
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

    array.emplace(array.begin(), 50);
    CheckVector(array);
    array.emplace(array.end(), 51);
    CheckVector(array);
    array.emplace(array.end(), 52);
    CheckVector(array);
    array.emplace(array.begin() + 1, 49);
    CheckVector(array);
    array.emplace(array.begin(), 48);
    CheckVector(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "4850495152");

    Vector<NontrivialInt> array2;
    for (int i = 1; i <= 100; i++) {
      array2.emplace(array2.begin() + i - 1, i);
    }
    CheckVector(array2);
    int sum = 0;
    for (int i : array2) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);

    Vector<NontrivialInt> array3;
    for (int i = 1; i <= 100; i++) {
      array3.emplace(array3.begin(), i);
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

TEST(Vector, NontrivialHintOfTriviallyDestructibleAfterMove) {
  class NontrivialInt {
   public:
    using TriviallyDestructibleAfterMove = bool;

    NontrivialInt(int i = -1) { value_ = new std::string(std::to_string(i)); }

    ~NontrivialInt() {
      if (value_) {
        delete value_;
      }
    }

    NontrivialInt(const NontrivialInt& other) {
      value_ = new std::string(std::to_string((int)other));
    }

    NontrivialInt(NontrivialInt&& other) : value_(other.value_) {
      other.value_ = nullptr;
    }

    NontrivialInt& operator=(const NontrivialInt& other) {
      if (this == &other) {
        return *this;
      }
      if (value_) {
        delete value_;
      }
      value_ = new std::string(std::to_string((int)other));
      return *this;
    }

    NontrivialInt& operator=(NontrivialInt&& other) {
      if (this == &other) {
        return *this;
      }
      if (value_) {
        delete value_;
      }
      value_ = other.value_;
      other.value_ = nullptr;
      return *this;
    }

    operator int() const { return value_ ? std::stoi(*value_) : -1; }
    bool operator==(const NontrivialInt& other) {
      return (int)(*this) == (int)other;
    }
    NontrivialInt& operator+=(int value) {
      int old_value_ = (int)(*this);
      if (value_) {
        delete value_;
      }
      value_ = new std::string(std::to_string(old_value_ + value));
      return *this;
    }

   private:
    std::string* value_;
  };

  static_assert(!Vector<NontrivialInt>::is_trivial);
  static_assert(!Vector<NontrivialInt>::is_trivially_destructible);
  static_assert(Vector<NontrivialInt>::is_trivially_destructible_after_move);
  static_assert(
      Vector<
          std::pair<int, NontrivialInt>>::is_trivially_destructible_after_move);
  static_assert(
      Vector<
          std::pair<NontrivialInt, int>>::is_trivially_destructible_after_move);
  static_assert(!Vector<NontrivialInt>::is_trivially_relocatable);

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
    // Erase and emplace
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
      array.emplace(array.begin(), i);
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

    array.emplace(array.begin(), 50);
    CheckVector(array);
    array.emplace(array.end(), 51);
    CheckVector(array);
    array.emplace(array.end(), 52);
    CheckVector(array);
    array.emplace(array.begin() + 1, 49);
    CheckVector(array);
    array.emplace(array.begin(), 48);
    CheckVector(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "4850495152");

    Vector<NontrivialInt> array2;
    for (int i = 1; i <= 100; i++) {
      array2.emplace(array2.begin() + i - 1, i);
    }
    CheckVector(array2);
    int sum = 0;
    for (int i : array2) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);

    Vector<NontrivialInt> array3;
    for (int i = 1; i <= 100; i++) {
      array3.emplace(array3.begin(), i);
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

TEST(Vector, NontrivialHintOfTriviallyRelocatable) {
  class NontrivialInt {
   public:
    using TriviallyRelocatable = bool;

    NontrivialInt(int i = -1) { value_ = new std::string(std::to_string(i)); }

    ~NontrivialInt() {
      if (value_) {
        delete value_;
      }
    }

    NontrivialInt(const NontrivialInt& other) {
      value_ = new std::string(std::to_string((int)other));
    }

    NontrivialInt(NontrivialInt&& other) : value_(other.value_) {
      other.value_ = nullptr;
    }

    NontrivialInt& operator=(const NontrivialInt& other) {
      if (this == &other) {
        return *this;
      }
      if (value_) {
        delete value_;
      }
      value_ = new std::string(std::to_string((int)other));
      return *this;
    }

    NontrivialInt& operator=(NontrivialInt&& other) {
      if (this == &other) {
        return *this;
      }
      if (value_) {
        delete value_;
      }
      value_ = other.value_;
      other.value_ = nullptr;
      return *this;
    }

    operator int() const { return value_ ? std::stoi(*value_) : -1; }
    bool operator==(const NontrivialInt& other) {
      return (int)(*this) == (int)other;
    }
    NontrivialInt& operator+=(int value) {
      int old_value_ = (int)(*this);
      if (value_) {
        delete value_;
      }
      value_ = new std::string(std::to_string(old_value_ + value));
      return *this;
    }

   private:
    std::string* value_;
  };

  static_assert(!Vector<NontrivialInt>::is_trivial);
  static_assert(!Vector<NontrivialInt>::is_trivially_destructible);
  static_assert(Vector<NontrivialInt>::is_trivially_destructible_after_move);
  static_assert(
      Vector<
          std::pair<int, NontrivialInt>>::is_trivially_destructible_after_move);
  static_assert(
      Vector<
          std::pair<NontrivialInt, int>>::is_trivially_destructible_after_move);
  static_assert(Vector<NontrivialInt>::is_trivially_relocatable);
  static_assert(
      Vector<std::pair<int, NontrivialInt>>::is_trivially_relocatable);
  static_assert(
      Vector<std::pair<NontrivialInt, int>>::is_trivially_relocatable);

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
    // Erase and emplace
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
      array.emplace(array.begin(), i);
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

    array.emplace(array.begin(), 50);
    CheckVector(array);
    array.emplace(array.end(), 51);
    CheckVector(array);
    array.emplace(array.end(), 52);
    CheckVector(array);
    array.emplace(array.begin() + 1, 49);
    CheckVector(array);
    array.emplace(array.begin(), 48);
    CheckVector(array);
    EXPECT_EQ(array.size(), 5);
    EXPECT_EQ(to_s(array), "4850495152");

    Vector<NontrivialInt> array2;
    for (int i = 1; i <= 100; i++) {
      array2.emplace(array2.begin() + i - 1, i);
    }
    CheckVector(array2);
    int sum = 0;
    for (int i : array2) {
      sum += i;
    }
    EXPECT_EQ(sum, 5050);

    Vector<NontrivialInt> array3;
    for (int i = 1; i <= 100; i++) {
      array3.emplace(array3.begin(), i);
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
    VectorTemplateless_0::PushBackBatch(
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

  EXPECT_TRUE(VectorTemplateless_0::Erase(&array, 4, 0, 0));
  EXPECT_EQ(array.size(), 100);

  EXPECT_TRUE(VectorTemplateless_0::Erase(&array, 4, 99, 0));
  EXPECT_EQ(array.size(), 100);
  for (int i = 0; i < 100; i++) {
    // Data not changed.
    EXPECT_EQ(array[i], i);
  }

  // DeleteCount == 0 is allowed but index 100 is out of range, so return false.
  EXPECT_FALSE(VectorTemplateless_0::Erase(&array, 4, 100, 0));
  EXPECT_EQ(array.size(), 100);

  EXPECT_TRUE(VectorTemplateless_0::Erase(&array, 4, 0, 50));

  EXPECT_EQ(array.size(), 50);
  EXPECT_EQ(array[0], 50);

  EXPECT_TRUE(VectorTemplateless_0::Erase(&array, 4, 10, 10));

  EXPECT_EQ(array.size(), 40);
  EXPECT_EQ(array[0], 50);
  EXPECT_EQ(array[10], 70);

  EXPECT_FALSE(VectorTemplateless_0::Erase(&array, 4, 10, 100));
  EXPECT_EQ(array.size(), 40);

  EXPECT_TRUE(VectorTemplateless_0::Erase(&array, 4, 0, 40));
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

TEST(Vector, ArrayEmplace) {
  Vector<std::string> vec;
  vec.emplace_back("abc", 2);
  vec.emplace_back("123", 2);
  EXPECT_EQ(*vec.emplace(vec.begin(), "xyz", 2), "xy");
  EXPECT_EQ(*vec.emplace(vec.begin() + 1, "opq", 2), "op");
  EXPECT_EQ(*vec.emplace(vec.end(), "lmn", 2), "lm");
  EXPECT_EQ(vec.size(), 5);
  EXPECT_TRUE(vec[0] == "xy");
  EXPECT_TRUE(vec[1] == "op");
  EXPECT_TRUE(vec[2] == "ab");
  EXPECT_TRUE(vec[3] == "12");
  EXPECT_TRUE(vec[4] == "lm");

  Vector<int> vec2;
  vec2.emplace_back(9);
  vec2.emplace_back(8);
  EXPECT_EQ(*vec2.emplace(vec2.begin(), 7), 7);
  EXPECT_EQ(*vec2.emplace(vec2.begin() + 1, 6), 6);
  EXPECT_EQ(*vec2.emplace(vec2.end(), 5), 5);
  EXPECT_EQ(vec2.size(), 5);
  EXPECT_TRUE(vec2[0] == 7);
  EXPECT_TRUE(vec2[1] == 6);
  EXPECT_TRUE(vec2[2] == 9);
  EXPECT_TRUE(vec2[3] == 8);
  EXPECT_TRUE(vec2[4] == 5);
}

TEST(VectorIntTest, BasicOperations) {
  Vector<int> v1;
  ASSERT_TRUE(v1.empty());

  Vector<int> v2{1, 2, 3};
  ASSERT_EQ(v2.size(), 3);
  EXPECT_EQ(v2.front(), 1);
  EXPECT_EQ(v2.back(), 3);

  Vector<int> copy(v2);
  ASSERT_EQ(copy.size(), 3);

  Vector<int> moved(std::move(copy));
  ASSERT_EQ(moved.size(), 3);
  EXPECT_TRUE(copy.empty());
}

TEST(VectorStringTest, BasicOperations) {
  Vector<std::string> sv{"Hello", "World"};
  ASSERT_EQ(sv.size(), 2);
  EXPECT_EQ(sv[0].length(), 5);

  std::string s = "Test";
  sv.push_back(std::move(s));
  EXPECT_TRUE(s.empty());
  EXPECT_EQ(sv.back(), "Test");
}

TEST(VectorIntTest, ElementAccess) {
  Vector<int> v{10, 20, 30};

  v[1] = 99;
  EXPECT_EQ(v.at(1), 99);

  int* ptr = v.data();
  EXPECT_EQ(ptr[0], 10);
}

TEST(VectorStringTest, ElementAccess) {
  Vector<std::string> sv{"A", "B", "C"};

  sv.back() += "_suffix";
  EXPECT_EQ(sv[2], "C_suffix");
}

TEST(VectorIntTest, CapacityManagement) {
  Vector<int> v;
  v.shrink_to_fit();
  EXPECT_EQ(v.capacity(), 0);

  v.reserve(100);
  ASSERT_GE(v.capacity(), 100);

  v.resize<true>(5, 42);
  ASSERT_EQ(v.size(), 5);
  EXPECT_EQ(v[3], 42);

  v.shrink_to_fit();
  EXPECT_EQ(v.capacity(), 5);
  EXPECT_EQ(v, (Vector<int>{42, 42, 42, 42, 42}));

  InlineVector<int, 5> v2{1, 2, 3};
  EXPECT_TRUE(v2.is_static_buffer());
  EXPECT_EQ(v2.size(), 3);
  EXPECT_EQ(v2.capacity(), 5);
  v2.shrink_to_fit();
  EXPECT_EQ(v2.capacity(), 5);
  EXPECT_EQ(v2, (Vector<int>{1, 2, 3}));

  v2.push_back(4);
  v2.push_back(5);
  v2.push_back(6);
  EXPECT_FALSE(v2.is_static_buffer());
  EXPECT_EQ(v2.size(), 6);
  EXPECT_EQ(v2, (Vector<int>{1, 2, 3, 4, 5, 6}));
  v2.pop_back();
  EXPECT_EQ(v2.size(), 5);
  v2.shrink_to_fit();
  EXPECT_EQ(v2.capacity(), 5);
  EXPECT_FALSE(
      v2.is_static_buffer());  // Unable to use static buffer event if after
                               // shrink_to_fit(), the buffer is fit.
  EXPECT_EQ(v2, (Vector<int>{1, 2, 3, 4, 5}));

  InlineVector<std::string, 5> v3;
  v3.shrink_to_fit();
  EXPECT_EQ(v.capacity(), 5);
  v3.emplace_back("1");
  v3.emplace_back("2");
  v3.emplace_back("3");
  v3.shrink_to_fit();
  EXPECT_EQ(v.capacity(), 5);
  EXPECT_TRUE(v3.is_static_buffer());
  v3.emplace_back("4");
  v3.emplace_back("5");
  v3.emplace_back("6");
  EXPECT_FALSE(v3.is_static_buffer());
  v3.pop_back();
  v3.shrink_to_fit();
  EXPECT_EQ(v3.capacity(), 5);
  EXPECT_EQ(v3.size(), 5);
  EXPECT_FALSE(v3.is_static_buffer());
  EXPECT_EQ(v3[0], "1");
  EXPECT_EQ(v3[1], "2");
  EXPECT_EQ(v3[2], "3");
  EXPECT_EQ(v3[3], "4");
  EXPECT_EQ(v3[4], "5");
}

TEST(VectorStringTest, CapacityManagement) {
  Vector<std::string> sv(3, "Init");
  sv.reserve(100);

  sv.emplace_back("NewString");
  EXPECT_GT(sv.capacity(), 3);
  EXPECT_EQ(sv.back().length(), 9);
}

TEST(VectorIntTest, InsertOperations) {
  Vector<int> v{1, 3};

  auto it = v.insert(v.begin() + 1, 2);
  ASSERT_EQ(v.size(), 3);
  EXPECT_EQ(*it, 2);
  EXPECT_EQ(v, (Vector<int>{1, 2, 3}));

  v.insert(v.end(), 4);
  EXPECT_EQ(v.back(), 4);
}

TEST(VectorStringTest, InsertOperations) {
  Vector<std::string> sv{"Start", "End"};

  sv.insert(sv.begin(), "Header");
  EXPECT_EQ(sv.front(), "Header");

  sv.emplace(sv.begin() + 1, 3, 'X');  // 构造 "XXX"
  EXPECT_EQ(sv[1], "XXX");
}

TEST(VectorIntTest, EdgeCases) {
  Vector<int> v;

  v.insert(v.end(), 42);
  ASSERT_EQ(v.size(), 1);

  v.reserve(2);
  v = {1, 2};  // capacity=2
  v.insert(v.begin(), 0);
  EXPECT_GT(v.capacity(), 2);
  EXPECT_EQ(v, (Vector<int>{0, 1, 2}));
}

template <class SET>
static void TestSet() {
  auto to_s = [](SET& set) -> std::string {
    std::string result;
    for (auto& i : set) {
      result += std::to_string(i);
    }
    return result;
  };

  SET set;
  set.insert(1);
  set.insert(5);
  set.insert(3);
  set.insert(7);
  set.insert(6);
  set.insert(2);
  set.insert(4);
  auto it = set.insert(8);
  EXPECT_EQ(*it.first, 8);
  EXPECT_TRUE(it.second);
  EXPECT_FALSE(set.insert(3).second);
  if (set.is_data_ordered()) {
    EXPECT_EQ(to_s(set), "12345678");
  } else {
    EXPECT_EQ(to_s(set), "15376248");
  }

  EXPECT_EQ(set.size(), 8);

  EXPECT_EQ(set.erase(5), 1);
  set.erase(1);
  EXPECT_EQ(set.size(), 6);
  EXPECT_EQ(to_s(set), set.is_data_ordered() ? "234678" : "376248");

  EXPECT_TRUE(set.contains(6));
  EXPECT_FALSE(set.contains(1));
  EXPECT_FALSE(set.contains(5));

  auto find3 = set.find(3);
  auto find1 = set.find(1);
  EXPECT_EQ(*find3, 3);
  EXPECT_TRUE(find1 == set.end());

  EXPECT_EQ(to_s(set), set.is_data_ordered() ? "234678" : "376248");
  EXPECT_EQ(*set.erase(find3), set.is_data_ordered() ? 4 : 7);
  EXPECT_EQ(to_s(set), set.is_data_ordered() ? "24678" : "76248");

  set.clear();
  EXPECT_TRUE(set.empty());

  // Check functionality after clear.
  set.insert(1);
  EXPECT_EQ(set.size(), 1);
  EXPECT_TRUE(set.contains(1));
  EXPECT_TRUE(set.find(1) != set.end());
}

TEST(Vector, OrderedFlatSet) {
  TestSet<OrderedFlatSet<int>>();
  TestSet<LinearFlatSet<int>>();
}

TEST(Vector, InlineOrderedFlatSet) {
  TestSet<InlineOrderedFlatSet<int, 20>>();
  TestSet<InlineLinearFlatSet<int, 20>>();
}

template <class MAP>
static void TestMap1() {
  auto to_s = [](MAP& map) -> std::string {
    std::string result;
    for (auto& i : map) {
      result += i.second;
    }
    return result;
  };

  MAP map;
  EXPECT_TRUE(map.empty());

  map.insert({1, "1"});
  map.insert({5, "5"});
  map.insert({3, "3"});
  map.insert({7, "7"});
  map.insert({6, "6"});
  map.insert({2, "2"});
  map.insert({4, "4"});
  auto it = map.insert({8, "8"});
  EXPECT_EQ(it.first->first, 8);
  EXPECT_EQ(it.first->second, "8");
  EXPECT_TRUE(it.second);
  EXPECT_FALSE(map.insert({3, "3"}).second);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "12345678" : "15376248");

  EXPECT_EQ(map.size(), 8);

  typename MAP::value_type pair{0, "0"};
  map.insert(pair);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "012345678" : "153762480");

  EXPECT_EQ(map.erase(5), 1);
  map.erase(1);
  map.erase(1024);
  EXPECT_EQ(map.size(), 7);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "0234678" : "3762480");

  EXPECT_TRUE(map.contains(0));
  EXPECT_TRUE(map.contains(6));
  EXPECT_FALSE(map.contains(1));
  EXPECT_FALSE(map.contains(5));

  auto find3 = map.find(3);
  auto find1 = map.find(1);
  EXPECT_TRUE(find1 == map.end());
  EXPECT_EQ(find3->first, 3);
  EXPECT_EQ(find3->second, "3");
  find3->second = "333";
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "023334678" : "333762480");

  EXPECT_EQ(map.erase(find3)->second, map.is_data_ordered() ? "4" : "7");
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "024678" : "762480");

  EXPECT_EQ(map[1], "");
  EXPECT_EQ(map.size(), 7);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "024678" : "762480");

  map[1] = "1";
  map[5] = "5";
  map[8] = "888";
  EXPECT_EQ(map.size(), 8);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "0124567888" : "7624888015");

  {
    auto result = map.insert_or_assign(5, "555");
    auto result2 = map.insert_or_assign(9, "9");
    EXPECT_EQ(result.first->first, 5);
    EXPECT_EQ(result.first->second, "555");
    EXPECT_FALSE(result.second);

    EXPECT_EQ(result2.first->first, 9);
    EXPECT_EQ(result2.first->second, "9");
    EXPECT_TRUE(result2.second);
  }
  EXPECT_EQ(map.size(), 9);
  EXPECT_EQ(to_s(map),
            map.is_data_ordered() ? "0124555678889" : "7624888015559");

  {
    auto er = map.emplace(1, "1");
    EXPECT_EQ(er.first->first, 1);
    EXPECT_EQ(er.first->second, "1");
    EXPECT_FALSE(er.second);
    EXPECT_EQ(map.size(), 9);
    EXPECT_EQ(to_s(map),
              map.is_data_ordered() ? "0124555678889" : "7624888015559");
  }

  {
    auto er = map.emplace(10, "abcdef", 3);
    EXPECT_EQ(er.first->first, 10);
    EXPECT_EQ(er.first->second, "abc");
    EXPECT_TRUE(er.second);
    EXPECT_EQ(map.size(), 10);
    EXPECT_EQ(to_s(map),
              map.is_data_ordered() ? "0124555678889abc" : "7624888015559abc");
  }

  map.clear();
  EXPECT_TRUE(map.empty());

  // Check functionality after clear.
  map.insert({1, "1"});
  EXPECT_EQ(map.size(), 1);
  EXPECT_TRUE(map.contains(1));
  EXPECT_TRUE(map.find(1) != map.end());
}

template <class MAP>
static void TestMap2() {
  auto to_s = [](MAP& map) -> std::string {
    std::string result;
    for (auto& i : map) {
      result += std::to_string(i.second);
    }
    return result;
  };

  MAP map;
  EXPECT_TRUE(map.empty());

  map.insert({"1", 1});
  map.insert({"5", 5});
  map.insert({"3", 3});
  map.insert({"7", 7});
  map.insert({"6", 6});
  map.insert({"2", 2});
  map.insert({"4", 4});
  auto it = map.insert({"8", 8});
  EXPECT_EQ(it.first->first, "8");
  EXPECT_EQ(it.first->second, 8);
  EXPECT_TRUE(it.second);
  EXPECT_FALSE(map.insert({"3", 3}).second);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "12345678" : "15376248");

  EXPECT_EQ(map.size(), 8);

  typename MAP::value_type pair{"0", 0};
  map.insert(pair);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "012345678" : "153762480");

  EXPECT_EQ(map.erase("5"), 1);
  map.erase("1");
  map.erase("abc");
  EXPECT_EQ(map.size(), 7);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "0234678" : "3762480");

  EXPECT_TRUE(map.contains("0"));
  EXPECT_TRUE(map.contains("6"));
  EXPECT_FALSE(map.contains("1"));
  EXPECT_FALSE(map.contains("5"));

  auto find3 = map.find("3");
  auto find1 = map.find("1");
  EXPECT_TRUE(find1 == map.end());
  EXPECT_EQ(find3->first, "3");
  EXPECT_EQ(find3->second, 3);
  find3->second = 333;
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "023334678" : "333762480");

  EXPECT_EQ(map.erase(find3)->second, map.is_data_ordered() ? 4 : 7);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "024678" : "762480");

  EXPECT_EQ(map["1"], 0);  // key inserted
  EXPECT_EQ(map.size(), 7);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "0024678" : "7624800");

  map["1"] = 1;
  map["5"] = 5;
  map["8"] = 888;
  EXPECT_EQ(map.size(), 8);
  EXPECT_EQ(to_s(map), map.is_data_ordered() ? "0124567888" : "7624888015");

  std::string key = "a";
  map[std::move(key)] = 999;
  EXPECT_EQ(to_s(map),
            map.is_data_ordered() ? "0124567888999" : "7624888015999");
  EXPECT_TRUE(key.empty());

  map.clear();
  EXPECT_TRUE(map.empty());

  // Check functionality after clear.
  map.insert({"1", 1});
  EXPECT_EQ(map.size(), 1);
  EXPECT_TRUE(map.contains("1"));
  EXPECT_TRUE(map.find("1") != map.end());
}

TEST(Vector, OrderedFlatMap) {
  TestMap1<OrderedFlatMap<int, std::string>>();
  TestMap1<LinearFlatMap<int, std::string>>();
  TestMap2<OrderedFlatMap<std::string, int>>();
  TestMap2<LinearFlatMap<std::string, int>>();
}

TEST(Vector, InlineOrderedFlatMap) {
  TestMap1<InlineOrderedFlatMap<int, std::string, 20>>();
  TestMap1<InlineLinearFlatMap<int, std::string, 20>>();
  TestMap2<InlineOrderedFlatMap<std::string, int, 20>>();
  TestMap2<InlineLinearFlatMap<std::string, int, 20>>();
}

TEST(Vector, MapInsertOrAssign) {
  InlineOrderedFlatMap<std::string, std::string, 5> map{
      {"3", "c"}, {"2", "b"}, {"1", "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.capacity(), 5);
  EXPECT_EQ(map["1"], "a");
  EXPECT_EQ(map["2"], "b");
  EXPECT_EQ(map["3"], "c");
  EXPECT_EQ(map["4"], "");

  auto r = map.insert_or_assign("4", "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map["4"], "d");

  std::string s5 = "5";
  std::string se = "e";
  auto r2 = map.insert_or_assign(s5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map["5"], "e");
  EXPECT_EQ(s5, "5");
  EXPECT_TRUE(se.empty());

  std::string s6 = "6";
  std::string sf = "f";
  auto r3 = map.insert_or_assign(std::move(s6), std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map["6"], "f");
  EXPECT_TRUE(s6.empty());
  EXPECT_TRUE(sf.empty());

  std::string s7 = "7";
  std::string sg = "g";
  auto r4 = map.insert_or_assign(std::move(s7), sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map["7"], "g");
  EXPECT_TRUE(s7.empty());
  EXPECT_EQ(sg, "g");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, MapEmplaceOrAssign) {
  InlineOrderedFlatMap<std::string, std::string, 5> map{
      {"3", "c"}, {"2", "b"}, {"1", "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map["1"], "a");
  EXPECT_EQ(map["2"], "b");
  EXPECT_EQ(map["3"], "c");
  EXPECT_EQ(map["4"], "");

  auto r = map.emplace_or_assign("4", "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map["4"], "d");

  std::string s5 = "5";
  std::string se = "e";
  auto r2 = map.emplace_or_assign(s5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map["5"], "e");
  EXPECT_EQ(s5, "5");
  EXPECT_TRUE(se.empty());

  std::string s6 = "6";
  std::string sf = "f";
  auto r3 = map.emplace_or_assign(std::move(s6), std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map["6"], "f");
  EXPECT_TRUE(s6.empty());
  EXPECT_TRUE(sf.empty());

  std::string s7 = "7";
  std::string sg = "g";
  auto r4 = map.emplace_or_assign(std::move(s7), sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map["7"], "g");
  EXPECT_TRUE(s7.empty());
  EXPECT_EQ(sg, "g");

  auto r5 = map.emplace_or_assign("7", 5, 'g');
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(map["7"], "ggggg");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, SetInsert) {
  OrderedFlatSet<std::string> set{"1", "2", "3"};
  auto r = set.insert("3");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(set.size(), 3);

  auto r2 = set.insert("4");
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(set.size(), 4);

  std::string s5 = "5";
  auto r3 = set.insert(std::move(s5));
  EXPECT_TRUE(r3.second);
  EXPECT_TRUE(s5.empty());

  EXPECT_EQ(set.size(), 5);
  EXPECT_TRUE(set.contains("1"));
  EXPECT_TRUE(set.contains("2"));
  EXPECT_TRUE(set.contains("3"));
  EXPECT_TRUE(set.contains("4"));
  EXPECT_TRUE(set.contains("5"));
  EXPECT_FALSE(set.contains("6"));
}

TEST(Vector, MapEmplace) {
  OrderedFlatMap<std::string, std::string> map;
  auto r = map.emplace(std::piecewise_construct,
                       std::tuple<const char*, size_t>("123", 2),
                       std::tuple<const char*, size_t>("abc", 2));
  EXPECT_TRUE(r.second);
  EXPECT_EQ(r.first->first, "12");
  EXPECT_EQ(r.first->second, "ab");
  auto r2 = map.emplace(std::piecewise_construct,
                        std::tuple<const char*, size_t>("112", 2),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(r2.first->first, "11");
  EXPECT_EQ(r2.first->second, "xy");

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");

  auto r3 = map.emplace(std::piecewise_construct, std::forward_as_tuple("12"),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, "12");
  EXPECT_EQ(r3.first->second, "ab");

  EXPECT_EQ(map.size(), 2);

  auto r4 = map.try_emplace("11", "ab");
  EXPECT_FALSE(r4.second);
  EXPECT_EQ(r4.first->first, "11");
  EXPECT_EQ(r4.first->second, "xy");

  std::string s11 = "11";
  std::string sXYZ = "xyz";
  auto r5 = map.try_emplace(std::move(s11), std::move(sXYZ));
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(r5.first->first, "11");
  EXPECT_EQ(r5.first->second, "xy");
  EXPECT_EQ(s11, "11");
  EXPECT_EQ(sXYZ, "xyz");

  std::string s13 = "13";
  auto r6 = map.try_emplace(std::move(s13), std::move(sXYZ));
  EXPECT_TRUE(r6.second);
  EXPECT_EQ(r6.first->first, "13");
  EXPECT_EQ(r6.first->second, "xyz");
  EXPECT_TRUE(s13.empty());
  EXPECT_TRUE(sXYZ.empty());

  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");

  std::string s14 = "14";
  std::string sUVW = "uvw";
  auto r7 = map.try_emplace(s14, sUVW);
  EXPECT_TRUE(r7.second);
  EXPECT_EQ(r7.first->first, "14");
  EXPECT_EQ(r7.first->second, "uvw");
  EXPECT_EQ(s14, "14");
  EXPECT_EQ(sUVW, "uvw");

  EXPECT_EQ(map.size(), 4);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");
  EXPECT_EQ(map["14"], "uvw");
}

TEST(MapStringTest, BasicOperations) {
  OrderedFlatMap<std::string, std::string> m;

  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.size(), 0);

  auto ret = m.insert({"apple", "red"});
  ASSERT_TRUE(ret.second);
  EXPECT_EQ(ret.first->first, "apple");
  EXPECT_EQ(ret.first->second, "red");
  EXPECT_EQ(m.size(), 1);
}

TEST(MapStringTest, ElementAccess) {
  OrderedFlatMap<std::string, std::string> m{{"apple", "red"},
                                             {"banana", "yellow"}};

  EXPECT_EQ(m["apple"], "red");

  m["apple"] = "green";
  EXPECT_EQ(m["apple"], "green");
  EXPECT_EQ(m.at("apple"), "green");

  EXPECT_EQ(m["grape"], "");
  EXPECT_EQ(m.at("grape"), "");
  EXPECT_EQ(m.size(), 3);

  EXPECT_EQ(m.at("orange"), "");
  m.at("orange") = "orange";
  EXPECT_EQ(m["orange"], "orange");
  EXPECT_EQ(m.size(), 4);

  std::string melon = "melon";
  m.at(std::move(melon)) = "green";
  EXPECT_EQ(melon, "");
  EXPECT_EQ(m.find("melon")->second, "green");
  EXPECT_EQ(m.size(), 5);

  std::string pear = "pear";
  auto r = m.insert_default_if_absent(std::move(pear));
  EXPECT_TRUE(r.second);
  r.first->second = "yellow";
  EXPECT_EQ(pear, "");
  EXPECT_EQ(m["pear"], "yellow");
  EXPECT_EQ(m.size(), 6);

  auto r2 = m.insert_default_if_absent("apple");
  EXPECT_FALSE(r2.second);
  EXPECT_EQ(m["apple"], "green");
  r2.first->second = "red";
  EXPECT_EQ(m["apple"], "red");
  EXPECT_EQ(m.size(), 6);
}

TEST(MapStringTest, InsertUpdate) {
  OrderedFlatMap<std::string, std::string> m;

  auto ret1 = m.insert({"fruit", "apple"});
  EXPECT_TRUE(ret1.second);
  auto ret2 = m.insert({"fruit", "banana"});
  EXPECT_FALSE(ret2.second);
  EXPECT_EQ(ret2.first->second, "apple");

  auto emp_ret = m.emplace("color", "blue");
  EXPECT_TRUE(emp_ret.second);
  EXPECT_EQ(emp_ret.first->first, "color");

  m["color"] = "red";
  EXPECT_EQ(m["color"], "red");
}

TEST(MapStringTest, EraseOperations) {
  OrderedFlatMap<std::string, std::string> m{
      {"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_EQ(m.size(), 3);
  EXPECT_EQ(m["A"], "1");
  EXPECT_EQ(m["B"], "2");
  EXPECT_EQ(m["C"], "3");

  size_t cnt = m.erase("B");
  EXPECT_EQ(cnt, 1);
  EXPECT_EQ(m.size(), 2);
  EXPECT_FALSE(m.contains("B"));

  auto it = m.find("A");
  m.erase(it);
  EXPECT_EQ(m.size(), 1);

  EXPECT_EQ(m.erase("X"), 0);
}

TEST(SetStringTest, Iterators) {
  InlineOrderedFlatSet<std::string, 10> s{"a", "z", "c", "b",
                                          "m", "g", "q", "h"};
  std::string order;
  for (auto it = s.begin(); it != s.end(); it++) {
    order += *it;
  }
  EXPECT_EQ(order, "abcghmqz");

  order = "";
  for (auto it = s.rbegin(); it != s.rend(); it++) {
    order += *it;
  }
  EXPECT_EQ(order, "zqmhgcba");
}

TEST(SetStringTest, FrontBack) {
  {
    InlineOrderedFlatSet<std::string, 10> s{"a", "z", "c", "b",
                                            "m", "g", "q", "h"};
    EXPECT_EQ(s.front(), "a");
    EXPECT_EQ(s.back(), "z");
    s.erase("a");
    s.erase("z");
    s.erase("g");
    EXPECT_EQ(s.front(), "b");
    EXPECT_EQ(s.back(), "q");
  }
  {
    const InlineOrderedFlatSet<std::string, 10> s{"a", "z", "c", "b",
                                                  "m", "g", "q", "h"};
    EXPECT_EQ(s.front(), "a");
    EXPECT_EQ(s.back(), "z");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(s)>>&>(s))
        .erase("a");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(s)>>&>(s))
        .erase("z");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(s)>>&>(s))
        .erase("g");
    EXPECT_EQ(s.front(), "b");
    EXPECT_EQ(s.back(), "q");
  }
}

TEST(SetStringTest, Basic) {
  InlineOrderedFlatSet<std::string, 10> s{"a", "z", "c", "b",
                                          "m", "g", "q", "h"};
  EXPECT_TRUE(s.is_static_buffer());
  EXPECT_TRUE(s.contains("a"));
  EXPECT_FALSE(s.contains("y"));
  EXPECT_TRUE(s.find("c") != s.end());
  EXPECT_TRUE(s.find("y") == s.end());
  EXPECT_TRUE(s.count("q") == 1);
  EXPECT_TRUE(s.count("y") == 0);
  EXPECT_EQ(s.erase("y"), 0);
  EXPECT_EQ(s.size(), 8);
  EXPECT_EQ(s.erase("z"), 1);
  EXPECT_EQ(s.size(), 7);
  EXPECT_FALSE(s.contains("z"));
  auto it = s.erase(s.find("g"));
  EXPECT_EQ(s.size(), 6);
  EXPECT_EQ(*it, "h");
}

TEST(MapStringTest, Iterators) {
  OrderedFlatMap<std::string, std::string> m{
      {"Z", "26"}, {"A", "1"}, {"M", "13"}};

  auto it = m.begin();
  EXPECT_EQ(it->first, "A");
  ++it;
  EXPECT_EQ(it->first, "M");
  ++it;
  EXPECT_EQ(it->first, "Z");
  ++it;
  EXPECT_EQ(it, m.end());

  auto rit = m.rbegin();
  EXPECT_EQ(rit->first, "Z");
  ++rit;
  EXPECT_EQ(rit->first, "M");
  ++rit;
  EXPECT_EQ(rit->first, "A");
  ++rit;
  EXPECT_EQ(rit, m.rend());
}

TEST(MapStringTest, FrontBack) {
  {
    OrderedFlatMap<std::string, std::string> m{
        {"Z", "26"}, {"A", "1"}, {"M", "13"}};
    EXPECT_EQ(m.front().first, "A");
    EXPECT_EQ(m.front().second, "1");
    EXPECT_EQ(m.back().first, "Z");
    EXPECT_EQ(m.back().second, "26");
    m.erase("A");
    m.erase("Z");
    EXPECT_EQ(m.front().first, "M");
    EXPECT_EQ(m.front().second, "13");
    EXPECT_EQ(m.back().first, "M");
    EXPECT_EQ(m.back().second, "13");

    m.front().second = "This is M";
    EXPECT_EQ(m["M"], "This is M");
    m.back().second = "This is M, too";
    EXPECT_EQ(m["M"], "This is M, too");
  }

  {
    const OrderedFlatMap<std::string, std::string> m{
        {"Z", "26"}, {"A", "1"}, {"M", "13"}};
    EXPECT_EQ(m.front().first, "A");
    EXPECT_EQ(m.front().second, "1");
    EXPECT_EQ(m.back().first, "Z");
    EXPECT_EQ(m.back().second, "26");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(m)>>&>(m))
        .erase("A");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(m)>>&>(m))
        .erase("Z");
    EXPECT_EQ(m.front().first, "M");
    EXPECT_EQ(m.front().second, "13");
    EXPECT_EQ(m.back().first, "M");
    EXPECT_EQ(m.back().second, "13");
  }
}

TEST(MapStringTest, EdgeCases) {
  OrderedFlatMap<std::string, std::string> m;

  m[""] = "empty_key";
  m.emplace("empty_value", "");
  EXPECT_EQ(m[""], "empty_key");
  EXPECT_EQ(m["empty_value"], "");

  std::string big_key(1000, 'K');
  std::string big_value(10000, 'V');
  m[big_key] = big_value;
  EXPECT_EQ(m[big_key].size(), 10000);
}

TEST(MapStringTest, InsertOrAssign) {
  OrderedFlatMap<std::string, std::string> m;

  {
    auto [it, inserted] = m.insert_or_assign("fruit", "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.insert_or_assign("fruit", "banana");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "banana");
    EXPECT_EQ(m.size(), 1);
  }

  m.insert_or_assign("empty", "");
  EXPECT_EQ(m["empty"], "");

  auto [it, _] = m.insert_or_assign("new_key", "value");
  EXPECT_EQ(it->first, "new_key");
}

TEST(MapStringTest, EmplaceOrAssign) {
  OrderedFlatMap<std::string, std::string> m;

  {
    auto [it, inserted] = m.emplace_or_assign("fruit", "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.emplace_or_assign("fruit", "banana", 4);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "bana");
    EXPECT_EQ(m["fruit"], "bana");
    EXPECT_EQ(m.size(), 1);
  }

  m.emplace_or_assign("empty", "");
  EXPECT_EQ(m["empty"], "");

  auto [it, _] = m.emplace_or_assign("new_key", "value");
  EXPECT_EQ(it->first, "new_key");
}

TEST(MapStringTest, EmplacePiecewise) {
  OrderedFlatMap<std::string, std::string> m;

  auto emp_it =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple(5, 'X'));
  ASSERT_TRUE(emp_it.second);
  EXPECT_EQ(emp_it.first->second, "XXXXX");

  m.emplace(std::piecewise_construct, std::forward_as_tuple(3, 'K'),
            std::forward_as_tuple(3, 'k'));
  EXPECT_EQ(m["KKK"], "kkk");

  auto emp_fail =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple("new_value"));
  EXPECT_FALSE(emp_fail.second);
  EXPECT_EQ(m["piece_key"], "XXXXX");
}

TEST(Vector, LinearMapInsertOrAssign) {
  InlineLinearFlatMap<std::string, std::string, 5> map{
      {"3", "c"}, {"2", "b"}, {"1", "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.capacity(), 5);
  EXPECT_EQ(map["1"], "a");
  EXPECT_EQ(map["2"], "b");
  EXPECT_EQ(map["3"], "c");
  EXPECT_EQ(map["4"], "");

  auto r = map.insert_or_assign("4", "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map["4"], "d");

  std::string s5 = "5";
  std::string se = "e";
  auto r2 = map.insert_or_assign(s5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map["5"], "e");
  EXPECT_EQ(s5, "5");
  EXPECT_TRUE(se.empty());

  std::string s6 = "6";
  std::string sf = "f";
  auto r3 = map.insert_or_assign(std::move(s6), std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map["6"], "f");
  EXPECT_TRUE(s6.empty());
  EXPECT_TRUE(sf.empty());

  std::string s7 = "7";
  std::string sg = "g";
  auto r4 = map.insert_or_assign(std::move(s7), sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map["7"], "g");
  EXPECT_TRUE(s7.empty());
  EXPECT_EQ(sg, "g");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearMapEmplaceOrAssign) {
  InlineLinearFlatMap<std::string, std::string, 5> map{
      {"3", "c"}, {"2", "b"}, {"1", "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map["1"], "a");
  EXPECT_EQ(map["2"], "b");
  EXPECT_EQ(map["3"], "c");
  EXPECT_EQ(map["4"], "");

  auto r = map.emplace_or_assign("4", "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map["4"], "d");

  std::string s5 = "5";
  std::string se = "e";
  auto r2 = map.emplace_or_assign(s5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map["5"], "e");
  EXPECT_EQ(s5, "5");
  EXPECT_TRUE(se.empty());

  std::string s6 = "6";
  std::string sf = "f";
  auto r3 = map.emplace_or_assign(std::move(s6), std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map["6"], "f");
  EXPECT_TRUE(s6.empty());
  EXPECT_TRUE(sf.empty());

  std::string s7 = "7";
  std::string sg = "g";
  auto r4 = map.emplace_or_assign(std::move(s7), sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map["7"], "g");
  EXPECT_TRUE(s7.empty());
  EXPECT_EQ(sg, "g");

  auto r5 = map.emplace_or_assign("7", 5, 'g');
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(map["7"], "ggggg");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearSetInsert) {
  LinearFlatSet<std::string> set{"1", "2", "3"};
  auto r = set.insert("3");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(set.size(), 3);

  auto r2 = set.insert("4");
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(set.size(), 4);

  std::string s5 = "5";
  auto r3 = set.insert(std::move(s5));
  EXPECT_TRUE(r3.second);
  EXPECT_TRUE(s5.empty());

  EXPECT_EQ(set.size(), 5);
  EXPECT_TRUE(set.contains("1"));
  EXPECT_TRUE(set.contains("2"));
  EXPECT_TRUE(set.contains("3"));
  EXPECT_TRUE(set.contains("4"));
  EXPECT_TRUE(set.contains("5"));
  EXPECT_FALSE(set.contains("6"));
}

TEST(Vector, LinearMapEmplace) {
  LinearFlatMap<std::string, std::string> map;
  auto r = map.emplace(std::piecewise_construct,
                       std::tuple<const char*, size_t>("123", 2),
                       std::tuple<const char*, size_t>("abc", 2));
  EXPECT_TRUE(r.second);
  EXPECT_EQ(r.first->first, "12");
  EXPECT_EQ(r.first->second, "ab");
  auto r2 = map.emplace(std::piecewise_construct,
                        std::tuple<const char*, size_t>("112", 2),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(r2.first->first, "11");
  EXPECT_EQ(r2.first->second, "xy");

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");

  auto r3 = map.emplace(std::piecewise_construct, std::forward_as_tuple("12"),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, "12");
  EXPECT_EQ(r3.first->second, "ab");

  EXPECT_EQ(map.size(), 2);

  auto r4 = map.try_emplace("11", "ab");
  EXPECT_FALSE(r4.second);
  EXPECT_EQ(r4.first->first, "11");
  EXPECT_EQ(r4.first->second, "xy");

  std::string s11 = "11";
  std::string sXYZ = "xyz";
  auto r5 = map.try_emplace(std::move(s11), std::move(sXYZ));
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(r5.first->first, "11");
  EXPECT_EQ(r5.first->second, "xy");
  EXPECT_EQ(s11, "11");
  EXPECT_EQ(sXYZ, "xyz");

  std::string s13 = "13";
  auto r6 = map.try_emplace(std::move(s13), std::move(sXYZ));
  EXPECT_TRUE(r6.second);
  EXPECT_EQ(r6.first->first, "13");
  EXPECT_EQ(r6.first->second, "xyz");
  EXPECT_TRUE(s13.empty());
  EXPECT_TRUE(sXYZ.empty());

  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");

  std::string s14 = "14";
  std::string sUVW = "uvw";
  auto r7 = map.try_emplace(s14, sUVW);
  EXPECT_TRUE(r7.second);
  EXPECT_EQ(r7.first->first, "14");
  EXPECT_EQ(r7.first->second, "uvw");
  EXPECT_EQ(s14, "14");
  EXPECT_EQ(sUVW, "uvw");

  EXPECT_EQ(map.size(), 4);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");
  EXPECT_EQ(map["14"], "uvw");
}

TEST(MapStringTest, LinearBasicOperations) {
  LinearFlatMap<std::string, std::string> m;

  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.size(), 0);

  auto ret = m.insert({"apple", "red"});
  ASSERT_TRUE(ret.second);
  EXPECT_EQ(ret.first->first, "apple");
  EXPECT_EQ(ret.first->second, "red");
  EXPECT_EQ(m.size(), 1);
}

TEST(MapStringTest, LinearElementAccess) {
  LinearFlatMap<std::string, std::string> m{{"apple", "red"},
                                            {"banana", "yellow"}};

  EXPECT_EQ(m["apple"], "red");

  m["apple"] = "green";
  EXPECT_EQ(m["apple"], "green");
  EXPECT_EQ(m.at("apple"), "green");

  EXPECT_EQ(m["grape"], "");
  EXPECT_EQ(m.at("grape"), "");
  EXPECT_EQ(m.size(), 3);

  EXPECT_EQ(m.at("orange"), "");
  m.at("orange") = "orange";
  EXPECT_EQ(m["orange"], "orange");
  EXPECT_EQ(m.size(), 4);

  std::string melon = "melon";
  m.at(std::move(melon)) = "green";
  EXPECT_EQ(melon, "");
  EXPECT_EQ(m.find("melon")->second, "green");
  EXPECT_EQ(m.size(), 5);

  std::string pear = "pear";
  auto r = m.insert_default_if_absent(std::move(pear));
  EXPECT_TRUE(r.second);
  r.first->second = "yellow";
  EXPECT_EQ(pear, "");
  EXPECT_EQ(m["pear"], "yellow");
  EXPECT_EQ(m.size(), 6);

  auto r2 = m.insert_default_if_absent("apple");
  EXPECT_FALSE(r2.second);
  EXPECT_EQ(m["apple"], "green");
  r2.first->second = "red";
  EXPECT_EQ(m["apple"], "red");
  EXPECT_EQ(m.size(), 6);

  std::string black = "black";
  auto r3 = m.insert_if_absent("apple", black);
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, "apple");
  EXPECT_EQ(r3.first->second, "red");
  EXPECT_EQ(m.size(), 6);

  std::string peach = "peach";
  std::string pink = "pink";
  auto r4 = m.insert_if_absent(std::move(peach), std::move(pink));
  EXPECT_TRUE(r4.second);
  EXPECT_TRUE(peach.empty());
  EXPECT_TRUE(pink.empty());
  EXPECT_EQ(r4.first->first, "peach");
  EXPECT_EQ(r4.first->second, "pink");
  EXPECT_EQ(m.size(), 7);

  auto r5 = m.insert_if_absent("tomato", black);
  EXPECT_TRUE(r5.second);
  EXPECT_EQ(r5.first->first, "tomato");
  EXPECT_EQ(r5.first->second, "black");
  EXPECT_EQ(black, "black");
  EXPECT_EQ(m.size(), 8);
}

TEST(MapStringTest, LinearInsertUpdate) {
  LinearFlatMap<std::string, std::string> m;

  auto ret1 = m.insert({"fruit", "apple"});
  EXPECT_TRUE(ret1.second);
  auto ret2 = m.insert({"fruit", "banana"});
  EXPECT_FALSE(ret2.second);
  EXPECT_EQ(ret2.first->second, "apple");

  auto emp_ret = m.emplace("color", "blue");
  EXPECT_TRUE(emp_ret.second);
  EXPECT_EQ(emp_ret.first->first, "color");

  m["color"] = "red";
  EXPECT_EQ(m["color"], "red");
}

TEST(MapStringTest, LinearEraseOperations) {
  LinearFlatMap<std::string, std::string> m{{"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_EQ(m.size(), 3);
  EXPECT_EQ(m["A"], "1");
  EXPECT_EQ(m["B"], "2");
  EXPECT_EQ(m["C"], "3");

  size_t cnt = m.erase("B");
  EXPECT_EQ(cnt, 1);
  EXPECT_EQ(m.size(), 2);
  EXPECT_FALSE(m.contains("B"));

  auto it = m.find("A");
  m.erase(it);
  EXPECT_EQ(m.size(), 1);

  EXPECT_EQ(m.erase("X"), 0);
}

TEST(SetStringTest, LinearIterators) {
  InlineLinearFlatSet<std::string, 10> s{"a", "z", "c", "b",
                                         "m", "g", "q", "h"};
  std::string order;
  for (auto it = s.begin(); it != s.end(); it++) {
    order += *it;
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "abcghmqz" : "azcbmgqh");

  order = "";
  for (auto it = s.rbegin(); it != s.rend(); it++) {
    order += *it;
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "zqmhgcba" : "hqgmbcza");
}

TEST(SetStringTest, LinearFrontBack) {
  {
    InlineLinearFlatSet<std::string, 10> s{"a", "z", "c", "b",
                                           "m", "g", "q", "h"};
    EXPECT_EQ(s.front(), "a");
    EXPECT_EQ(s.back(), "h");
    s.erase("a");
    s.erase("h");
    s.erase("g");
    EXPECT_EQ(s.front(), "z");
    EXPECT_EQ(s.back(), "q");
  }
  {
    InlineLinearFlatSet<std::string, 10> s{"a", "z", "c", "b",
                                           "m", "g", "q", "h"};
    EXPECT_EQ(s.front(), "a");
    EXPECT_EQ(s.back(), "h");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(s)>>&>(s))
        .erase("a");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(s)>>&>(s))
        .erase("h");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(s)>>&>(s))
        .erase("g");
    EXPECT_EQ(s.front(), "z");
    EXPECT_EQ(s.back(), "q");
  }
}

TEST(SetStringTest, LinearBasic) {
  InlineLinearFlatSet<std::string, 10> s{"a", "z", "c", "b",
                                         "m", "g", "q", "h"};
  EXPECT_TRUE(s.is_static_buffer());
  EXPECT_TRUE(s.contains("a"));
  EXPECT_FALSE(s.contains("y"));
  EXPECT_TRUE(s.find("c") != s.end());
  EXPECT_TRUE(s.find("y") == s.end());
  EXPECT_TRUE(s.count("q") == 1);
  EXPECT_TRUE(s.count("y") == 0);
  EXPECT_EQ(s.erase("y"), 0);
  EXPECT_EQ(s.size(), 8);
  EXPECT_EQ(s.erase("z"), 1);
  EXPECT_EQ(s.size(), 7);
  EXPECT_FALSE(s.contains("z"));
  auto it = s.erase(s.find("g"));
  EXPECT_EQ(s.size(), 6);
  EXPECT_EQ(*it, s.is_data_ordered() ? "h" : "q");
}

TEST(MapStringTest, LinearIterators) {
  LinearFlatMap<std::string, std::string> m{
      {"Z", "26"}, {"A", "1"}, {"M", "13"}};

  auto it = m.begin();
  EXPECT_EQ(it->first, m.is_data_ordered() ? "A" : "Z");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? "M" : "A");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? "Z" : "M");
  ++it;
  EXPECT_EQ(it, m.end());

  auto rit = m.rbegin();
  EXPECT_EQ(rit->first, m.is_data_ordered() ? "Z" : "M");
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? "M" : "A");
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? "A" : "Z");
  ++rit;
  EXPECT_EQ(rit, m.rend());
}

TEST(MapStringTest, LinearFrontBack) {
  {
    LinearFlatMap<std::string, std::string> m{
        {"Z", "26"}, {"A", "1"}, {"M", "13"}};
    EXPECT_EQ(m.front().first, "Z");
    EXPECT_EQ(m.front().second, "26");
    EXPECT_EQ(m.back().first, "M");
    EXPECT_EQ(m.back().second, "13");
    m.erase("Z");
    m.erase("M");
    EXPECT_EQ(m.front().first, "A");
    EXPECT_EQ(m.front().second, "1");
    EXPECT_EQ(m.back().first, "A");
    EXPECT_EQ(m.back().second, "1");

    m.front().second = "This is A";
    EXPECT_EQ(m["A"], "This is A");
    m.back().second = "This is A, too";
    EXPECT_EQ(m["A"], "This is A, too");
  }

  {
    const LinearFlatMap<std::string, std::string> m{
        {"Z", "26"}, {"A", "1"}, {"M", "13"}};
    EXPECT_EQ(m.front().first, "Z");
    EXPECT_EQ(m.front().second, "26");
    EXPECT_EQ(m.back().first, "M");
    EXPECT_EQ(m.back().second, "13");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(m)>>&>(m))
        .erase("Z");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(m)>>&>(m))
        .erase("M");
    EXPECT_EQ(m.front().first, "A");
    EXPECT_EQ(m.front().second, "1");
    EXPECT_EQ(m.back().first, "A");
    EXPECT_EQ(m.back().second, "1");
  }
}

TEST(MapStringTest, LinearEdgeCases) {
  LinearFlatMap<std::string, std::string> m;

  m[""] = "empty_key";
  m.emplace("empty_value", "");
  EXPECT_EQ(m[""], "empty_key");
  EXPECT_EQ(m["empty_value"], "");

  std::string big_key(1000, 'K');
  std::string big_value(10000, 'V');
  m[big_key] = big_value;
  EXPECT_EQ(m[big_key].size(), 10000);
}

TEST(MapStringTest, LinearInsertOrAssign) {
  LinearFlatMap<std::string, std::string> m;

  {
    auto [it, inserted] = m.insert_or_assign("fruit", "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.insert_or_assign("fruit", "banana");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "banana");
    EXPECT_EQ(m.size(), 1);
  }

  m.insert_or_assign("empty", "");
  EXPECT_EQ(m["empty"], "");

  auto [it, _] = m.insert_or_assign("new_key", "value");
  EXPECT_EQ(it->first, "new_key");
}

TEST(MapStringTest, LinearEmplaceOrAssign) {
  LinearFlatMap<std::string, std::string> m;

  {
    auto [it, inserted] = m.emplace_or_assign("fruit", "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.emplace_or_assign("fruit", "banana", 4);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "bana");
    EXPECT_EQ(m["fruit"], "bana");
    EXPECT_EQ(m.size(), 1);
  }

  m.insert_or_assign("empty", "");
  EXPECT_EQ(m["empty"], "");

  auto [it, _] = m.emplace_or_assign("new_key", "value");
  EXPECT_EQ(it->first, "new_key");
}

TEST(MapStringTest, LinearEmplacePiecewise) {
  LinearFlatMap<std::string, std::string> m;

  auto emp_it =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple(5, 'X'));
  ASSERT_TRUE(emp_it.second);
  EXPECT_EQ(emp_it.first->second, "XXXXX");

  m.emplace(std::piecewise_construct, std::forward_as_tuple(3, 'K'),
            std::forward_as_tuple(3, 'k'));
  EXPECT_EQ(m["KKK"], "kkk");

  auto emp_fail =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple("new_value"));
  EXPECT_FALSE(emp_fail.second);
  EXPECT_EQ(m["piece_key"], "XXXXX");
}

namespace {
template <class MapType>
bool AssertMapContent_ABC_123(MapType& m) {
  return (m.size() == 3) && (m["A"] == "1") && (m["B"] == "2") &&
         (m["C"] == "3");
}

template <class MapType>
bool AssertMapContent_abc_123(MapType& m) {
  return (m.size() == 3) && (m["a"] == "1") && (m["b"] == "2") &&
         (m["c"] == "3");
}
}  // namespace

TEST(MapStringTest, MixedInlineSize) {
  OrderedFlatMap<std::string, std::string> m_src{
      {"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_TRUE(AssertMapContent_ABC_123(m_src));
  InlineOrderedFlatMap<std::string, std::string, 3> m_src2{
      {"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_TRUE(AssertMapContent_ABC_123(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  OrderedFlatMap<std::string, std::string> m1(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  EXPECT_TRUE(m1 == m_src);
  OrderedFlatMap<std::string, std::string> m2(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineOrderedFlatMap<std::string, std::string, 2> m3(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineOrderedFlatMap<std::string, std::string, 2> m4(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineOrderedFlatMap<std::string, std::string, 5> m5(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineOrderedFlatMap<std::string, std::string, 5> m6(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  OrderedFlatMap<std::string, std::string> m7{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m7));

  InlineOrderedFlatMap<std::string, std::string, 3> m8{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineOrderedFlatMap<std::string, std::string, 2> m9{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineOrderedFlatMap<std::string, std::string, 5> m10{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  OrderedFlatMap<std::string, std::string> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m11));
  EXPECT_TRUE(m7.empty());

  InlineOrderedFlatMap<std::string, std::string, 3> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineOrderedFlatMap<std::string, std::string, 2> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineOrderedFlatMap<std::string, std::string, 5> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  OrderedFlatMap<std::string, std::string> m15{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m15));
  EXPECT_TRUE(m11.empty());

  InlineOrderedFlatMap<std::string, std::string, 3> m16{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m16));
  EXPECT_TRUE(m_src.empty());

  InlineOrderedFlatMap<std::string, std::string, 2> m17{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(MapStringTest, LinearMixedInlineSize) {
  LinearFlatMap<std::string, std::string> m_src{
      {"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_TRUE(AssertMapContent_ABC_123(m_src));
  InlineLinearFlatMap<std::string, std::string, 3> m_src2{
      {"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_TRUE(AssertMapContent_ABC_123(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatMap<std::string, std::string> m1(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatMap<std::string, std::string> m2(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatMap<std::string, std::string, 2> m3(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatMap<std::string, std::string, 2> m4(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatMap<std::string, std::string, 5> m5(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatMap<std::string, std::string, 5> m6(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatMap<std::string, std::string> m7{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m7));

  InlineLinearFlatMap<std::string, std::string, 3> m8{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatMap<std::string, std::string, 2> m9{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatMap<std::string, std::string, 5> m10{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatMap<std::string, std::string> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatMap<std::string, std::string, 3> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatMap<std::string, std::string, 2> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatMap<std::string, std::string, 5> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatMap<std::string, std::string> m15{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatMap<std::string, std::string, 3> m16{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatMap<std::string, std::string, 2> m17{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m17));
  EXPECT_TRUE(m_src2.empty());
}

namespace {
template <class SetType>
bool AssertSetContent_ABC(const SetType& m) {
  return m.size() == 3 && m.contains("A") && m.contains("B") && m.contains("C");
}

template <class SetType>
bool AssertSetContent_abc(const SetType& m) {
  return m.size() == 3 && m.contains("a") && m.contains("b") && m.contains("c");
}
}  // namespace

TEST(SetStringTest, Emplace) {
  OrderedFlatSet<std::string> s;
  s.emplace("ABC", 2);
  s.emplace("D");
  s.insert("AB");
  EXPECT_EQ(s.size(), 2);
  EXPECT_TRUE(s.contains("AB"));
  EXPECT_TRUE(s.contains("D"));
}

TEST(SetStringTest, LinearEmplace) {
  LinearFlatSet<std::string> s;
  s.emplace("ABC", 2);
  s.emplace("D");
  s.insert("AB");
  EXPECT_EQ(s.size(), 2);
  EXPECT_TRUE(s.contains("AB"));
  EXPECT_TRUE(s.contains("D"));
}

TEST(SetStringTest, MixedInlineSize) {
  OrderedFlatSet<std::string> m_src{"A", "B", "C"};
  EXPECT_TRUE(AssertSetContent_ABC(m_src));
  InlineOrderedFlatSet<std::string, 3> m_src2{"A", "B", "C"};
  EXPECT_TRUE(AssertSetContent_ABC(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  OrderedFlatSet<std::string> m1(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m1));
  EXPECT_TRUE(m1 == m_src);
  OrderedFlatSet<std::string> m2(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineOrderedFlatSet<std::string, 2> m3(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineOrderedFlatSet<std::string, 2> m4(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineOrderedFlatSet<std::string, 5> m5(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineOrderedFlatSet<std::string, 5> m6(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  OrderedFlatSet<std::string> m7{"a", "b", "c"};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m7));

  InlineOrderedFlatSet<std::string, 3> m8{"a", "b", "c"};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineOrderedFlatSet<std::string, 2> m9{"a", "b", "c"};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineOrderedFlatSet<std::string, 5> m10{"a", "b", "c"};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  OrderedFlatSet<std::string> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m11));
  EXPECT_TRUE(m7.empty());

  InlineOrderedFlatSet<std::string, 3> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineOrderedFlatSet<std::string, 2> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineOrderedFlatSet<std::string, 5> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  OrderedFlatSet<std::string> m15{"a", "b", "c"};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m15));
  EXPECT_TRUE(m11.empty());

  InlineOrderedFlatSet<std::string, 3> m16{"a", "b", "c"};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m16));
  EXPECT_TRUE(m_src.empty());

  InlineOrderedFlatSet<std::string, 2> m17{"a", "b", "c"};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(SetStringTest, LinearMixedInlineSize) {
  LinearFlatSet<std::string> m_src{"A", "B", "C"};
  EXPECT_TRUE(AssertSetContent_ABC(m_src));
  InlineLinearFlatSet<std::string, 3> m_src2{"A", "B", "C"};
  EXPECT_TRUE(AssertSetContent_ABC(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatSet<std::string> m1(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatSet<std::string> m2(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatSet<std::string, 2> m3(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatSet<std::string, 2> m4(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatSet<std::string, 5> m5(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatSet<std::string, 5> m6(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatSet<std::string> m7{"a", "b", "c"};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m7));

  InlineLinearFlatSet<std::string, 3> m8{"a", "b", "c"};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatSet<std::string, 2> m9{"a", "b", "c"};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatSet<std::string, 5> m10{"a", "b", "c"};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatSet<std::string> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatSet<std::string, 3> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatSet<std::string, 2> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatSet<std::string, 5> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatSet<std::string> m15{"a", "b", "c"};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatSet<std::string, 3> m16{"a", "b", "c"};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatSet<std::string, 2> m17{"a", "b", "c"};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(LinearMap, FromSourceArray) {
  {
    Vector<std::pair<std::string, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    LinearFlatMap<std::string, std::string, void> map(std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
    EXPECT_TRUE(source_array.empty());
  }

  {
    InlineVector<std::pair<std::string, std::string>, 5> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    LinearFlatMap<std::string, std::string, void> map(std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<std::string, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<std::string, std::string, 2, void> map(
        std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<std::string, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<std::string, std::string, 5, void> map(
        std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    InlineVector<std::pair<std::string, std::string>, 5> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<std::string, std::string, 5, void> map(
        std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<std::string, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    LinearFlatMap<std::string, std::string, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
    EXPECT_TRUE(source_array.empty());
  }

  {
    InlineVector<std::pair<std::string, std::string>, 5> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    LinearFlatMap<std::string, std::string, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<std::string, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<std::string, std::string, 2, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<std::string, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<std::string, std::string, 5, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    InlineVector<std::pair<std::string, std::string>, 5> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<std::string, std::string, 5, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }
}

TEST(LinearSet, FromSourceArray) {
  {
    Vector<std::string> source_array{"z", "a", "e"};
    LinearFlatSet<std::string, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
    EXPECT_TRUE(source_array.empty());
  }

  {
    InlineVector<std::string, 5> source_array{"z", "a", "e"};
    LinearFlatSet<std::string, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<std::string> source_array{"z", "a", "e"};
    InlineLinearFlatSet<std::string, 2, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<std::string> source_array{"z", "a", "e"};
    InlineLinearFlatSet<std::string, 5, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    InlineVector<std::string, 5> source_array{"z", "a", "e"};
    InlineLinearFlatSet<std::string, 5, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<std::string> source_array{"z", "a", "e"};
    LinearFlatSet<std::string, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
    EXPECT_TRUE(source_array.empty());
  }

  {
    InlineVector<std::string, 5> source_array{"z", "a", "e"};
    LinearFlatSet<std::string, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<std::string> source_array{"z", "a", "e"};
    InlineLinearFlatSet<std::string, 2, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<std::string> source_array{"z", "a", "e"};
    InlineLinearFlatSet<std::string, 5, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    InlineVector<std::string, 5> source_array{"z", "a", "e"};
    InlineLinearFlatSet<std::string, 5, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }
}

TEST(OrderedMap, Swap) {
  {
    OrderedFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    OrderedFlatMap<std::string, std::string> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    OrderedFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineOrderedFlatMap<std::string, std::string, 2> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    OrderedFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineOrderedFlatMap<std::string, std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    InlineOrderedFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineOrderedFlatMap<std::string, std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    OrderedFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    OrderedFlatMap<std::string, std::string> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    OrderedFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineOrderedFlatMap<std::string, std::string, 2> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    OrderedFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineOrderedFlatMap<std::string, std::string, 5> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    InlineOrderedFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineOrderedFlatMap<std::string, std::string, 5> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }
}

TEST(LinearMap, Swap) {
  {
    LinearFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 2> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    InlineLinearFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 2> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 5> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    InlineLinearFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 5> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }
}

TEST(OrderedSet, Swap) {
  {
    OrderedFlatSet<std::string> m1{"A", "B", "C"};
    OrderedFlatSet<std::string> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    OrderedFlatSet<std::string> m1{"A", "B", "C"};
    InlineOrderedFlatSet<std::string, 2> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    OrderedFlatSet<std::string> m1{"A", "B", "C"};
    InlineOrderedFlatSet<std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    InlineOrderedFlatSet<std::string, 3> m1{"A", "B", "C"};
    InlineOrderedFlatSet<std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    OrderedFlatSet<std::string> m1{"A", "B", "C"};
    OrderedFlatSet<std::string> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    OrderedFlatSet<std::string> m1{"A", "B", "C"};
    InlineOrderedFlatSet<std::string, 2> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    OrderedFlatSet<std::string> m1{"A", "B", "C"};
    InlineOrderedFlatSet<std::string, 5> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    InlineOrderedFlatSet<std::string, 3> m1{"A", "B", "C"};
    InlineOrderedFlatSet<std::string, 5> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }
}

TEST(LinearSet, Swap) {
  {
    LinearFlatSet<std::string> m1{"A", "B", "C"};
    LinearFlatSet<std::string> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<std::string> m1{"A", "B", "C"};
    InlineLinearFlatSet<std::string, 2> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<std::string> m1{"A", "B", "C"};
    InlineLinearFlatSet<std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    InlineLinearFlatSet<std::string, 3> m1{"A", "B", "C"};
    InlineLinearFlatSet<std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<std::string> m1{"A", "B", "C"};
    LinearFlatSet<std::string> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<std::string> m1{"A", "B", "C"};
    InlineLinearFlatSet<std::string, 2> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<std::string> m1{"A", "B", "C"};
    InlineLinearFlatSet<std::string, 5> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    InlineLinearFlatSet<std::string, 3> m1{"A", "B", "C"};
    InlineLinearFlatSet<std::string, 5> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }
}

namespace {
template <class MapType>
bool AssertMapContent_ABCbD_1232040(MapType& m) {
  return (m.size() == 5) && (m["A"] == "1") && (m["B"] == "2") &&
         (m["C"] == "3") && (m["b"] == "20") && (m["D"] == "40");
}

template <class MapType>
bool AssertMapContent_ABCbD_102302040(MapType& m) {
  return (m.size() == 5) && (m["A"] == "10") && (m["B"] == "2") &&
         (m["C"] == "30") && (m["b"] == "20") && (m["D"] == "40");
}

template <class MapType>
bool AssertMapContent_AbCD_10203040(MapType& m) {
  return (m.size() == 4) && (m["A"] == "10") && (m["b"] == "20") &&
         (m["C"] == "30") && (m["D"] == "40");
}

template <class MapType>
bool AssertMapContent_AC_1030(MapType& m) {
  return (m.size() == 2) && (m["A"] == "10") && (m["C"] == "30");
}
}  // namespace

TEST(OrderedMap, Merge) {
  {
    OrderedFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    OrderedFlatMap<std::string, std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    OrderedFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    OrderedFlatMap<std::string, std::string> m2{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    OrderedFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    OrderedFlatMap<std::string, std::string> m2{
        {"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_1232040(m1));
    EXPECT_TRUE(AssertMapContent_AC_1030(m2));
  }

  {
    InlineOrderedFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    OrderedFlatMap<std::string, std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineOrderedFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineOrderedFlatMap<std::string, std::string, 3> m2{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    InlineOrderedFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineOrderedFlatMap<std::string, std::string, 4> m2{
        {"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_1232040(m1));
    EXPECT_TRUE(AssertMapContent_AC_1030(m2));
  }
}

TEST(LinearMap, Merge) {
  {
    LinearFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string> m2{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    LinearFlatMap<std::string, std::string> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string> m2{
        {"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_1232040(m1));
    EXPECT_TRUE(AssertMapContent_AC_1030(m2));
  }

  {
    InlineLinearFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 3> m2{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    InlineLinearFlatMap<std::string, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 4> m2{
        {"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_1232040(m1));
    EXPECT_TRUE(AssertMapContent_AC_1030(m2));
  }
}

template <class K>
struct MergeAssignKeyPolicy : public ReducedHashKeyPolicy<K> {
  static constexpr auto assign_existing_for_merge = true;
};

TEST(LinearMap, MergeAssign) {
  {
    LinearFlatMap<std::string, std::string, MergeAssignKeyPolicy<std::string>>
        m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string, MergeAssignKeyPolicy<std::string>>
        m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<std::string, std::string, MergeAssignKeyPolicy<std::string>>
        m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string, MergeAssignKeyPolicy<std::string>>
        m2{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    LinearFlatMap<std::string, std::string, MergeAssignKeyPolicy<std::string>>
        m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string, MergeAssignKeyPolicy<std::string>>
        m2{{"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_102302040(m1));
    EXPECT_TRUE(AssertMapContent_AbCD_10203040(m2));
  }

  {
    InlineLinearFlatMap<std::string, std::string, 3,
                        MergeAssignKeyPolicy<std::string>>
        m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<std::string, std::string, MergeAssignKeyPolicy<std::string>>
        m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    InlineLinearFlatMap<std::string, std::string, 3,
                        MergeAssignKeyPolicy<std::string>>
        m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 3,
                        MergeAssignKeyPolicy<std::string>>
        m2{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    InlineLinearFlatMap<std::string, std::string, 3,
                        MergeAssignKeyPolicy<std::string>>
        m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<std::string, std::string, 4,
                        MergeAssignKeyPolicy<std::string>>
        m2{{"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_102302040(m1));
    EXPECT_TRUE(AssertMapContent_AbCD_10203040(m2));
  }
}

namespace {
template <class SetType>
bool AssertSetContent_ABCbD(SetType& m) {
  return m.size() == 5 && m.contains("A") && m.contains("B") &&
         m.contains("C") && m.contains("b") && m.contains("D");
}

template <class SetType>
bool AssertSetContent_AC(SetType& m) {
  return m.size() == 2 && m.contains("A") && m.contains("C");
}

template <class SetType>
bool AssertSetContent_AbCD(SetType& m) {
  return m.size() == 4 && m.contains("A") && m.contains("b") &&
         m.contains("C") && m.contains("D");
}
}  // namespace

TEST(OrderedSet, Merge) {
  {
    OrderedFlatSet<std::string> m1{"A", "B", "C"};
    OrderedFlatSet<std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    OrderedFlatSet<std::string> m1{"A", "B", "C"};
    OrderedFlatSet<std::string> m2{"A", "B", "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    OrderedFlatSet<std::string> m1{"A", "B", "C"};
    OrderedFlatSet<std::string> m2{"A", "b", "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AC(m2));
  }

  {
    InlineOrderedFlatSet<std::string, 3> m1{"A", "B", "C"};
    OrderedFlatSet<std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineOrderedFlatSet<std::string, 3> m1{"A", "B", "C"};
    InlineOrderedFlatSet<std::string, 3> m2{"A", "B", "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    InlineOrderedFlatSet<std::string, 3> m1{"A", "B", "C"};
    InlineOrderedFlatSet<std::string, 4> m2{"A", "b", "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AC(m2));
  }
}

TEST(LinearSet, Merge) {
  {
    LinearFlatSet<std::string> m1{"A", "B", "C"};
    LinearFlatSet<std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatSet<std::string> m1{"A", "B", "C"};
    LinearFlatSet<std::string> m2{"A", "B", "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    LinearFlatSet<std::string> m1{"A", "B", "C"};
    LinearFlatSet<std::string> m2{"A", "b", "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AC(m2));
  }

  {
    InlineLinearFlatSet<std::string, 3> m1{"A", "B", "C"};
    LinearFlatSet<std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatSet<std::string, 3> m1{"A", "B", "C"};
    InlineLinearFlatSet<std::string, 3> m2{"A", "B", "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    InlineLinearFlatSet<std::string, 3> m1{"A", "B", "C"};
    InlineLinearFlatSet<std::string, 4> m2{"A", "b", "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AC(m2));
  }
}

TEST(LinearSet, MergeAssign) {
  {
    LinearFlatSet<std::string, MergeAssignKeyPolicy<std::string>> m1{"A", "B",
                                                                     "C"};
    LinearFlatSet<std::string, MergeAssignKeyPolicy<std::string>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<std::string, MergeAssignKeyPolicy<std::string>> m1{"A", "B",
                                                                     "C"};
    LinearFlatSet<std::string, MergeAssignKeyPolicy<std::string>> m2{"A", "B",
                                                                     "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    LinearFlatSet<std::string, MergeAssignKeyPolicy<std::string>> m1{"A", "B",
                                                                     "C"};
    LinearFlatSet<std::string, MergeAssignKeyPolicy<std::string>> m2{"A", "b",
                                                                     "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AbCD(m2));
  }

  {
    InlineLinearFlatSet<std::string, 3, MergeAssignKeyPolicy<std::string>> m1{
        "A", "B", "C"};
    LinearFlatSet<std::string, MergeAssignKeyPolicy<std::string>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    InlineLinearFlatSet<std::string, 3, MergeAssignKeyPolicy<std::string>> m1{
        "A", "B", "C"};
    InlineLinearFlatSet<std::string, 3, MergeAssignKeyPolicy<std::string>> m2{
        "A", "B", "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    InlineLinearFlatSet<std::string, 3, MergeAssignKeyPolicy<std::string>> m1{
        "A", "B", "C"};
    InlineLinearFlatSet<std::string, 4, MergeAssignKeyPolicy<std::string>> m2{
        "A", "b", "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AbCD(m2));
  }
}

TEST(Vector, LinearMapInsertOrAssignBaseStringKey) {
  InlineLinearFlatMap<String, std::string, 5> map{
      {"3", "c"}, {"2", "b"}, {"1", "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.capacity(), 5);
  EXPECT_EQ(map["1"], "a");
  EXPECT_EQ(map["2"], "b");
  EXPECT_EQ(map["3"], "c");
  EXPECT_EQ(map["4"], "");

  auto r = map.insert_or_assign("4", "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map["4"], "d");

  String s5("5");
  std::string se = "e";
  auto r2 = map.insert_or_assign(s5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map["5"], "e");
  EXPECT_EQ(s5, "5");
  EXPECT_TRUE(se.empty());

  String s6("6");
  std::string sf = "f";
  auto r3 = map.insert_or_assign(std::move(s6), std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map["6"], "f");
  EXPECT_TRUE(s6.empty());
  EXPECT_TRUE(sf.empty());

  String s7("7");
  std::string sg = "g";
  auto r4 = map.insert_or_assign(std::move(s7), sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map["7"], "g");
  EXPECT_TRUE(s7.empty());
  EXPECT_EQ(sg, "g");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearMapEmplaceOrAssignBaseStringKey) {
  InlineLinearFlatMap<String, std::string, 5> map{
      {"3", "c"}, {"2", "b"}, {"1", "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map["1"], "a");
  EXPECT_EQ(map["2"], "b");
  EXPECT_EQ(map["3"], "c");
  EXPECT_EQ(map["4"], "");

  auto r = map.emplace_or_assign("4", "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map["4"], "d");

  String s5("5");
  std::string se = "e";
  auto r2 = map.emplace_or_assign(s5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map["5"], "e");
  EXPECT_EQ(s5, "5");
  EXPECT_TRUE(se.empty());

  String s6("6");
  std::string sf = "f";
  auto r3 = map.emplace_or_assign(std::move(s6), std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map["6"], "f");
  EXPECT_TRUE(s6.empty());
  EXPECT_TRUE(sf.empty());

  String s7("7");
  std::string sg = "g";
  auto r4 = map.emplace_or_assign(std::move(s7), sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map["7"], "g");
  EXPECT_TRUE(s7.empty());
  EXPECT_EQ(sg, "g");

  auto r5 = map.emplace_or_assign("7", 5, 'g');
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(map["7"], "ggggg");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearSetInsertBaseStringKey) {
  LinearFlatSet<String> set{"1", "2", "3"};
  auto r = set.insert("3");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(set.size(), 3);

  auto r2 = set.insert("4");
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(set.size(), 4);

  std::string s5 = "5";
  auto r3 = set.insert(std::move(s5));
  EXPECT_TRUE(r3.second);
  EXPECT_TRUE(s5.empty());

  EXPECT_EQ(set.size(), 5);
  EXPECT_TRUE(set.contains("1"));
  EXPECT_TRUE(set.contains("2"));
  EXPECT_TRUE(set.contains("3"));
  EXPECT_TRUE(set.contains("4"));
  EXPECT_TRUE(set.contains("5"));
  EXPECT_FALSE(set.contains("6"));
}

TEST(Vector, LinearSetInsertUniqueBaseStringKey) {
  LinearFlatSet<String> set;
  set.insert_unique("1");
  set.insert_unique("3");
  const String two("2");
  set.insert_unique(two);
  set.insert_unique("6");
  auto it = set.insert_unique("0");
  EXPECT_EQ(set.size(), 5);
  EXPECT_TRUE(set.contains("1"));
  EXPECT_TRUE(set.contains("3"));
  EXPECT_TRUE(set.contains("2"));
  EXPECT_TRUE(set.contains("6"));
  EXPECT_TRUE(set.contains("0"));
  EXPECT_TRUE(*it == "0");
}

TEST(Vector, LinearMapEmplaceBaseStringKey) {
  LinearFlatMap<String, std::string> map;
  auto r = map.emplace(std::piecewise_construct,
                       std::tuple<const char*, size_t>("123", 2),
                       std::tuple<const char*, size_t>("abc", 2));
  EXPECT_TRUE(r.second);
  EXPECT_EQ(r.first->first, "12");
  EXPECT_EQ(r.first->second, "ab");
  auto r2 = map.emplace(std::piecewise_construct,
                        std::tuple<const char*, size_t>("112", 2),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(r2.first->first, "11");
  EXPECT_EQ(r2.first->second, "xy");

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");

  auto r3 = map.emplace(std::piecewise_construct, std::forward_as_tuple("12"),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, "12");
  EXPECT_EQ(r3.first->second, "ab");

  EXPECT_EQ(map.size(), 2);

  auto r4 = map.try_emplace("11", "ab");
  EXPECT_FALSE(r4.second);
  EXPECT_EQ(r4.first->first, "11");
  EXPECT_EQ(r4.first->second, "xy");

  String s11("11");
  std::string sXYZ = "xyz";
  auto r5 = map.try_emplace(std::move(s11), std::move(sXYZ));
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(r5.first->first, "11");
  EXPECT_EQ(r5.first->second, "xy");
  EXPECT_EQ(s11, "11");
  EXPECT_EQ(sXYZ, "xyz");

  std::string s13 = "13";
  auto r6 = map.try_emplace(std::move(s13), std::move(sXYZ));
  EXPECT_TRUE(r6.second);
  EXPECT_EQ(r6.first->first, "13");
  EXPECT_EQ(r6.first->second, "xyz");
  EXPECT_TRUE(s13.empty());
  EXPECT_TRUE(sXYZ.empty());

  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");

  std::string s14 = "14";
  std::string sUVW = "uvw";
  auto r7 = map.try_emplace(s14, sUVW);
  EXPECT_TRUE(r7.second);
  EXPECT_EQ(r7.first->first, "14");
  EXPECT_EQ(r7.first->second, "uvw");
  EXPECT_EQ(s14, "14");
  EXPECT_EQ(sUVW, "uvw");

  EXPECT_EQ(map.size(), 4);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");
  EXPECT_EQ(map["14"], "uvw");
}

TEST(MapStringTest, LinearBasicOperationsBaseStringKey) {
  LinearFlatMap<String, std::string> m;

  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.size(), 0);

  auto ret = m.insert({"apple", "red"});
  ASSERT_TRUE(ret.second);
  EXPECT_EQ(ret.first->first, "apple");
  EXPECT_EQ(ret.first->second, "red");
  EXPECT_EQ(m.size(), 1);
}

TEST(MapStringTest, LinearElementAccessBaseStringKey) {
  LinearFlatMap<String, std::string> m{{"apple", "red"}, {"banana", "yellow"}};

  EXPECT_EQ(m["apple"], "red");

  m["apple"] = "green";
  EXPECT_EQ(m["apple"], "green");
  EXPECT_EQ(m.at("apple"), "green");

  EXPECT_EQ(m["grape"], "");
  EXPECT_EQ(m.at("grape"), "");
  EXPECT_EQ(m.size(), 3);

  EXPECT_EQ(m.at("orange"), "");
  m.at("orange") = "orange";
  EXPECT_EQ(m["orange"], "orange");
  EXPECT_EQ(m.size(), 4);

  std::string melon = "melon";
  m.at(std::move(melon)) = "green";
  EXPECT_EQ(melon, "");
  EXPECT_EQ(m.find("melon")->second, "green");
  EXPECT_EQ(m.size(), 5);

  std::string pear = "pear";
  auto r = m.insert_default_if_absent(std::move(pear));
  EXPECT_TRUE(r.second);
  r.first->second = "yellow";
  EXPECT_EQ(pear, "");
  EXPECT_EQ(m["pear"], "yellow");
  EXPECT_EQ(m.size(), 6);

  auto r2 = m.insert_default_if_absent("apple");
  EXPECT_FALSE(r2.second);
  EXPECT_EQ(m["apple"], "green");
  r2.first->second = "red";
  EXPECT_EQ(m["apple"], "red");
  EXPECT_EQ(m.size(), 6);

  std::string black = "black";
  auto r3 = m.insert_if_absent("apple", black);
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, "apple");
  EXPECT_EQ(r3.first->second, "red");
  EXPECT_EQ(m.size(), 6);

  std::string peach = "peach";
  std::string pink = "pink";
  auto r4 = m.insert_if_absent(std::move(peach), std::move(pink));
  EXPECT_TRUE(r4.second);
  EXPECT_TRUE(peach.empty());
  EXPECT_TRUE(pink.empty());
  EXPECT_EQ(r4.first->first, "peach");
  EXPECT_EQ(r4.first->second, "pink");
  EXPECT_EQ(m.size(), 7);

  auto r5 = m.insert_if_absent("tomato", black);
  EXPECT_TRUE(r5.second);
  EXPECT_EQ(r5.first->first, "tomato");
  EXPECT_EQ(r5.first->second, "black");
  EXPECT_EQ(black, "black");
  EXPECT_EQ(m.size(), 8);
}

TEST(MapStringTest, LinearInsertUpdateBaseStringKey) {
  LinearFlatMap<String, std::string> m;

  auto ret1 = m.insert({"fruit", "apple"});
  EXPECT_TRUE(ret1.second);
  auto ret2 = m.insert({"fruit", "banana"});
  EXPECT_FALSE(ret2.second);
  EXPECT_EQ(ret2.first->second, "apple");

  auto emp_ret = m.emplace("color", "blue");
  EXPECT_TRUE(emp_ret.second);
  EXPECT_EQ(emp_ret.first->first, "color");

  m["color"] = "red";
  EXPECT_EQ(m["color"], "red");

  std::pair<const String, std::string> value_type{String("vehicle"), "car"};
  auto ret3 = m.insert(value_type);
  EXPECT_TRUE(ret3.second);
  EXPECT_TRUE(!value_type.first.empty());
  auto ret4 = m.insert(std::move(value_type));
  EXPECT_FALSE(ret4.second);
  EXPECT_TRUE(!value_type.second.empty());

  m.erase("vehicle");
  auto ret5 = m.insert(std::move(value_type));
  EXPECT_TRUE(ret5.second);
  EXPECT_TRUE(!value_type.first.empty());
  EXPECT_TRUE(value_type.second.empty());

  std::pair<const String, std::string> value_data0{"job", "doctor"};
  std::pair<const String, std::string> value_data1{"road", "highway"};
  std::pair<String, std::string> value_data2{"building", "hospital"};
  std::pair<String, std::string> value_data3{"animal", "tiger"};
  auto it = m.insert_unique(value_data0);
  EXPECT_TRUE(it->first == "job" && it->second == "doctor");
  EXPECT_FALSE(value_data0.first.empty());
  EXPECT_FALSE(value_data0.second.empty());
  auto it2 = m.insert_unique(std::move(value_data1));
  EXPECT_TRUE(it2->first == "road" && it2->second == "highway");
  EXPECT_FALSE(value_data1.first.empty());
  EXPECT_TRUE(value_data1.second.empty());
  auto it3 = m.insert_unique(value_data2);
  EXPECT_TRUE(it3->first == "building" && it3->second == "hospital");
  EXPECT_FALSE(value_data2.first.empty());
  EXPECT_FALSE(value_data2.second.empty());
  auto it4 = m.insert_unique(std::move(value_data3));
  EXPECT_TRUE(it4->first == "animal" && it4->second == "tiger");
  EXPECT_TRUE(value_data3.first.empty());
  EXPECT_TRUE(value_data3.second.empty());
  EXPECT_TRUE(m.contains("job"));
  EXPECT_TRUE(m.contains("road"));
  EXPECT_TRUE(m.contains("building"));
  EXPECT_TRUE(m.contains("animal"));
  auto it5 = m.emplace_unique("number", "111", 2);
  EXPECT_TRUE(it5->first == "number" && it5->second == "11");
  EXPECT_TRUE(m["number"] == "11");
  String key_body = "body";
  auto it6 = m.emplace_unique(key_body, "hand");
  EXPECT_TRUE(it6->first == key_body && it6->second == "hand");
  EXPECT_TRUE(!key_body.empty());
  EXPECT_TRUE(m["body"] == "hand");
  auto it7 = m.emplace_unique(std::piecewise_construct,
                              std::forward_as_tuple("letter"),
                              std::forward_as_tuple(5, 'X'));
  EXPECT_TRUE(it7->first == "letter" && it7->second == "XXXXX");
  EXPECT_TRUE(m["letter"] == "XXXXX");
}

TEST(MapStringTest, LinearEraseOperationsBaseStringKey) {
  LinearFlatMap<String, std::string> m{{"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_EQ(m.size(), 3);
  EXPECT_EQ(m["A"], "1");
  EXPECT_EQ(m["B"], "2");
  EXPECT_EQ(m["C"], "3");

  size_t cnt = m.erase("B");
  EXPECT_EQ(cnt, 1);
  EXPECT_EQ(m.size(), 2);
  EXPECT_FALSE(m.contains("B"));

  auto it = m.find("A");
  m.erase(it);
  EXPECT_EQ(m.size(), 1);

  EXPECT_EQ(m.erase("X"), 0);
}

TEST(SetStringTest, LinearIteratorsBaseStringKey) {
  InlineLinearFlatSet<String, 10> s{"a", "z", "c", "b", "m", "g", "q", "h"};
  std::string order;
  for (auto it = s.begin(); it != s.end(); it++) {
    order += (*it).str();
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "abcghmqz" : "azcbmgqh");

  order = "";
  for (auto it = s.rbegin(); it != s.rend(); it++) {
    order += (*it).str();
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "zqmhgcba" : "hqgmbcza");
}

TEST(SetStringTest, LinearBasicBaseStringKey) {
  InlineLinearFlatSet<String, 10> s{"a", "z", "c", "b", "m", "g", "q", "h"};
  EXPECT_TRUE(s.is_static_buffer());
  EXPECT_TRUE(s.contains("a"));
  EXPECT_FALSE(s.contains("y"));
  EXPECT_TRUE(s.find("c") != s.end());
  EXPECT_TRUE(s.find("y") == s.end());
  EXPECT_TRUE(s.count("q") == 1);
  EXPECT_TRUE(s.count("y") == 0);
  EXPECT_EQ(s.erase("y"), 0);
  EXPECT_EQ(s.size(), 8);
  EXPECT_EQ(s.erase("z"), 1);
  EXPECT_EQ(s.size(), 7);
  EXPECT_FALSE(s.contains("z"));
  auto it = s.erase(s.find("g"));
  EXPECT_EQ(s.size(), 6);
  EXPECT_EQ(*it, s.is_data_ordered() ? "h" : "q");
}

TEST(MapStringTest, LinearIteratorsBaseStringKey) {
  LinearFlatMap<String, std::string> m{{"Z", "26"}, {"A", "1"}, {"M", "13"}};

  auto it = m.begin();
  EXPECT_EQ(it->first, m.is_data_ordered() ? "A" : "Z");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? "M" : "A");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? "Z" : "M");
  ++it;
  EXPECT_EQ(it, m.end());

  auto rit = m.rbegin();
  EXPECT_EQ(rit->first, m.is_data_ordered() ? "Z" : "M");
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? "M" : "A");
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? "A" : "Z");
  ++rit;
  EXPECT_EQ(rit, m.rend());
}

TEST(MapStringTest, LinearEdgeCasesBaseStringKey) {
  LinearFlatMap<String, std::string> m;

  m[""] = "empty_key";
  m.emplace("empty_value", "");
  EXPECT_EQ(m[""], "empty_key");
  EXPECT_EQ(m["empty_value"], "");

  std::string big_key(1000, 'K');
  std::string big_value(10000, 'V');
  m[big_key] = big_value;
  EXPECT_EQ(m[big_key].size(), 10000);
}

TEST(MapStringTest, LinearInsertOrAssignBaseStringKey) {
  LinearFlatMap<String, std::string> m;

  {
    auto [it, inserted] = m.insert_or_assign("fruit", "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.insert_or_assign("fruit", "banana");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "banana");
    EXPECT_EQ(m.size(), 1);
  }

  m.insert_or_assign("empty", "");
  EXPECT_EQ(m["empty"], "");

  auto [it, _] = m.insert_or_assign("new_key", "value");
  EXPECT_EQ(it->first, "new_key");
}

TEST(MapStringTest, LinearEmplaceOrAssignBaseStringKey) {
  LinearFlatMap<String, std::string> m;

  {
    auto [it, inserted] = m.emplace_or_assign("fruit", "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.emplace_or_assign("fruit", "banana", 4);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "bana");
    EXPECT_EQ(m["fruit"], "bana");
    EXPECT_EQ(m.size(), 1);
  }

  m.emplace_or_assign("empty", "");
  EXPECT_EQ(m["empty"], "");

  auto [it, _] = m.emplace_or_assign("new_key", "value");
  EXPECT_EQ(it->first, "new_key");
}

TEST(MapStringTest, LinearEmplacePiecewiseBaseStringKey) {
  LinearFlatMap<String, std::string> m;

  auto emp_it =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple(5, 'X'));
  ASSERT_TRUE(emp_it.second);
  EXPECT_EQ(emp_it.first->second, "XXXXX");

  m.emplace(std::piecewise_construct, std::forward_as_tuple("KKKKK", 3),
            std::forward_as_tuple(3, 'k'));
  EXPECT_EQ(m["KKK"], "kkk");

  auto emp_fail =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple("new_value"));
  EXPECT_FALSE(emp_fail.second);
  EXPECT_EQ(m["piece_key"], "XXXXX");
}

TEST(MapStringTest, LinearMixedInlineSizeBaseStringKey) {
  LinearFlatMap<String, std::string> m_src{{"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_TRUE(AssertMapContent_ABC_123(m_src));
  InlineLinearFlatMap<String, std::string, 3> m_src2{
      {"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_TRUE(AssertMapContent_ABC_123(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatMap<String, std::string> m1(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatMap<String, std::string> m2(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatMap<String, std::string, 2> m3(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatMap<String, std::string, 2> m4(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatMap<String, std::string, 5> m5(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatMap<String, std::string, 5> m6(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatMap<String, std::string> m7{{"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m7));

  InlineLinearFlatMap<String, std::string, 3> m8{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatMap<String, std::string, 2> m9{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatMap<String, std::string, 5> m10{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatMap<String, std::string> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatMap<String, std::string, 3> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatMap<String, std::string, 2> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatMap<String, std::string, 5> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatMap<String, std::string> m15{{"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatMap<String, std::string, 3> m16{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatMap<String, std::string, 2> m17{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(SetStringTest, LinearEmplaceBaseStringKey) {
  LinearFlatSet<String> s;
  s.emplace("ABC", 2);
  s.emplace("D");
  s.insert("AB");
  EXPECT_EQ(s.size(), 2);
  EXPECT_TRUE(s.contains("AB"));
  EXPECT_TRUE(s.contains("D"));
}

TEST(SetStringTest, LinearMixedInlineSizeBaseStringKey) {
  LinearFlatSet<String> m_src{"A", "B", "C"};
  EXPECT_TRUE(AssertSetContent_ABC(m_src));
  InlineLinearFlatSet<String, 3> m_src2{"A", "B", "C"};
  EXPECT_TRUE(AssertSetContent_ABC(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatSet<String> m1(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatSet<String> m2(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatSet<String, 2> m3(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatSet<String, 2> m4(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatSet<String, 5> m5(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatSet<String, 5> m6(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatSet<String> m7{"a", "b", "c"};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m7));

  InlineLinearFlatSet<String, 3> m8{"a", "b", "c"};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatSet<String, 2> m9{"a", "b", "c"};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatSet<String, 5> m10{"a", "b", "c"};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatSet<String> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatSet<String, 3> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatSet<String, 2> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatSet<String, 5> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatSet<String> m15{"a", "b", "c"};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatSet<String, 3> m16{"a", "b", "c"};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatSet<String, 2> m17{"a", "b", "c"};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(LinearMap, FromSourceArrayBaseStringKey) {
  {
    Vector<std::pair<String, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    LinearFlatMap<String, std::string, void> map(std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
    EXPECT_TRUE(source_array.empty());
  }

  {
    InlineVector<std::pair<String, std::string>, 5> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    LinearFlatMap<String, std::string, void> map(std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<String, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<String, std::string, 2, void> map(
        std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<String, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<String, std::string, 5, void> map(
        std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    InlineVector<std::pair<String, std::string>, 5> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<String, std::string, 5, void> map(
        std::move(source_array));
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<String, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    LinearFlatMap<String, std::string, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
    EXPECT_TRUE(source_array.empty());
  }

  {
    InlineVector<std::pair<String, std::string>, 5> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    LinearFlatMap<String, std::string, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<String, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<String, std::string, 2, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    Vector<std::pair<String, std::string>> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<String, std::string, 5, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }

  {
    InlineVector<std::pair<String, std::string>, 5> source_array{
        {"z", "Z"}, {"a", "A"}, {"e", "E"}};
    InlineLinearFlatMap<String, std::string, 5, void> map;
    map = std::move(source_array);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map["z"], "Z");
    EXPECT_EQ(map["a"], "A");
    EXPECT_EQ(map["e"], "E");
  }
}

TEST(LinearSet, FromSourceArrayBaseStringKey) {
  {
    Vector<String> source_array{"z", "a", "e"};
    LinearFlatSet<String, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
    EXPECT_TRUE(source_array.empty());
  }

  {
    InlineVector<String, 5> source_array{"z", "a", "e"};
    LinearFlatSet<String, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<String> source_array{"z", "a", "e"};
    InlineLinearFlatSet<String, 2, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<String> source_array{"z", "a", "e"};
    InlineLinearFlatSet<String, 5, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    InlineVector<String, 5> source_array{"z", "a", "e"};
    InlineLinearFlatSet<String, 5, void> set(std::move(source_array));
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<String> source_array{"z", "a", "e"};
    LinearFlatSet<String, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
    EXPECT_TRUE(source_array.empty());
  }

  {
    InlineVector<String, 5> source_array{"z", "a", "e"};
    LinearFlatSet<String, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<String> source_array{"z", "a", "e"};
    InlineLinearFlatSet<String, 2, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    Vector<String> source_array{"z", "a", "e"};
    InlineLinearFlatSet<String, 5, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }

  {
    InlineVector<String, 5> source_array{"z", "a", "e"};
    InlineLinearFlatSet<String, 5, void> set;
    set = std::move(source_array);
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("z"));
    EXPECT_TRUE(set.contains("a"));
    EXPECT_TRUE(set.contains("e"));
  }
}

TEST(LinearMap, SwapBaseStringKey) {
  {
    LinearFlatMap<String, std::string> m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string> m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 2> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string> m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    InlineLinearFlatMap<String, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string> m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string> m2{{"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string> m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 2> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string> m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 5> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    InlineLinearFlatMap<String, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 5> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }
}

TEST(LinearSet, SwapBaseStringKey) {
  {
    LinearFlatSet<String> m1{"A", "B", "C"};
    LinearFlatSet<String> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 2> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    InlineLinearFlatSet<String, 3> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String> m1{"A", "B", "C"};
    LinearFlatSet<String> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 2> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 5> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    InlineLinearFlatSet<String, 3> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 5> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }
}

TEST(LinearMap, MergeBaseStringKey) {
  {
    LinearFlatMap<String, std::string> m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatMap<String, std::string> m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string> m2{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    LinearFlatMap<String, std::string> m1{{"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string> m2{
        {"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_1232040(m1));
    EXPECT_TRUE(AssertMapContent_AC_1030(m2));
  }

  {
    InlineLinearFlatMap<String, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatMap<String, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 3> m2{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    InlineLinearFlatMap<String, std::string, 3> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 4> m2{
        {"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_1232040(m1));
    EXPECT_TRUE(AssertMapContent_AC_1030(m2));
  }
}

TEST(LinearSet, MergeBaseStringKey) {
  {
    LinearFlatSet<String> m1{"A", "B", "C"};
    LinearFlatSet<String> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatSet<String> m1{"A", "B", "C"};
    LinearFlatSet<String> m2{"A", "B", "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    LinearFlatSet<String> m1{"A", "B", "C"};
    LinearFlatSet<String> m2{"A", "b", "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AC(m2));
  }

  {
    InlineLinearFlatSet<String, 3> m1{"A", "B", "C"};
    LinearFlatSet<String> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatSet<String, 3> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 3> m2{"A", "B", "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    InlineLinearFlatSet<String, 3> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 4> m2{"A", "b", "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AC(m2));
  }
}

TEST(Vector, LinearMapInsertOrAssignBaseStringKeyWithPolicy) {
  InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> map{
      {"3", "c"}, {"2", "b"}, {"1", "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.capacity(), 5);
  EXPECT_EQ(map["1"], "a");
  EXPECT_EQ(map["2"], "b");
  EXPECT_EQ(map["3"], "c");
  EXPECT_EQ(map["4"], "");

  auto r = map.insert_or_assign("4", "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map["4"], "d");

  String s5("5");
  std::string se = "e";
  auto r2 = map.insert_or_assign(s5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map["5"], "e");
  EXPECT_EQ(s5, "5");
  EXPECT_TRUE(se.empty());

  String s6("6");
  std::string sf = "f";
  auto r3 = map.insert_or_assign(std::move(s6), std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map["6"], "f");
  EXPECT_TRUE(s6.empty());
  EXPECT_TRUE(sf.empty());

  String s7("7");
  std::string sg = "g";
  auto r4 = map.insert_or_assign(std::move(s7), sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map["7"], "g");
  EXPECT_TRUE(s7.empty());
  EXPECT_EQ(sg, "g");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearMapEmplaceOrAssignBaseStringKeyWithPolicy) {
  InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> map{
      {"3", "c"}, {"2", "b"}, {"1", "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map["1"], "a");
  EXPECT_EQ(map["2"], "b");
  EXPECT_EQ(map["3"], "c");
  EXPECT_EQ(map["4"], "");

  auto r = map.emplace_or_assign("4", "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map["4"], "d");

  String s5("5");
  std::string se = "e";
  auto r2 = map.emplace_or_assign(s5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map["5"], "e");
  EXPECT_EQ(s5, "5");
  EXPECT_TRUE(se.empty());

  String s6("6");
  std::string sf = "f";
  auto r3 = map.emplace_or_assign(std::move(s6), std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map["6"], "f");
  EXPECT_TRUE(s6.empty());
  EXPECT_TRUE(sf.empty());

  String s7("7");
  std::string sg = "g";
  auto r4 = map.emplace_or_assign(std::move(s7), sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map["7"], "g");
  EXPECT_TRUE(s7.empty());
  EXPECT_EQ(sg, "g");

  auto r5 = map.emplace_or_assign("7", 5, 'g');
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(map["7"], "ggggg");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearSetInsertBaseStringKeyWithPolicy) {
  LinearFlatSet<String, KeyPolicy<String>> set{"1", "2", "3"};
  auto r = set.insert("3");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(set.size(), 3);

  auto r2 = set.insert("4");
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(set.size(), 4);

  std::string s5 = "5";
  auto r3 = set.insert(std::move(s5));
  EXPECT_TRUE(r3.second);
  EXPECT_TRUE(s5.empty());

  EXPECT_EQ(set.size(), 5);
  EXPECT_TRUE(set.contains("1"));
  EXPECT_TRUE(set.contains("2"));
  EXPECT_TRUE(set.contains("3"));
  EXPECT_TRUE(set.contains("4"));
  EXPECT_TRUE(set.contains("5"));
  EXPECT_FALSE(set.contains("6"));
}

TEST(Vector, LinearSetInsertUniqueBaseStringKeyWithPolicy) {
  LinearFlatSet<String, KeyPolicy<String>> set;
  set.insert_unique("1");
  set.insert_unique("3");
  const String two("2");
  set.insert_unique(two);
  set.insert_unique("6");
  auto it = set.insert_unique("0");
  EXPECT_EQ(set.size(), 5);
  EXPECT_TRUE(set.contains("1"));
  EXPECT_TRUE(set.contains("3"));
  EXPECT_TRUE(set.contains("2"));
  EXPECT_TRUE(set.contains("6"));
  EXPECT_TRUE(set.contains("0"));
  EXPECT_TRUE(*it == "0");
}

TEST(Vector, LinearMapEmplaceBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> map;
  auto r = map.emplace(std::piecewise_construct,
                       std::tuple<const char*, size_t>("123", 2),
                       std::tuple<const char*, size_t>("abc", 2));
  EXPECT_TRUE(r.second);
  EXPECT_EQ(r.first->first, "12");
  EXPECT_EQ(r.first->second, "ab");
  auto r2 = map.emplace(std::piecewise_construct,
                        std::tuple<const char*, size_t>("112", 2),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(r2.first->first, "11");
  EXPECT_EQ(r2.first->second, "xy");

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");

  auto r3 = map.emplace(std::piecewise_construct, std::forward_as_tuple("12"),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, "12");
  EXPECT_EQ(r3.first->second, "ab");

  EXPECT_EQ(map.size(), 2);

  auto r4 = map.try_emplace("11", "ab");
  EXPECT_FALSE(r4.second);
  EXPECT_EQ(r4.first->first, "11");
  EXPECT_EQ(r4.first->second, "xy");

  String s11("11");
  std::string sXYZ = "xyz";
  auto r5 = map.try_emplace(std::move(s11), std::move(sXYZ));
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(r5.first->first, "11");
  EXPECT_EQ(r5.first->second, "xy");
  EXPECT_EQ(s11, "11");
  EXPECT_EQ(sXYZ, "xyz");

  std::string s13 = "13";
  auto r6 = map.try_emplace(std::move(s13), std::move(sXYZ));
  EXPECT_TRUE(r6.second);
  EXPECT_EQ(r6.first->first, "13");
  EXPECT_EQ(r6.first->second, "xyz");
  EXPECT_TRUE(s13.empty());
  EXPECT_TRUE(sXYZ.empty());

  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");

  std::string s14 = "14";
  std::string sUVW = "uvw";
  auto r7 = map.try_emplace(s14, sUVW);
  EXPECT_TRUE(r7.second);
  EXPECT_EQ(r7.first->first, "14");
  EXPECT_EQ(r7.first->second, "uvw");
  EXPECT_EQ(s14, "14");
  EXPECT_EQ(sUVW, "uvw");

  EXPECT_EQ(map.size(), 4);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");
  EXPECT_EQ(map["14"], "uvw");
}

TEST(MapStringTest, LinearBasicOperationsBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m;

  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.size(), 0);

  auto ret = m.insert({"apple", "red"});
  ASSERT_TRUE(ret.second);
  EXPECT_EQ(ret.first->first, "apple");
  EXPECT_EQ(ret.first->second, "red");
  EXPECT_EQ(m.size(), 1);
}

TEST(MapStringTest, LinearElementAccessBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m{{"apple", "red"},
                                                          {"banana", "yellow"}};

  EXPECT_EQ(m["apple"], "red");

  m["apple"] = "green";
  EXPECT_EQ(m["apple"], "green");
  EXPECT_EQ(m.at("apple"), "green");

  EXPECT_EQ(m["grape"], "");
  EXPECT_EQ(m.at("grape"), "");
  EXPECT_EQ(m.size(), 3);

  EXPECT_EQ(m.at("orange"), "");
  m.at("orange") = "orange";
  EXPECT_EQ(m["orange"], "orange");
  EXPECT_EQ(m.size(), 4);

  std::string melon = "melon";
  m.at(std::move(melon)) = "green";
  EXPECT_EQ(melon, "");
  EXPECT_EQ(m.find("melon")->second, "green");
  EXPECT_EQ(m.size(), 5);

  std::string pear = "pear";
  auto r = m.insert_default_if_absent(std::move(pear));
  EXPECT_TRUE(r.second);
  r.first->second = "yellow";
  EXPECT_EQ(pear, "");
  EXPECT_EQ(m["pear"], "yellow");
  EXPECT_EQ(m.size(), 6);

  auto r2 = m.insert_default_if_absent("apple");
  EXPECT_FALSE(r2.second);
  EXPECT_EQ(m["apple"], "green");
  r2.first->second = "red";
  EXPECT_EQ(m["apple"], "red");
  EXPECT_EQ(m.size(), 6);

  std::string black = "black";
  auto r3 = m.insert_if_absent("apple", black);
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, "apple");
  EXPECT_EQ(r3.first->second, "red");
  EXPECT_EQ(m.size(), 6);

  std::string peach = "peach";
  std::string pink = "pink";
  auto r4 = m.insert_if_absent(std::move(peach), std::move(pink));
  EXPECT_TRUE(r4.second);
  EXPECT_TRUE(peach.empty());
  EXPECT_TRUE(pink.empty());
  EXPECT_EQ(r4.first->first, "peach");
  EXPECT_EQ(r4.first->second, "pink");
  EXPECT_EQ(m.size(), 7);

  auto r5 = m.insert_if_absent("tomato", black);
  EXPECT_TRUE(r5.second);
  EXPECT_EQ(r5.first->first, "tomato");
  EXPECT_EQ(r5.first->second, "black");
  EXPECT_EQ(black, "black");
  EXPECT_EQ(m.size(), 8);
}

TEST(MapStringTest, LinearInsertUpdateBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m;

  auto ret1 = m.insert({"fruit", "apple"});
  EXPECT_TRUE(ret1.second);
  auto ret2 = m.insert({"fruit", "banana"});
  EXPECT_FALSE(ret2.second);
  EXPECT_EQ(ret2.first->second, "apple");

  auto emp_ret = m.emplace("color", "blue");
  EXPECT_TRUE(emp_ret.second);
  EXPECT_EQ(emp_ret.first->first, "color");

  m["color"] = "red";
  EXPECT_EQ(m["color"], "red");

  std::pair<const String, std::string> value_type{String("vehicle"), "car"};
  auto ret3 = m.insert(value_type);
  EXPECT_TRUE(ret3.second);
  EXPECT_TRUE(!value_type.first.empty());
  auto ret4 = m.insert(std::move(value_type));
  EXPECT_FALSE(ret4.second);
  EXPECT_TRUE(!value_type.second.empty());

  m.erase("vehicle");
  auto ret5 = m.insert(std::move(value_type));
  EXPECT_TRUE(ret5.second);
  EXPECT_TRUE(!value_type.first.empty());
  EXPECT_TRUE(value_type.second.empty());

  std::pair<const String, std::string> value_data0{"job", "doctor"};
  std::pair<const String, std::string> value_data1{"road", "highway"};
  std::pair<String, std::string> value_data2{"building", "hospital"};
  std::pair<String, std::string> value_data3{"animal", "tiger"};
  auto it = m.insert_unique(value_data0);
  EXPECT_TRUE(it->first == "job" && it->second == "doctor");
  EXPECT_FALSE(value_data0.first.empty());
  EXPECT_FALSE(value_data0.second.empty());
  auto it2 = m.insert_unique(std::move(value_data1));
  EXPECT_TRUE(it2->first == "road" && it2->second == "highway");
  EXPECT_FALSE(value_data1.first.empty());
  EXPECT_TRUE(value_data1.second.empty());
  auto it3 = m.insert_unique(value_data2);
  EXPECT_TRUE(it3->first == "building" && it3->second == "hospital");
  EXPECT_FALSE(value_data2.first.empty());
  EXPECT_FALSE(value_data2.second.empty());
  auto it4 = m.insert_unique(std::move(value_data3));
  EXPECT_TRUE(it4->first == "animal" && it4->second == "tiger");
  EXPECT_TRUE(value_data3.first.empty());
  EXPECT_TRUE(value_data3.second.empty());
  EXPECT_TRUE(m.contains("job"));
  EXPECT_TRUE(m.contains("road"));
  EXPECT_TRUE(m.contains("building"));
  EXPECT_TRUE(m.contains("animal"));
  auto it5 = m.emplace_unique("number", "111", 2);
  EXPECT_TRUE(it5->first == "number" && it5->second == "11");
  EXPECT_TRUE(m["number"] == "11");
  String key_body = "body";
  auto it6 = m.emplace_unique(key_body, "hand");
  EXPECT_TRUE(it6->first == key_body && it6->second == "hand");
  EXPECT_TRUE(!key_body.empty());
  EXPECT_TRUE(m["body"] == "hand");
  auto it7 = m.emplace_unique(std::piecewise_construct,
                              std::forward_as_tuple("letter"),
                              std::forward_as_tuple(5, 'X'));
  EXPECT_TRUE(it7->first == "letter" && it7->second == "XXXXX");
  EXPECT_TRUE(m["letter"] == "XXXXX");
}

TEST(MapStringTest, LinearEraseOperationsBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m{
      {"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_EQ(m.size(), 3);
  EXPECT_EQ(m["A"], "1");
  EXPECT_EQ(m["B"], "2");
  EXPECT_EQ(m["C"], "3");

  size_t cnt = m.erase("B");
  EXPECT_EQ(cnt, 1);
  EXPECT_EQ(m.size(), 2);
  EXPECT_FALSE(m.contains("B"));

  auto it = m.find("A");
  m.erase(it);
  EXPECT_EQ(m.size(), 1);

  EXPECT_EQ(m.erase("X"), 0);
}

TEST(SetStringTest, LinearIteratorsBaseStringKeyWithPolicy) {
  InlineLinearFlatSet<String, 10, KeyPolicy<String>> s{"a", "z", "c", "b",
                                                       "m", "g", "q", "h"};
  std::string order;
  for (auto it = s.begin(); it != s.end(); it++) {
    order += (*it).str();
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "abcghmqz" : "azcbmgqh");

  order = "";
  for (auto it = s.rbegin(); it != s.rend(); it++) {
    order += (*it).str();
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "zqmhgcba" : "hqgmbcza");
}

TEST(SetStringTest, LinearBasicBaseStringKeyWithPolicy) {
  InlineLinearFlatSet<String, 10, KeyPolicy<String>> s{"a", "z", "c", "b",
                                                       "m", "g", "q", "h"};
  EXPECT_TRUE(s.is_static_buffer());
  EXPECT_TRUE(s.contains("a"));
  EXPECT_FALSE(s.contains("y"));
  EXPECT_TRUE(s.find("c") != s.end());
  EXPECT_TRUE(s.find("y") == s.end());
  EXPECT_TRUE(s.count("q") == 1);
  EXPECT_TRUE(s.count("y") == 0);
  EXPECT_EQ(s.erase("y"), 0);
  EXPECT_EQ(s.size(), 8);
  EXPECT_EQ(s.erase("z"), 1);
  EXPECT_EQ(s.size(), 7);
  EXPECT_FALSE(s.contains("z"));
  auto it = s.erase(s.find("g"));
  EXPECT_EQ(s.size(), 6);
  EXPECT_EQ(*it, s.is_data_ordered() ? "h" : "q");
}

TEST(MapStringTest, LinearIteratorsBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m{
      {"Z", "26"}, {"A", "1"}, {"M", "13"}};

  auto it = m.begin();
  EXPECT_EQ(it->first, m.is_data_ordered() ? "A" : "Z");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? "M" : "A");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? "Z" : "M");
  ++it;
  EXPECT_EQ(it, m.end());

  auto rit = m.rbegin();
  EXPECT_EQ(rit->first, m.is_data_ordered() ? "Z" : "M");
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? "M" : "A");
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? "A" : "Z");
  ++rit;
  EXPECT_EQ(rit, m.rend());
}

TEST(MapStringTest, LinearEdgeCasesBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m;

  m[""] = "empty_key";
  m.emplace("empty_value", "");
  EXPECT_EQ(m[""], "empty_key");
  EXPECT_EQ(m["empty_value"], "");

  std::string big_key(1000, 'K');
  std::string big_value(10000, 'V');
  m[big_key] = big_value;
  EXPECT_EQ(m[big_key].size(), 10000);
}

TEST(MapStringTest, LinearInsertOrAssignBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m;

  {
    auto [it, inserted] = m.insert_or_assign("fruit", "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.insert_or_assign("fruit", "banana");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "banana");
    EXPECT_EQ(m.size(), 1);
  }

  m.insert_or_assign("empty", "");
  EXPECT_EQ(m["empty"], "");

  auto [it, _] = m.insert_or_assign("new_key", "value");
  EXPECT_EQ(it->first, "new_key");
}

TEST(MapStringTest, LinearEmplaceOrAssignBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m;

  {
    auto [it, inserted] = m.emplace_or_assign("fruit", "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.emplace_or_assign("fruit", "banana", 4);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "bana");
    EXPECT_EQ(m["fruit"], "bana");
    EXPECT_EQ(m.size(), 1);
  }

  m.emplace_or_assign("empty", "");
  EXPECT_EQ(m["empty"], "");

  auto [it, _] = m.emplace_or_assign("new_key", "value");
  EXPECT_EQ(it->first, "new_key");
}

TEST(MapStringTest, LinearEmplacePiecewiseBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m;

  auto emp_it =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple(5, 'X'));
  ASSERT_TRUE(emp_it.second);
  EXPECT_EQ(emp_it.first->second, "XXXXX");

  m.emplace(std::piecewise_construct, std::forward_as_tuple("KKKKK", 3),
            std::forward_as_tuple(3, 'k'));
  EXPECT_EQ(m["KKK"], "kkk");

  auto emp_fail =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple("new_value"));
  EXPECT_FALSE(emp_fail.second);
  EXPECT_EQ(m["piece_key"], "XXXXX");
}

TEST(MapStringTest, LinearMixedInlineSizeBaseStringKeyWithPolicy) {
  LinearFlatMap<String, std::string, KeyPolicy<String>> m_src{
      {"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_TRUE(AssertMapContent_ABC_123(m_src));
  InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m_src2{
      {"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_TRUE(AssertMapContent_ABC_123(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatMap<String, std::string, KeyPolicy<String>> m1(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatMap<String, std::string, KeyPolicy<String>> m2(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatMap<String, std::string, 2, KeyPolicy<String>> m3(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatMap<String, std::string, 2, KeyPolicy<String>> m4(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> m5(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> m6(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatMap<String, std::string, KeyPolicy<String>> m7{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m7));

  InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m8{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatMap<String, std::string, 2, KeyPolicy<String>> m9{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> m10{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatMap<String, std::string, KeyPolicy<String>> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m12(
      std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatMap<String, std::string, 2, KeyPolicy<String>> m13(
      std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> m14(
      std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatMap<String, std::string, KeyPolicy<String>> m15{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m16{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertMapContent_ABC_123(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatMap<String, std::string, 2, KeyPolicy<String>> m17{
      {"A", "11"}, {"B", "22"}, {"C", "33"}};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertMapContent_ABC_123(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(SetStringTest, LinearEmplaceBaseStringKeyWithPolicy) {
  LinearFlatSet<String, KeyPolicy<String>> s;
  s.emplace("ABC", 2);
  s.emplace("D");
  s.insert("AB");
  EXPECT_EQ(s.size(), 2);
  EXPECT_TRUE(s.contains("AB"));
  EXPECT_TRUE(s.contains("D"));
}

TEST(SetStringTest, LinearMixedInlineSizeBaseStringKeyWithPolicy) {
  LinearFlatSet<String, KeyPolicy<String>> m_src{"A", "B", "C"};
  EXPECT_TRUE(AssertSetContent_ABC(m_src));
  InlineLinearFlatSet<String, 3, KeyPolicy<String>> m_src2{"A", "B", "C"};
  EXPECT_TRUE(AssertSetContent_ABC(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatSet<String, KeyPolicy<String>> m1(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatSet<String, KeyPolicy<String>> m2(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatSet<String, 2, KeyPolicy<String>> m3(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatSet<String, 2, KeyPolicy<String>> m4(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatSet<String, 5, KeyPolicy<String>> m5(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatSet<String, 5, KeyPolicy<String>> m6(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatSet<String, KeyPolicy<String>> m7{"a", "b", "c"};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m7));

  InlineLinearFlatSet<String, 3, KeyPolicy<String>> m8{"a", "b", "c"};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatSet<String, 2, KeyPolicy<String>> m9{"a", "b", "c"};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatSet<String, 5, KeyPolicy<String>> m10{"a", "b", "c"};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatSet<String, KeyPolicy<String>> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatSet<String, 3, KeyPolicy<String>> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatSet<String, 2, KeyPolicy<String>> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatSet<String, 5, KeyPolicy<String>> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatSet<String, KeyPolicy<String>> m15{"a", "b", "c"};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatSet<String, 3, KeyPolicy<String>> m16{"a", "b", "c"};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertSetContent_ABC(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatSet<String, 2, KeyPolicy<String>> m17{"a", "b", "c"};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertSetContent_ABC(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(LinearMap, SwapBaseStringKeyWithPolicy) {
  {
    LinearFlatMap<String, std::string, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string, KeyPolicy<String>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 2, KeyPolicy<String>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string, KeyPolicy<String>> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 2, KeyPolicy<String>> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    LinearFlatMap<String, std::string, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }

  {
    InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 5, KeyPolicy<String>> m2{
        {"a", "1"}, {"b", "2"}, {"c", "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_abc_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_abc_123(m2));
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
  }
}

TEST(LinearSet, SwapBaseStringKeyWithPolicy) {
  {
    LinearFlatSet<String, KeyPolicy<String>> m1{"A", "B", "C"};
    LinearFlatSet<String, KeyPolicy<String>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String, KeyPolicy<String>> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 2, KeyPolicy<String>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String, KeyPolicy<String>> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 5, KeyPolicy<String>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    InlineLinearFlatSet<String, 3, KeyPolicy<String>> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 5, KeyPolicy<String>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String, KeyPolicy<String>> m1{"A", "B", "C"};
    LinearFlatSet<String, KeyPolicy<String>> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String, KeyPolicy<String>> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 2, KeyPolicy<String>> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    LinearFlatSet<String, KeyPolicy<String>> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 5, KeyPolicy<String>> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }

  {
    InlineLinearFlatSet<String, 3, KeyPolicy<String>> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 5, KeyPolicy<String>> m2{"a", "b", "c"};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_abc(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_abc(m2));
    EXPECT_TRUE(AssertSetContent_ABC(m1));
  }
}

TEST(LinearMap, MergeBaseStringKeyWithPolicy) {
  {
    LinearFlatMap<String, std::string, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string, KeyPolicy<String>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatMap<String, std::string, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string, KeyPolicy<String>> m2{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    LinearFlatMap<String, std::string, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string, KeyPolicy<String>> m2{
        {"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_1232040(m1));
    EXPECT_TRUE(AssertMapContent_AC_1030(m2));
  }

  {
    InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    LinearFlatMap<String, std::string, KeyPolicy<String>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m2{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABC_123(m1));
    EXPECT_TRUE(AssertMapContent_ABC_123(m2));
  }

  {
    InlineLinearFlatMap<String, std::string, 3, KeyPolicy<String>> m1{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    InlineLinearFlatMap<String, std::string, 4, KeyPolicy<String>> m2{
        {"A", "10"}, {"b", "20"}, {"C", "30"}, {"D", "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_ABCbD_1232040(m1));
    EXPECT_TRUE(AssertMapContent_AC_1030(m2));
  }
}

TEST(LinearSet, MergeBaseStringKeyWithPolicy) {
  {
    LinearFlatSet<String, KeyPolicy<String>> m1{"A", "B", "C"};
    LinearFlatSet<String, KeyPolicy<String>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatSet<String, KeyPolicy<String>> m1{"A", "B", "C"};
    LinearFlatSet<String, KeyPolicy<String>> m2{"A", "B", "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    LinearFlatSet<String, KeyPolicy<String>> m1{"A", "B", "C"};
    LinearFlatSet<String, KeyPolicy<String>> m2{"A", "b", "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AC(m2));
  }

  {
    InlineLinearFlatSet<String, 3, KeyPolicy<String>> m1{"A", "B", "C"};
    LinearFlatSet<String, KeyPolicy<String>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_ABC(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatSet<String, 3, KeyPolicy<String>> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 3, KeyPolicy<String>> m2{"A", "B", "C"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABC(m1));
    EXPECT_TRUE(AssertSetContent_ABC(m2));
  }

  {
    InlineLinearFlatSet<String, 3, KeyPolicy<String>> m1{"A", "B", "C"};
    InlineLinearFlatSet<String, 4, KeyPolicy<String>> m2{"A", "b", "C", "D"};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_ABCbD(m1));
    EXPECT_TRUE(AssertSetContent_AC(m2));
  }
}

namespace {
template <class SetType>
bool AssertSetContent_123(const SetType& m) {
  return m.size() == 3 && m.contains(1) && m.contains(2) && m.contains(3);
}

template <class SetType>
bool AssertSetContent_n1n2n3(const SetType& m) {
  return m.size() == 3 && m.contains(-1) && m.contains(-2) && m.contains(-3);
}

template <class SetType>
bool AssertSetContent_123n24(SetType& m) {
  return m.size() == 5 && m.contains(1) && m.contains(2) && m.contains(3) &&
         m.contains(-2) && m.contains(4);
}

template <class SetType>
bool AssertMapContent_13(SetType& m) {
  return m.size() == 2 && m.contains(1) && m.contains(3);
}

template <class SetType>
bool AssertMapContent_1n234(SetType& m) {
  return m.size() == 4 && m.contains(1) && m.contains(-2) && m.contains(3) &&
         m.contains(4);
}

template <class MapType>
bool AssertMapContent_123_123(MapType& m) {
  return (m.size() == 3) && (m[1] == "1") && (m[2] == "2") && (m[3] == "3");
}

template <class MapType>
bool AssertMapContent_n1n2n3_123(MapType& m) {
  return (m.size() == 3) && (m[-1] == "1") && (m[-2] == "2") && (m[-3] == "3");
}

template <class MapType>
bool AssertMapContent_123n24_1232040(MapType& m) {
  return (m.size() == 5) && (m[1] == "1") && (m[2] == "2") && (m[3] == "3") &&
         (m[-2] == "20") && (m[4] == "40");
}

template <class MapType>
bool AssertMapContent_123n24_102302040(MapType& m) {
  return (m.size() == 5) && (m[1] == "10") && (m[2] == "2") && (m[3] == "30") &&
         (m[-2] == "20") && (m[4] == "40");
}

template <class MapType>
bool AssertMapContent_13_1030(MapType& m) {
  return (m.size() == 2) && (m[1] == "10") && (m[3] == "30");
}

template <class MapType>
bool AssertMapContent_1n234_10203040(MapType& m) {
  return (m.size() == 4) && (m[1] == "10") && (m[-2] == "20") &&
         (m[3] == "30") && (m[4] == "40");
}
}  // namespace

TEST(Vector, LinearMapInsertOrAssignIntKey) {
  InlineLinearFlatMap<int16_t, std::string, 5> map{
      {3, "c"}, {2, "b"}, {1, "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.capacity(), 5);
  EXPECT_EQ(map[1], "a");
  EXPECT_EQ(map[2], "b");
  EXPECT_EQ(map[3], "c");
  EXPECT_EQ(map[4], "");

  auto r = map.insert_or_assign(4, "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map[4], "d");

  std::string se = "e";
  auto r2 = map.insert_or_assign(5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map[5], "e");
  EXPECT_TRUE(se.empty());

  std::string sf = "f";
  auto r3 = map.insert_or_assign(6, std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map[6], "f");
  EXPECT_TRUE(sf.empty());

  std::string sg = "g";
  auto r4 = map.insert_or_assign(7, sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map[7], "g");
  EXPECT_EQ(sg, "g");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearMapEmplaceOrAssignIntKey) {
  InlineLinearFlatMap<int16_t, std::string, 5> map{
      {3, "c"}, {2, "b"}, {1, "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map[1], "a");
  EXPECT_EQ(map[2], "b");
  EXPECT_EQ(map[3], "c");
  EXPECT_EQ(map[4], "");

  auto r = map.emplace_or_assign(4, "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map[4], "d");

  std::string se = "e";
  auto r2 = map.emplace_or_assign(5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map[5], "e");
  EXPECT_TRUE(se.empty());

  std::string sf = "f";
  auto r3 = map.emplace_or_assign(6, std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map[6], "f");
  EXPECT_TRUE(sf.empty());

  std::string sg = "g";
  auto r4 = map.emplace_or_assign(7, sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map[7], "g");
  EXPECT_EQ(sg, "g");

  auto r5 = map.emplace_or_assign(7, 5, 'g');
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(map[7], "ggggg");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearSetInsertIntKey) {
  LinearFlatSet<int16_t> set{1, 2, 3};
  auto r = set.insert(3);
  EXPECT_FALSE(r.second);
  EXPECT_EQ(set.size(), 3);

  auto r2 = set.insert(4);
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(set.size(), 4);

  auto r3 = set.insert(5);
  EXPECT_TRUE(r3.second);

  EXPECT_EQ(set.size(), 5);
  EXPECT_TRUE(set.contains(1));
  EXPECT_TRUE(set.contains(2));
  EXPECT_TRUE(set.contains(3));
  EXPECT_TRUE(set.contains(4));
  EXPECT_TRUE(set.contains(5));
  EXPECT_FALSE(set.contains(6));
}

TEST(Vector, LinearMapEmplaceIntKey) {
  LinearFlatMap<int16_t, std::string> map;
  auto r = map.emplace(std::piecewise_construct, std::tuple<int16_t>(12),
                       std::tuple<const char*, size_t>("abc", 2));
  EXPECT_TRUE(r.second);
  EXPECT_EQ(r.first->first, 12);
  EXPECT_EQ(r.first->second, "ab");
  auto r2 = map.emplace(std::piecewise_construct, std::tuple<int16_t>(11),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(r2.first->first, 11);
  EXPECT_EQ(r2.first->second, "xy");

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map[12], "ab");
  EXPECT_EQ(map[11], "xy");

  auto r3 = map.emplace(std::piecewise_construct, std::forward_as_tuple(12),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, 12);
  EXPECT_EQ(r3.first->second, "ab");

  EXPECT_EQ(map.size(), 2);

  auto r4 = map.try_emplace(11, "ab");
  EXPECT_FALSE(r4.second);
  EXPECT_EQ(r4.first->first, 11);
  EXPECT_EQ(r4.first->second, "xy");

  std::string sXYZ = "xyz";
  auto r5 = map.try_emplace(11, std::move(sXYZ));
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(r5.first->first, 11);
  EXPECT_EQ(r5.first->second, "xy");
  EXPECT_EQ(sXYZ, "xyz");

  auto r6 = map.try_emplace(13, std::move(sXYZ));
  EXPECT_TRUE(r6.second);
  EXPECT_EQ(r6.first->first, 13);
  EXPECT_EQ(r6.first->second, "xyz");
  EXPECT_TRUE(sXYZ.empty());

  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map[12], "ab");
  EXPECT_EQ(map[11], "xy");
  EXPECT_EQ(map[13], "xyz");

  std::string sUVW = "uvw";
  auto r7 = map.try_emplace(14, sUVW);
  EXPECT_TRUE(r7.second);
  EXPECT_EQ(r7.first->first, 14);
  EXPECT_EQ(r7.first->second, "uvw");
  EXPECT_EQ(sUVW, "uvw");

  EXPECT_EQ(map.size(), 4);
  EXPECT_EQ(map[12], "ab");
  EXPECT_EQ(map[11], "xy");
  EXPECT_EQ(map[13], "xyz");
  EXPECT_EQ(map[14], "uvw");
}

TEST(MapIntTest, LinearBasicOperationsIntKey) {
  LinearFlatMap<int8_t, std::string> m;

  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.size(), 0);

  auto ret = m.insert({99, "red"});
  ASSERT_TRUE(ret.second);
  EXPECT_EQ(ret.first->first, 99);
  EXPECT_EQ(ret.first->second, "red");
  EXPECT_EQ(m.size(), 1);
}

TEST(MapIntTest, LinearElementAccessIntKey) {
  LinearFlatMap<int32_t, std::string> m{{99, "red"}, {100, "yellow"}};

  EXPECT_EQ(m[99], "red");

  m[99] = "green";
  EXPECT_EQ(m[99], "green");
  EXPECT_EQ(m.at(99), "green");

  EXPECT_EQ(m[101], "");
  EXPECT_EQ(m.at(101), "");
  EXPECT_EQ(m.size(), 3);

  EXPECT_EQ(m.at(102), "");
  m.at(102) = "orange";
  EXPECT_EQ(m[102], "orange");
  EXPECT_EQ(m.size(), 4);

  int32_t i103 = 103;
  m.at(std::move(i103)) = "green";
  EXPECT_EQ(m.find(103)->first, 103);
  EXPECT_EQ(m.find(103)->second, "green");
  EXPECT_EQ(m.size(), 5);

  int32_t i105 = 105;
  auto r = m.insert_default_if_absent(std::move(i105));
  EXPECT_TRUE(r.second);
  r.first->second = "yellow";
  EXPECT_EQ(m[105], "yellow");
  EXPECT_EQ(m.size(), 6);

  auto r2 = m.insert_default_if_absent(99);
  EXPECT_FALSE(r2.second);
  EXPECT_EQ(m[99], "green");
  r2.first->second = "red";
  EXPECT_EQ(m[99], "red");
  EXPECT_EQ(m.size(), 6);

  std::string black = "black";
  auto r3 = m.insert_if_absent(99, black);
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, 99);
  EXPECT_EQ(r3.first->second, "red");
  EXPECT_EQ(m.size(), 6);

  std::string pink = "pink";
  auto r4 = m.insert_if_absent(200, std::move(pink));
  EXPECT_TRUE(r4.second);
  EXPECT_TRUE(pink.empty());
  EXPECT_EQ(r4.first->first, 200);
  EXPECT_EQ(r4.first->second, "pink");
  EXPECT_EQ(m.size(), 7);

  auto r5 = m.insert_if_absent(300, black);
  EXPECT_TRUE(r5.second);
  EXPECT_EQ(r5.first->first, 300);
  EXPECT_EQ(r5.first->second, "black");
  EXPECT_EQ(black, "black");
  EXPECT_EQ(m.size(), 8);
}

TEST(MapIntTest, LinearInsertUpdateIntKey) {
  LinearFlatMap<int16_t, std::string> m;

  auto ret1 = m.insert({99, "apple"});
  EXPECT_TRUE(ret1.second);
  auto ret2 = m.insert({99, "banana"});
  EXPECT_FALSE(ret2.second);
  EXPECT_EQ(ret2.first->second, "apple");

  auto emp_ret = m.emplace(100, "blue");
  EXPECT_TRUE(emp_ret.second);
  EXPECT_EQ(emp_ret.first->first, 100);

  m[100] = "red";
  EXPECT_EQ(m[100], "red");
}

TEST(MapIntTest, LinearEraseOperationsIntKey) {
  LinearFlatMap<int16_t, std::string> m{{30, "1"}, {31, "2"}, {32, "3"}};
  EXPECT_EQ(m.size(), 3);
  EXPECT_EQ(m[30], "1");
  EXPECT_EQ(m[31], "2");
  EXPECT_EQ(m[32], "3");

  size_t cnt = m.erase(31);
  EXPECT_EQ(cnt, 1);
  EXPECT_EQ(m.size(), 2);
  EXPECT_FALSE(m.contains(31));

  auto it = m.find(30);
  m.erase(it);
  EXPECT_EQ(m.size(), 1);
  EXPECT_FALSE(m.contains(30));

  EXPECT_EQ(m.erase(100), 0);
}

TEST(SetIntTest, LinearIteratorsIntKey) {
  InlineLinearFlatSet<int32_t, 10> s{5, 4, 9, 0, 1, 8, 2, 7};
  std::string order;
  for (auto it = s.begin(); it != s.end(); it++) {
    order += std::to_string((*it));
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "01245789" : "54901827");

  order = "";
  for (auto it = s.rbegin(); it != s.rend(); it++) {
    order += std::to_string((*it));
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "98754210" : "72810945");
}

TEST(SetIntTest, LinearBasicIntKey) {
  {
    InlineLinearFlatSet<int8_t, 10> s{5, 4, 9, 0, 1, 8, 2, 7};
    EXPECT_TRUE(s.is_static_buffer());
    EXPECT_TRUE(s.contains(5));
    EXPECT_FALSE(s.contains(3));
    EXPECT_TRUE(s.find(9) != s.end());
    EXPECT_TRUE(s.find(3) == s.end());
    EXPECT_TRUE(s.count(2) == 1);
    EXPECT_TRUE(s.count(3) == 0);
    EXPECT_EQ(s.erase(3), 0);
    EXPECT_EQ(s.size(), 8);
    EXPECT_EQ(s.erase(4), 1);
    EXPECT_EQ(s.size(), 7);
    EXPECT_FALSE(s.contains(4));
    auto it = s.erase(s.find(8));
    EXPECT_EQ(s.size(), 6);
    EXPECT_EQ(*it, s.is_data_ordered() ? 9 : 2);
  }
  {
    InlineLinearFlatSet<int16_t, 10> s{5, 4, 9, 0, 1, 8, 2, 7};
    EXPECT_TRUE(s.is_static_buffer());
    EXPECT_TRUE(s.contains(5));
    EXPECT_FALSE(s.contains(3));
    EXPECT_TRUE(s.find(9) != s.end());
    EXPECT_TRUE(s.find(3) == s.end());
    EXPECT_TRUE(s.count(2) == 1);
    EXPECT_TRUE(s.count(3) == 0);
    EXPECT_EQ(s.erase(3), 0);
    EXPECT_EQ(s.size(), 8);
    EXPECT_EQ(s.erase(4), 1);
    EXPECT_EQ(s.size(), 7);
    EXPECT_FALSE(s.contains(4));
    auto it = s.erase(s.find(8));
    EXPECT_EQ(s.size(), 6);
    EXPECT_EQ(*it, s.is_data_ordered() ? 9 : 2);
  }
  {
    InlineLinearFlatSet<int32_t, 10> s{5, 4, 9, 0, 1, 8, 2, 7};
    EXPECT_TRUE(s.is_static_buffer());
    EXPECT_TRUE(s.contains(5));
    EXPECT_FALSE(s.contains(3));
    EXPECT_TRUE(s.find(9) != s.end());
    EXPECT_TRUE(s.find(3) == s.end());
    EXPECT_TRUE(s.count(2) == 1);
    EXPECT_TRUE(s.count(3) == 0);
    EXPECT_EQ(s.erase(3), 0);
    EXPECT_EQ(s.size(), 8);
    EXPECT_EQ(s.erase(4), 1);
    EXPECT_EQ(s.size(), 7);
    EXPECT_FALSE(s.contains(4));
    auto it = s.erase(s.find(8));
    EXPECT_EQ(s.size(), 6);
    EXPECT_EQ(*it, s.is_data_ordered() ? 9 : 2);
  }
}

TEST(MapIntTest, LinearIteratorsIntKey) {
  LinearFlatMap<int32_t, std::string> m{{26, "26"}, {1, "1"}, {13, "13"}};

  auto it = m.begin();
  EXPECT_EQ(it->first, m.is_data_ordered() ? 1 : 26);
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? 13 : 1);
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? 26 : 13);
  ++it;
  EXPECT_EQ(it, m.end());

  auto rit = m.rbegin();
  EXPECT_EQ(rit->first, m.is_data_ordered() ? 26 : 13);
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? 13 : 1);
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? 1 : 26);
  ++rit;
  EXPECT_EQ(rit, m.rend());
}

TEST(MapIntTest, LinearInsertOrAssignIntKey) {
  LinearFlatMap<int16_t, std::string> m;

  {
    auto [it, inserted] = m.insert_or_assign(10, "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.insert_or_assign(10, "banana");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "banana");
    EXPECT_EQ(m.size(), 1);
  }

  m.insert_or_assign(11, "11");
  EXPECT_EQ(m[11], "11");

  auto [it, _] = m.insert_or_assign(12, "orange");
  EXPECT_EQ(it->first, 12);
}

TEST(MapIntTest, LinearEmplaceOrAssignIntKey) {
  LinearFlatMap<int16_t, std::string> m;

  {
    auto [it, inserted] = m.emplace_or_assign(10, "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.emplace_or_assign(10, "banana", 4);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "bana");
    EXPECT_EQ(m[10], "bana");
    EXPECT_EQ(m.size(), 1);
  }

  m.emplace_or_assign(11, "11");
  EXPECT_EQ(m[11], "11");

  auto [it, _] = m.emplace_or_assign(12, "orange");
  EXPECT_EQ(it->first, 12);
}

TEST(MapIntTest, LinearEmplacePiecewiseIntKey) {
  LinearFlatMap<uint32_t, std::string> m;

  auto emp_it = m.emplace(std::piecewise_construct, std::forward_as_tuple(99u),
                          std::forward_as_tuple(5, 'X'));
  ASSERT_TRUE(emp_it.second);
  EXPECT_EQ(emp_it.first->second, "XXXXX");

  m.emplace(std::piecewise_construct, std::forward_as_tuple(199u),
            std::forward_as_tuple(3, 'k'));
  EXPECT_EQ(m[199], "kkk");

  auto emp_fail = m.emplace(std::piecewise_construct, std::forward_as_tuple(99),
                            std::forward_as_tuple("new_value"));
  EXPECT_FALSE(emp_fail.second);
  EXPECT_EQ(m[99], "XXXXX");
}

TEST(MapIntTest, LinearMixedInlineSizeIntKey) {
  LinearFlatMap<int8_t, std::string> m_src{{1, "1"}, {2, "2"}, {3, "3"}};
  EXPECT_TRUE(AssertMapContent_123_123(m_src));
  InlineLinearFlatMap<int8_t, std::string, 3> m_src2{
      {1, "1"}, {2, "2"}, {3, "3"}};
  EXPECT_TRUE(AssertMapContent_123_123(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatMap<int8_t, std::string> m1(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatMap<int8_t, std::string> m2(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatMap<int8_t, std::string, 2> m3(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatMap<int8_t, std::string, 2> m4(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatMap<int8_t, std::string, 5> m5(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatMap<int8_t, std::string, 5> m6(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatMap<int8_t, std::string> m7{{1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m7));

  InlineLinearFlatMap<int8_t, std::string, 3> m8{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatMap<int8_t, std::string, 2> m9{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatMap<int8_t, std::string, 5> m10{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatMap<int8_t, std::string> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatMap<int8_t, std::string, 3> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatMap<int8_t, std::string, 2> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatMap<int8_t, std::string, 5> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatMap<int8_t, std::string> m15{{1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatMap<int8_t, std::string, 3> m16{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatMap<int8_t, std::string, 2> m17{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(SetIntTest, LinearEmplaceIntKey) {
  LinearFlatSet<int32_t> s;
  s.emplace(9);
  s.emplace(8);
  s.insert(9);
  EXPECT_EQ(s.size(), 2);
  EXPECT_TRUE(s.contains(9));
  EXPECT_TRUE(s.contains(8));
}

TEST(SetIntTest, LinearMixedInlineSizeIntKey) {
  LinearFlatSet<int16_t> m_src{1, 2, 3};
  EXPECT_TRUE(AssertSetContent_123(m_src));
  InlineLinearFlatSet<int16_t, 3> m_src2{1, 2, 3};
  EXPECT_TRUE(AssertSetContent_123(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatSet<int16_t> m1(m_src);
  EXPECT_TRUE(AssertSetContent_123(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatSet<int16_t> m2(m_src2);
  EXPECT_TRUE(AssertSetContent_123(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatSet<int16_t, 2> m3(m_src);
  EXPECT_TRUE(AssertSetContent_123(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatSet<int16_t, 2> m4(m_src2);
  EXPECT_TRUE(AssertSetContent_123(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatSet<int16_t, 5> m5(m_src);
  EXPECT_TRUE(AssertSetContent_123(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatSet<int16_t, 5> m6(m_src2);
  EXPECT_TRUE(AssertSetContent_123(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatSet<int16_t> m7{21, 22, 23};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m7));

  InlineLinearFlatSet<int16_t, 3> m8{21, 22, 23};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatSet<int16_t, 2> m9{21, 22, 23};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatSet<int16_t, 5> m10{21, 22, 23};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatSet<int16_t> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatSet<int16_t, 3> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatSet<int16_t, 2> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatSet<int16_t, 5> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatSet<int16_t> m15{21, 22, 23};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatSet<int16_t, 3> m16{21, 22, 23};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertSetContent_123(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatSet<int16_t, 2> m17{21, 22, 23};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertSetContent_123(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(LinearMap, SwapIntKey) {
  {
    LinearFlatMap<int32_t, std::string> m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<int32_t, std::string> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string> m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 2> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string> m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    InlineLinearFlatMap<int32_t, std::string, 3> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string> m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<int32_t, std::string> m2{{-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string> m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 2> m2{
        {-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string> m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5> m2{
        {-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    InlineLinearFlatMap<int32_t, std::string, 3> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5> m2{
        {-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }
}

TEST(LinearSet, SwapIntKey) {
  {
    LinearFlatSet<int> m1{1, 2, 3};
    LinearFlatSet<int> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int> m1{1, 2, 3};
    InlineLinearFlatSet<int, 2> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int> m1{1, 2, 3};
    InlineLinearFlatSet<int, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    InlineLinearFlatSet<int, 3> m1{1, 2, 3};
    InlineLinearFlatSet<int, 5> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int> m1{1, 2, 3};
    LinearFlatSet<int> m2{-1, -2, -3};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int> m1{1, 2, 3};
    InlineLinearFlatSet<int, 2> m2{-1, -2, -3};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int> m1{1, 2, 3};
    InlineLinearFlatSet<int, 5> m2{-1, -2, -3};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    InlineLinearFlatSet<int, 3> m1{1, 2, 3};
    InlineLinearFlatSet<int, 5> m2{-1, -2, -3};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }
}

TEST(LinearMap, MergeIntKey) {
  {
    LinearFlatMap<uint8_t, std::string> m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatMap<uint8_t, std::string> m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string> m2{{1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    LinearFlatMap<uint8_t, std::string> m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string> m2{
        {1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_1232040(m1));
    EXPECT_TRUE(AssertMapContent_13_1030(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 3> m2{
        {1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 4> m2{
        {1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_1232040(m1));
    EXPECT_TRUE(AssertMapContent_13_1030(m2));
  }
}

TEST(LinearMap, MergeAssignIntKey) {
  {
    LinearFlatMap<uint8_t, std::string, MergeAssignKeyPolicy<uint8_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string, MergeAssignKeyPolicy<uint8_t>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<uint8_t, std::string, MergeAssignKeyPolicy<uint8_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string, MergeAssignKeyPolicy<uint8_t>> m2{
        {1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    LinearFlatMap<uint8_t, std::string, MergeAssignKeyPolicy<uint8_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string, MergeAssignKeyPolicy<uint8_t>> m2{
        {1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_102302040(m1));
    EXPECT_TRUE(AssertMapContent_1n234_10203040(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3, MergeAssignKeyPolicy<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string, MergeAssignKeyPolicy<uint8_t>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3, MergeAssignKeyPolicy<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 3, MergeAssignKeyPolicy<uint8_t>>
        m2{{1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3, MergeAssignKeyPolicy<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 4, MergeAssignKeyPolicy<uint8_t>>
        m2{{1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_102302040(m1));
    EXPECT_TRUE(AssertMapContent_1n234_10203040(m2));
  }
}

TEST(LinearSet, MergeIntKey) {
  {
    LinearFlatSet<int8_t> m1{1, 2, 3};
    LinearFlatSet<int8_t> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatSet<int8_t> m1{1, 2, 3};
    LinearFlatSet<int8_t> m2{1, 2, 3};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));
  }

  {
    LinearFlatSet<int8_t> m1{1, 2, 3};
    LinearFlatSet<int8_t> m2{1, -2, 3, 4};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123n24(m1));
    EXPECT_TRUE(AssertMapContent_13(m2));
  }

  {
    InlineLinearFlatSet<int8_t, 3> m1{1, 2, 3};
    LinearFlatSet<int8_t> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatSet<int8_t, 3> m1{1, 2, 3};
    InlineLinearFlatSet<int8_t, 3> m2{1, 2, 3};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));
  }

  {
    InlineLinearFlatSet<int8_t, 3> m1{1, 2, 3};
    InlineLinearFlatSet<int8_t, 4> m2{1, -2, 3, 4};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123n24(m1));
    EXPECT_TRUE(AssertMapContent_13(m2));
  }
}

TEST(LinearSet, MergeAssignIntKey) {
  {
    LinearFlatSet<int8_t, MergeAssignKeyPolicy<int8_t>> m1{1, 2, 3};
    LinearFlatSet<int8_t, MergeAssignKeyPolicy<int8_t>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_123(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int8_t, MergeAssignKeyPolicy<int8_t>> m1{1, 2, 3};
    LinearFlatSet<int8_t, MergeAssignKeyPolicy<int8_t>> m2{1, 2, 3};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));
  }

  {
    LinearFlatSet<int8_t, MergeAssignKeyPolicy<int8_t>> m1{1, 2, 3};
    LinearFlatSet<int8_t, MergeAssignKeyPolicy<int8_t>> m2{1, -2, 3, 4};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123n24(m1));
    EXPECT_TRUE(AssertMapContent_1n234(m2));
  }

  {
    InlineLinearFlatSet<int8_t, 3, MergeAssignKeyPolicy<int8_t>> m1{1, 2, 3};
    LinearFlatSet<int8_t, MergeAssignKeyPolicy<int8_t>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_123(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    InlineLinearFlatSet<int8_t, 3, MergeAssignKeyPolicy<int8_t>> m1{1, 2, 3};
    InlineLinearFlatSet<int8_t, 3, MergeAssignKeyPolicy<int8_t>> m2{1, 2, 3};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));
  }

  {
    InlineLinearFlatSet<int8_t, 3, MergeAssignKeyPolicy<int8_t>> m1{1, 2, 3};
    InlineLinearFlatSet<int8_t, 4, MergeAssignKeyPolicy<int8_t>> m2{1, -2, 3,
                                                                    4};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123n24(m1));
    EXPECT_TRUE(AssertMapContent_1n234(m2));
  }
}

TEST(Vector, LinearMapInsertOrAssignIntKeyWithPolicy) {
  InlineLinearFlatMap<int16_t, std::string, 5, KeyPolicy<int16_t>> map{
      {3, "c"}, {2, "b"}, {1, "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.capacity(), 5);
  EXPECT_EQ(map[1], "a");
  EXPECT_EQ(map[2], "b");
  EXPECT_EQ(map[3], "c");
  EXPECT_EQ(map[4], "");

  auto r = map.insert_or_assign(4, "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map[4], "d");

  std::string se = "e";
  auto r2 = map.insert_or_assign(5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map[5], "e");
  EXPECT_TRUE(se.empty());

  std::string sf = "f";
  auto r3 = map.insert_or_assign(6, std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map[6], "f");
  EXPECT_TRUE(sf.empty());

  std::string sg = "g";
  auto r4 = map.insert_or_assign(7, sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map[7], "g");
  EXPECT_EQ(sg, "g");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearMapEmplaceOrAssignIntKeyWithPolicy) {
  InlineLinearFlatMap<int16_t, std::string, 5, KeyPolicy<int16_t>> map{
      {3, "c"}, {2, "b"}, {1, "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map[1], "a");
  EXPECT_EQ(map[2], "b");
  EXPECT_EQ(map[3], "c");
  EXPECT_EQ(map[4], "");

  auto r = map.emplace_or_assign(4, "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map[4], "d");

  std::string se = "e";
  auto r2 = map.emplace_or_assign(5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map[5], "e");
  EXPECT_TRUE(se.empty());

  std::string sf = "f";
  auto r3 = map.emplace_or_assign(6, std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map[6], "f");
  EXPECT_TRUE(sf.empty());

  std::string sg = "g";
  auto r4 = map.emplace_or_assign(7, sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map[7], "g");
  EXPECT_EQ(sg, "g");

  auto r5 = map.emplace_or_assign(7, 5, 'g');
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(map[7], "ggggg");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearSetInsertIntKeyWithPolicy) {
  LinearFlatSet<int16_t, KeyPolicy<int16_t>> set{1, 2, 3};
  auto r = set.insert(3);
  EXPECT_FALSE(r.second);
  EXPECT_EQ(set.size(), 3);

  auto r2 = set.insert(4);
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(set.size(), 4);

  auto r3 = set.insert(5);
  EXPECT_TRUE(r3.second);

  EXPECT_EQ(set.size(), 5);
  EXPECT_TRUE(set.contains(1));
  EXPECT_TRUE(set.contains(2));
  EXPECT_TRUE(set.contains(3));
  EXPECT_TRUE(set.contains(4));
  EXPECT_TRUE(set.contains(5));
  EXPECT_FALSE(set.contains(6));
}

TEST(Vector, LinearMapEmplaceIntKeyWithPolicy) {
  LinearFlatMap<int16_t, std::string, KeyPolicy<int16_t>> map;
  auto r = map.emplace(std::piecewise_construct, std::tuple<int16_t>(12),
                       std::tuple<const char*, size_t>("abc", 2));
  EXPECT_TRUE(r.second);
  EXPECT_EQ(r.first->first, 12);
  EXPECT_EQ(r.first->second, "ab");
  auto r2 = map.emplace(std::piecewise_construct, std::tuple<int16_t>(11),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(r2.first->first, 11);
  EXPECT_EQ(r2.first->second, "xy");

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map[12], "ab");
  EXPECT_EQ(map[11], "xy");

  auto r3 = map.emplace(std::piecewise_construct, std::forward_as_tuple(12),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, 12);
  EXPECT_EQ(r3.first->second, "ab");

  EXPECT_EQ(map.size(), 2);

  auto r4 = map.try_emplace(11, "ab");
  EXPECT_FALSE(r4.second);
  EXPECT_EQ(r4.first->first, 11);
  EXPECT_EQ(r4.first->second, "xy");

  std::string sXYZ = "xyz";
  auto r5 = map.try_emplace(11, std::move(sXYZ));
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(r5.first->first, 11);
  EXPECT_EQ(r5.first->second, "xy");
  EXPECT_EQ(sXYZ, "xyz");

  auto r6 = map.try_emplace(13, std::move(sXYZ));
  EXPECT_TRUE(r6.second);
  EXPECT_EQ(r6.first->first, 13);
  EXPECT_EQ(r6.first->second, "xyz");
  EXPECT_TRUE(sXYZ.empty());

  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map[12], "ab");
  EXPECT_EQ(map[11], "xy");
  EXPECT_EQ(map[13], "xyz");

  std::string sUVW = "uvw";
  auto r7 = map.try_emplace(14, sUVW);
  EXPECT_TRUE(r7.second);
  EXPECT_EQ(r7.first->first, 14);
  EXPECT_EQ(r7.first->second, "uvw");
  EXPECT_EQ(sUVW, "uvw");

  EXPECT_EQ(map.size(), 4);
  EXPECT_EQ(map[12], "ab");
  EXPECT_EQ(map[11], "xy");
  EXPECT_EQ(map[13], "xyz");
  EXPECT_EQ(map[14], "uvw");
}

TEST(MapIntTest, LinearBasicOperationsIntKeyWithPolicy) {
  LinearFlatMap<int8_t, std::string, KeyPolicy<int8_t>> m;

  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.size(), 0);

  auto ret = m.insert({99, "red"});
  ASSERT_TRUE(ret.second);
  EXPECT_EQ(ret.first->first, 99);
  EXPECT_EQ(ret.first->second, "red");
  EXPECT_EQ(m.size(), 1);
}

TEST(MapIntTest, LinearElementAccessIntKeyWithPolicy) {
  LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m{{99, "red"},
                                                            {100, "yellow"}};

  EXPECT_EQ(m[99], "red");

  m[99] = "green";
  EXPECT_EQ(m[99], "green");
  EXPECT_EQ(m.at(99), "green");

  EXPECT_EQ(m[101], "");
  EXPECT_EQ(m.at(101), "");
  EXPECT_EQ(m.size(), 3);

  EXPECT_EQ(m.at(102), "");
  m.at(102) = "orange";
  EXPECT_EQ(m[102], "orange");
  EXPECT_EQ(m.size(), 4);

  int32_t i103 = 103;
  m.at(std::move(i103)) = "green";
  EXPECT_EQ(m.find(103)->first, 103);
  EXPECT_EQ(m.find(103)->second, "green");
  EXPECT_EQ(m.size(), 5);

  int32_t i105 = 105;
  auto r = m.insert_default_if_absent(std::move(i105));
  EXPECT_TRUE(r.second);
  r.first->second = "yellow";
  EXPECT_EQ(m[105], "yellow");
  EXPECT_EQ(m.size(), 6);

  auto r2 = m.insert_default_if_absent(99);
  EXPECT_FALSE(r2.second);
  EXPECT_EQ(m[99], "green");
  r2.first->second = "red";
  EXPECT_EQ(m[99], "red");
  EXPECT_EQ(m.size(), 6);

  std::string black = "black";
  auto r3 = m.insert_if_absent(99, black);
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(r3.first->first, 99);
  EXPECT_EQ(r3.first->second, "red");
  EXPECT_EQ(m.size(), 6);

  std::string pink = "pink";
  auto r4 = m.insert_if_absent(200, std::move(pink));
  EXPECT_TRUE(r4.second);
  EXPECT_TRUE(pink.empty());
  EXPECT_EQ(r4.first->first, 200);
  EXPECT_EQ(r4.first->second, "pink");
  EXPECT_EQ(m.size(), 7);

  auto r5 = m.insert_if_absent(300, black);
  EXPECT_TRUE(r5.second);
  EXPECT_EQ(r5.first->first, 300);
  EXPECT_EQ(r5.first->second, "black");
  EXPECT_EQ(black, "black");
  EXPECT_EQ(m.size(), 8);
}

TEST(MapIntTest, LinearInsertUpdateIntKeyWithPolicy) {
  LinearFlatMap<int16_t, std::string, KeyPolicy<int16_t>> m;

  auto ret1 = m.insert({99, "apple"});
  EXPECT_TRUE(ret1.second);
  auto ret2 = m.insert({99, "banana"});
  EXPECT_FALSE(ret2.second);
  EXPECT_EQ(ret2.first->second, "apple");

  auto emp_ret = m.emplace(100, "blue");
  EXPECT_TRUE(emp_ret.second);
  EXPECT_EQ(emp_ret.first->first, 100);

  m[100] = "red";
  EXPECT_EQ(m[100], "red");
}

TEST(MapIntTest, LinearEraseOperationsIntKeyWithPolicy) {
  LinearFlatMap<int16_t, std::string, KeyPolicy<int16_t>> m{
      {30, "1"}, {31, "2"}, {32, "3"}};
  EXPECT_EQ(m.size(), 3);
  EXPECT_EQ(m[30], "1");
  EXPECT_EQ(m[31], "2");
  EXPECT_EQ(m[32], "3");

  size_t cnt = m.erase(31);
  EXPECT_EQ(cnt, 1);
  EXPECT_EQ(m.size(), 2);
  EXPECT_FALSE(m.contains(31));

  auto it = m.find(30);
  m.erase(it);
  EXPECT_EQ(m.size(), 1);
  EXPECT_FALSE(m.contains(30));

  EXPECT_EQ(m.erase(100), 0);
}

TEST(SetIntTest, LinearIteratorsIntKeyWithPolicy) {
  InlineLinearFlatSet<int32_t, 10, KeyPolicy<int32_t>> s{5, 4, 9, 0,
                                                         1, 8, 2, 7};
  std::string order;
  for (auto it = s.begin(); it != s.end(); it++) {
    order += std::to_string((*it));
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "01245789" : "54901827");

  order = "";
  for (auto it = s.rbegin(); it != s.rend(); it++) {
    order += std::to_string((*it));
  }
  EXPECT_EQ(order, s.is_data_ordered() ? "98754210" : "72810945");
}

TEST(SetIntTest, LinearBasicIntKeyWithPolicy) {
  {
    InlineLinearFlatSet<int8_t, 10, KeyPolicy<int8_t>> s{5, 4, 9, 0,
                                                         1, 8, 2, 7};
    EXPECT_TRUE(s.is_static_buffer());
    EXPECT_TRUE(s.contains(5));
    EXPECT_FALSE(s.contains(3));
    EXPECT_TRUE(s.find(9) != s.end());
    EXPECT_TRUE(s.find(3) == s.end());
    EXPECT_TRUE(s.count(2) == 1);
    EXPECT_TRUE(s.count(3) == 0);
    EXPECT_EQ(s.erase(3), 0);
    EXPECT_EQ(s.size(), 8);
    EXPECT_EQ(s.erase(4), 1);
    EXPECT_EQ(s.size(), 7);
    EXPECT_FALSE(s.contains(4));
    auto it = s.erase(s.find(8));
    EXPECT_EQ(s.size(), 6);
    EXPECT_EQ(*it, s.is_data_ordered() ? 9 : 2);
  }
  {
    InlineLinearFlatSet<int16_t, 10, KeyPolicy<int16_t>> s{5, 4, 9, 0,
                                                           1, 8, 2, 7};
    EXPECT_TRUE(s.is_static_buffer());
    EXPECT_TRUE(s.contains(5));
    EXPECT_FALSE(s.contains(3));
    EXPECT_TRUE(s.find(9) != s.end());
    EXPECT_TRUE(s.find(3) == s.end());
    EXPECT_TRUE(s.count(2) == 1);
    EXPECT_TRUE(s.count(3) == 0);
    EXPECT_EQ(s.erase(3), 0);
    EXPECT_EQ(s.size(), 8);
    EXPECT_EQ(s.erase(4), 1);
    EXPECT_EQ(s.size(), 7);
    EXPECT_FALSE(s.contains(4));
    auto it = s.erase(s.find(8));
    EXPECT_EQ(s.size(), 6);
    EXPECT_EQ(*it, s.is_data_ordered() ? 9 : 2);
  }
  {
    InlineLinearFlatSet<int32_t, 10, KeyPolicy<int32_t>> s{5, 4, 9, 0,
                                                           1, 8, 2, 7};
    EXPECT_TRUE(s.is_static_buffer());
    EXPECT_TRUE(s.contains(5));
    EXPECT_FALSE(s.contains(3));
    EXPECT_TRUE(s.find(9) != s.end());
    EXPECT_TRUE(s.find(3) == s.end());
    EXPECT_TRUE(s.count(2) == 1);
    EXPECT_TRUE(s.count(3) == 0);
    EXPECT_EQ(s.erase(3), 0);
    EXPECT_EQ(s.size(), 8);
    EXPECT_EQ(s.erase(4), 1);
    EXPECT_EQ(s.size(), 7);
    EXPECT_FALSE(s.contains(4));
    auto it = s.erase(s.find(8));
    EXPECT_EQ(s.size(), 6);
    EXPECT_EQ(*it, s.is_data_ordered() ? 9 : 2);
  }
}

TEST(MapIntTest, LinearIteratorsIntKeyWithPolicy) {
  LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m{
      {26, "26"}, {1, "1"}, {13, "13"}};

  auto it = m.begin();
  EXPECT_EQ(it->first, m.is_data_ordered() ? 1 : 26);
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? 13 : 1);
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? 26 : 13);
  ++it;
  EXPECT_EQ(it, m.end());

  auto rit = m.rbegin();
  EXPECT_EQ(rit->first, m.is_data_ordered() ? 26 : 13);
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? 13 : 1);
  ++rit;
  EXPECT_EQ(rit->first, m.is_data_ordered() ? 1 : 26);
  ++rit;
  EXPECT_EQ(rit, m.rend());
}

TEST(MapIntTest, LinearInsertOrAssignIntKeyWithPolicy) {
  LinearFlatMap<int16_t, std::string, KeyPolicy<int16_t>> m;

  {
    auto [it, inserted] = m.insert_or_assign(10, "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.insert_or_assign(10, "banana");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "banana");
    EXPECT_EQ(m.size(), 1);
  }

  m.insert_or_assign(11, "11");
  EXPECT_EQ(m[11], "11");

  auto [it, _] = m.insert_or_assign(12, "orange");
  EXPECT_EQ(it->first, 12);
}

TEST(MapIntTest, LinearEmplaceOrAssignIntKeyWithPolicy) {
  LinearFlatMap<int16_t, std::string, KeyPolicy<int16_t>> m;

  {
    auto [it, inserted] = m.emplace_or_assign(10, "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->second, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.emplace_or_assign(10, "banana", 4);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, "bana");
    EXPECT_EQ(m[10], "bana");
    EXPECT_EQ(m.size(), 1);
  }

  m.emplace_or_assign(11, "11");
  EXPECT_EQ(m[11], "11");

  auto [it, _] = m.emplace_or_assign(12, "orange");
  EXPECT_EQ(it->first, 12);
}

TEST(MapIntTest, LinearEmplacePiecewiseIntKeyWithPolicy) {
  LinearFlatMap<uint32_t, std::string, KeyPolicy<uint32_t>> m;

  auto emp_it = m.emplace(std::piecewise_construct, std::forward_as_tuple(99u),
                          std::forward_as_tuple(5, 'X'));
  ASSERT_TRUE(emp_it.second);
  EXPECT_EQ(emp_it.first->second, "XXXXX");

  m.emplace(std::piecewise_construct, std::forward_as_tuple(199u),
            std::forward_as_tuple(3, 'k'));
  EXPECT_EQ(m[199], "kkk");

  auto emp_fail = m.emplace(std::piecewise_construct, std::forward_as_tuple(99),
                            std::forward_as_tuple("new_value"));
  EXPECT_FALSE(emp_fail.second);
  EXPECT_EQ(m[99], "XXXXX");
}

TEST(MapIntTest, LinearMixedInlineSizeIntKeyWithPolicy) {
  LinearFlatMap<int8_t, std::string, KeyPolicy<int8_t>> m_src{
      {1, "1"}, {2, "2"}, {3, "3"}};
  EXPECT_TRUE(AssertMapContent_123_123(m_src));
  InlineLinearFlatMap<int8_t, std::string, 3, KeyPolicy<int8_t>> m_src2{
      {1, "1"}, {2, "2"}, {3, "3"}};
  EXPECT_TRUE(AssertMapContent_123_123(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatMap<int8_t, std::string, KeyPolicy<int8_t>> m1(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatMap<int8_t, std::string, KeyPolicy<int8_t>> m2(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatMap<int8_t, std::string, 2, KeyPolicy<int8_t>> m3(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatMap<int8_t, std::string, 2, KeyPolicy<int8_t>> m4(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatMap<int8_t, std::string, 5, KeyPolicy<int8_t>> m5(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatMap<int8_t, std::string, 5, KeyPolicy<int8_t>> m6(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatMap<int8_t, std::string, KeyPolicy<int8_t>> m7{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m7));

  InlineLinearFlatMap<int8_t, std::string, 3, KeyPolicy<int8_t>> m8{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatMap<int8_t, std::string, 2, KeyPolicy<int8_t>> m9{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatMap<int8_t, std::string, 5, KeyPolicy<int8_t>> m10{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatMap<int8_t, std::string, KeyPolicy<int8_t>> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatMap<int8_t, std::string, 3, KeyPolicy<int8_t>> m12(
      std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatMap<int8_t, std::string, 2, KeyPolicy<int8_t>> m13(
      std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatMap<int8_t, std::string, 5, KeyPolicy<int8_t>> m14(
      std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatMap<int8_t, std::string, KeyPolicy<int8_t>> m15{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatMap<int8_t, std::string, 3, KeyPolicy<int8_t>> m16{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatMap<int8_t, std::string, 2, KeyPolicy<int8_t>> m17{
      {1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(SetIntTest, LinearEmplaceIntKeyWithPolicy) {
  LinearFlatSet<int32_t, KeyPolicy<int32_t>> s;
  s.emplace(9);
  s.emplace(8);
  s.insert(9);
  EXPECT_EQ(s.size(), 2);
  EXPECT_TRUE(s.contains(9));
  EXPECT_TRUE(s.contains(8));
}

TEST(SetIntTest, LinearMixedInlineSizeIntKeyWithPolicy) {
  LinearFlatSet<int16_t, KeyPolicy<int16_t>> m_src{1, 2, 3};
  EXPECT_TRUE(AssertSetContent_123(m_src));
  InlineLinearFlatSet<int16_t, 3, KeyPolicy<int16_t>> m_src2{1, 2, 3};
  EXPECT_TRUE(AssertSetContent_123(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatSet<int16_t, KeyPolicy<int16_t>> m1(m_src);
  EXPECT_TRUE(AssertSetContent_123(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatSet<int16_t, KeyPolicy<int16_t>> m2(m_src2);
  EXPECT_TRUE(AssertSetContent_123(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatSet<int16_t, 2, KeyPolicy<int16_t>> m3(m_src);
  EXPECT_TRUE(AssertSetContent_123(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatSet<int16_t, 2, KeyPolicy<int16_t>> m4(m_src2);
  EXPECT_TRUE(AssertSetContent_123(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatSet<int16_t, 5, KeyPolicy<int16_t>> m5(m_src);
  EXPECT_TRUE(AssertSetContent_123(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatSet<int16_t, 5, KeyPolicy<int16_t>> m6(m_src2);
  EXPECT_TRUE(AssertSetContent_123(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatSet<int16_t, KeyPolicy<int16_t>> m7{21, 22, 23};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m7));

  InlineLinearFlatSet<int16_t, 3, KeyPolicy<int16_t>> m8{21, 22, 23};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatSet<int16_t, 2, KeyPolicy<int16_t>> m9{21, 22, 23};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatSet<int16_t, 5, KeyPolicy<int16_t>> m10{21, 22, 23};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatSet<int16_t, KeyPolicy<int16_t>> m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatSet<int16_t, 3, KeyPolicy<int16_t>> m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatSet<int16_t, 2, KeyPolicy<int16_t>> m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatSet<int16_t, 5, KeyPolicy<int16_t>> m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatSet<int16_t, KeyPolicy<int16_t>> m15{21, 22, 23};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertSetContent_123(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatSet<int16_t, 3, KeyPolicy<int16_t>> m16{21, 22, 23};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertSetContent_123(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatSet<int16_t, 2, KeyPolicy<int16_t>> m17{21, 22, 23};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertSetContent_123(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(LinearMap, SwapIntKeyWithPolicy) {
  {
    LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 2, KeyPolicy<int32_t>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5, KeyPolicy<int32_t>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    InlineLinearFlatMap<int32_t, std::string, 3, KeyPolicy<int32_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5, KeyPolicy<int32_t>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m2{
        {-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 2, KeyPolicy<int32_t>> m2{
        {-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string, KeyPolicy<int32_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5, KeyPolicy<int32_t>> m2{
        {-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    InlineLinearFlatMap<int32_t, std::string, 3, KeyPolicy<int32_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5, KeyPolicy<int32_t>> m2{
        {-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }
}

TEST(LinearSet, SwapIntKeyWithPolicy) {
  {
    LinearFlatSet<int, KeyPolicy<int>> m1{1, 2, 3};
    LinearFlatSet<int, KeyPolicy<int>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int, KeyPolicy<int>> m1{1, 2, 3};
    InlineLinearFlatSet<int, 2, KeyPolicy<int>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int, KeyPolicy<int>> m1{1, 2, 3};
    InlineLinearFlatSet<int, 5, KeyPolicy<int>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    InlineLinearFlatSet<int, 3, KeyPolicy<int>> m1{1, 2, 3};
    InlineLinearFlatSet<int, 5, KeyPolicy<int>> m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int, KeyPolicy<int>> m1{1, 2, 3};
    LinearFlatSet<int, KeyPolicy<int>> m2{-1, -2, -3};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int, KeyPolicy<int>> m1{1, 2, 3};
    InlineLinearFlatSet<int, 2, KeyPolicy<int>> m2{-1, -2, -3};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    LinearFlatSet<int, KeyPolicy<int>> m1{1, 2, 3};
    InlineLinearFlatSet<int, 5, KeyPolicy<int>> m2{-1, -2, -3};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }

  {
    InlineLinearFlatSet<int, 3, KeyPolicy<int>> m1{1, 2, 3};
    InlineLinearFlatSet<int, 5, KeyPolicy<int>> m2{-1, -2, -3};
    m1.swap(m2);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertSetContent_n1n2n3(m2));
    EXPECT_TRUE(AssertSetContent_123(m1));
  }
}

TEST(LinearMap, MergeIntKeyWithPolicy) {
  {
    LinearFlatMap<uint8_t, std::string, KeyPolicy<uint8_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string, KeyPolicy<uint8_t>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatMap<uint8_t, std::string, KeyPolicy<uint8_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string, KeyPolicy<uint8_t>> m2{
        {1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    LinearFlatMap<uint8_t, std::string, KeyPolicy<uint8_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string, KeyPolicy<uint8_t>> m2{
        {1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_1232040(m1));
    EXPECT_TRUE(AssertMapContent_13_1030(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3, KeyPolicy<uint8_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string, KeyPolicy<uint8_t>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3, KeyPolicy<uint8_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 3, KeyPolicy<uint8_t>> m2{
        {1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3, KeyPolicy<uint8_t>> m1{
        {1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 4, KeyPolicy<uint8_t>> m2{
        {1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_1232040(m1));
    EXPECT_TRUE(AssertMapContent_13_1030(m2));
  }
}

TEST(LinearSet, MergeIntKeyWithPolicy) {
  {
    LinearFlatSet<int8_t, KeyPolicy<int8_t>> m1{1, 2, 3};
    LinearFlatSet<int8_t, KeyPolicy<int8_t>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatSet<int8_t, KeyPolicy<int8_t>> m1{1, 2, 3};
    LinearFlatSet<int8_t, KeyPolicy<int8_t>> m2{1, 2, 3};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));
  }

  {
    LinearFlatSet<int8_t, KeyPolicy<int8_t>> m1{1, 2, 3};
    LinearFlatSet<int8_t, KeyPolicy<int8_t>> m2{1, -2, 3, 4};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123n24(m1));
    EXPECT_TRUE(AssertMapContent_13(m2));
  }

  {
    InlineLinearFlatSet<int8_t, 3, KeyPolicy<int8_t>> m1{1, 2, 3};
    LinearFlatSet<int8_t, KeyPolicy<int8_t>> m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertSetContent_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatSet<int8_t, 3, KeyPolicy<int8_t>> m1{1, 2, 3};
    InlineLinearFlatSet<int8_t, 3, KeyPolicy<int8_t>> m2{1, 2, 3};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123(m1));
    EXPECT_TRUE(AssertSetContent_123(m2));
  }

  {
    InlineLinearFlatSet<int8_t, 3, KeyPolicy<int8_t>> m1{1, 2, 3};
    InlineLinearFlatSet<int8_t, 4, KeyPolicy<int8_t>> m2{1, -2, 3, 4};
    m1.merge(m2);
    EXPECT_TRUE(AssertSetContent_123n24(m1));
    EXPECT_TRUE(AssertMapContent_13(m2));
  }
}

TEST(Vector, LinearMapInsertOrAssignIntKeyWithConsecutivePolicy) {
  InlineLinearFlatMap<int16_t, std::string, 5,
                      MapKeyPolicyConsecutiveIntegers<int16_t>>
      map{{3, "c"}, {2, "b"}, {1, "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.capacity(), 5);
  EXPECT_EQ(map[1], "a");
  EXPECT_EQ(map[2], "b");
  EXPECT_EQ(map[3], "c");
  EXPECT_EQ(map[4], "");

  auto r = map.insert_or_assign(4, "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map[4], "d");

  std::string se = "e";
  auto r2 = map.insert_or_assign(5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map[5], "e");
  EXPECT_TRUE(se.empty());

  std::string sf = "f";
  auto r3 = map.insert_or_assign(6, std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map[6], "f");
  EXPECT_TRUE(sf.empty());

  std::string sg = "g";
  auto r4 = map.insert_or_assign(7, sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map[7], "g");
  EXPECT_EQ(sg, "g");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearMapEmplaceOrAssignIntKeyWithConsecutivePolicy) {
  InlineLinearFlatMap<int16_t, std::string, 5,
                      MapKeyPolicyConsecutiveIntegers<int16_t>>
      map{{3, "c"}, {2, "b"}, {1, "a"}};
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map[1], "a");
  EXPECT_EQ(map[2], "b");
  EXPECT_EQ(map[3], "c");
  EXPECT_EQ(map[4], "");

  auto r = map.emplace_or_assign(4, "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map[4], "d");

  std::string se = "e";
  auto r2 = map.emplace_or_assign(5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map[5], "e");
  EXPECT_TRUE(se.empty());

  std::string sf = "f";
  auto r3 = map.emplace_or_assign(6, std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map[6], "f");
  EXPECT_TRUE(sf.empty());

  std::string sg = "g";
  auto r4 = map.emplace_or_assign(7, sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map[7], "g");
  EXPECT_EQ(sg, "g");

  auto r5 = map.emplace_or_assign(7, 5, 'g');
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(map[7], "ggggg");

  EXPECT_EQ(map.size(), 7);
}

TEST(Vector, LinearMapEmplaceIntKeyWithConsecutivePolicy) {
  LinearFlatMap<int16_t, std::string, MapKeyPolicyConsecutiveIntegers<int16_t>>
      map;
  auto r = map.emplace(std::piecewise_construct, std::tuple<int16_t>(12),
                       std::tuple<const char*, size_t>("abc", 2));
  EXPECT_TRUE(r.second);
  EXPECT_EQ(*r.first, "ab");
  auto r2 = map.emplace(std::piecewise_construct, std::tuple<int16_t>(11),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(*r2.first, "xy");

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map[12], "ab");
  EXPECT_EQ(map[11], "xy");

  auto r3 = map.emplace(std::piecewise_construct, std::forward_as_tuple(12),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(*r3.first, "ab");

  EXPECT_EQ(map.size(), 2);

  auto r4 = map.try_emplace(11, "ab");
  EXPECT_FALSE(r4.second);
  EXPECT_EQ(*r4.first, "xy");

  std::string sXYZ = "xyz";
  auto r5 = map.try_emplace(11, std::move(sXYZ));
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(*r5.first, "xy");
  EXPECT_EQ(sXYZ, "xyz");

  auto r6 = map.try_emplace(13, std::move(sXYZ));
  EXPECT_TRUE(r6.second);
  EXPECT_EQ(*r6.first, "xyz");
  EXPECT_TRUE(sXYZ.empty());

  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map[12], "ab");
  EXPECT_EQ(map[11], "xy");
  EXPECT_EQ(map[13], "xyz");

  std::string sUVW = "uvw";
  auto r7 = map.try_emplace(14, sUVW);
  EXPECT_TRUE(r7.second);
  EXPECT_EQ(*r7.first, "uvw");
  EXPECT_EQ(sUVW, "uvw");

  EXPECT_EQ(map.size(), 4);
  EXPECT_EQ(map[12], "ab");
  EXPECT_EQ(map[11], "xy");
  EXPECT_EQ(map[13], "xyz");
  EXPECT_EQ(map[14], "uvw");
}

TEST(MapIntTest, LinearBasicOperationsIntKeyWithConsecutivePolicy) {
  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>> m;

  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.size(), 0);

  auto ret = m.insert({99, "red"});
  ASSERT_TRUE(ret.second);
  EXPECT_EQ(*ret.first, "red");
  EXPECT_EQ(m.size(), 1);
}

TEST(MapIntTest, LinearElementAccessIntKeyWithConsecutivePolicy) {
  LinearFlatMap<int32_t, std::string, MapKeyPolicyConsecutiveIntegers<int32_t>>
      m{{99, "red"}, {100, "yellow"}};

  EXPECT_EQ(m[99], "red");

  m[99] = "green";
  EXPECT_EQ(m[99], "green");
  EXPECT_EQ(m.at(99), "green");

  EXPECT_EQ(m[101], "");
  EXPECT_EQ(m.at(101), "");
  EXPECT_EQ(m.size(), 3);

  EXPECT_EQ(m.at(102), "");
  m.at(102) = "orange";
  EXPECT_EQ(m[102], "orange");
  EXPECT_EQ(m.size(), 4);

  int32_t i103 = 103;
  m.at(std::move(i103)) = "green";
  EXPECT_EQ(m.find(103)->first, 103);
  EXPECT_EQ(m.find(103)->second, "green");
  EXPECT_EQ(m.size(), 5);

  int32_t i105 = 105;
  auto r = m.insert_default_if_absent(std::move(i105));
  EXPECT_TRUE(r.second);
  *r.first = "yellow";
  EXPECT_EQ(m[105], "yellow");
  EXPECT_EQ(m.size(), 6);

  auto r2 = m.insert_default_if_absent(99);
  EXPECT_FALSE(r2.second);
  EXPECT_EQ(m[99], "green");
  *r2.first = "red";
  EXPECT_EQ(m[99], "red");
  EXPECT_EQ(m.size(), 6);

  std::string black = "black";
  auto r3 = m.insert_if_absent(99, black);
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(*r3.first, "red");
  EXPECT_EQ(m.size(), 6);

  std::string pink = "pink";
  auto r4 = m.insert_if_absent(200, std::move(pink));
  EXPECT_TRUE(r4.second);
  EXPECT_TRUE(pink.empty());
  EXPECT_EQ(*r4.first, "pink");
  EXPECT_EQ(m.size(), 7);

  auto r5 = m.insert_if_absent(300, black);
  EXPECT_TRUE(r5.second);
  EXPECT_EQ(*r5.first, "black");
  EXPECT_EQ(black, "black");
  EXPECT_EQ(m.size(), 8);
}

TEST(MapIntTest, LinearInsertUpdateIntKeyWithConsecutivePolicy) {
  LinearFlatMap<int16_t, std::string, MapKeyPolicyConsecutiveIntegers<int16_t>>
      m;

  auto ret1 = m.insert({99, "apple"});
  EXPECT_TRUE(ret1.second);
  auto ret2 = m.insert({99, "banana"});
  EXPECT_FALSE(ret2.second);
  EXPECT_EQ(*ret2.first, "apple");

  auto emp_ret = m.emplace(100, "blue");
  EXPECT_TRUE(emp_ret.second);
  EXPECT_EQ(*emp_ret.first, "blue");

  m[100] = "red";
  EXPECT_EQ(m[100], "red");

  auto it = m.insert_unique({101, "car"});
  EXPECT_TRUE(*it == "car");
  std::pair<const int16_t, std::string> value_type{102, "train"};
  auto it2 = m.insert_unique(value_type);
  EXPECT_TRUE(*it2 == "train");
  EXPECT_TRUE(!value_type.second.empty());
  EXPECT_TRUE(m[101] == "car");
  EXPECT_TRUE(m[102] == "train");

  std::pair<const int16_t, std::string> value_type2{200, "tiger"};
  auto it3 = m.insert_unique(std::move(value_type2));
  EXPECT_TRUE(*it3 == "tiger");
  EXPECT_TRUE(value_type2.second.empty());
  EXPECT_TRUE(m[200] == "tiger");

  std::pair<int16_t, std::string> value_type3{201, "student"};
  auto it4 = m.insert_unique(value_type3);
  EXPECT_TRUE(*it4 == "student");
  EXPECT_FALSE(value_type3.second.empty());
  EXPECT_TRUE(m[201] == "student");

  std::pair<int16_t, std::string> value_type4{202, "doctor"};
  auto it5 = m.insert_unique(std::move(value_type4));
  EXPECT_TRUE(*it5 == "doctor");
  EXPECT_TRUE(value_type4.second.empty());
  EXPECT_TRUE(m[202] == "doctor");

  auto it6 = m.emplace_unique(300, "fanfan", 3);
  EXPECT_TRUE(*it6 == "fan");
  EXPECT_TRUE(m[300] == "fan");

  const int16_t key301 = 301;
  auto it7 = m.emplace_unique(key301, 3, 'x');
  EXPECT_TRUE(*it7 == "xxx");
  EXPECT_TRUE(m[301] == "xxx");

  auto it8 =
      m.emplace_unique(std::piecewise_construct, std::forward_as_tuple(302),
                       std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_TRUE(*it8 == "xy");
  EXPECT_TRUE(m[302] == "xy");
}

TEST(MapIntTest, LinearEraseOperationsIntKeyWithConsecutivePolicy) {
  LinearFlatMap<int16_t, std::string, MapKeyPolicyConsecutiveIntegers<int16_t>>
      m{{30, "1"}, {31, "2"}, {32, "3"}};
  EXPECT_EQ(m.size(), 3);
  EXPECT_EQ(m[30], "1");
  EXPECT_EQ(m[31], "2");
  EXPECT_EQ(m[32], "3");

  size_t cnt = m.erase(31);
  EXPECT_EQ(cnt, 1);
  EXPECT_EQ(m.size(), 2);
  EXPECT_FALSE(m.contains(31));

  auto it = m.find(30);
  m.erase(it);
  EXPECT_EQ(m.size(), 1);
  EXPECT_FALSE(m.contains(30));

  EXPECT_EQ(m.erase(100), 0);
}

TEST(MapIntTest, LinearIteratorsIntKeyWithConsecutivePolicy_i8) {
  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>> m{
      {26, "26"}, {1, "1"}, {13, "13"}};

  auto it = m.begin();
  EXPECT_EQ(it->first, m.is_data_ordered() ? 1 : 26);
  EXPECT_EQ(it->second, m.is_data_ordered() ? "1" : "26");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? 13 : 1);
  EXPECT_EQ(it->second, m.is_data_ordered() ? "13" : "1");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? 26 : 13);
  EXPECT_EQ(it->second, m.is_data_ordered() ? "26" : "13");
  ++it;
  EXPECT_EQ(it, m.end());

  auto rit = m.rbegin();
  EXPECT_EQ(*rit, m.is_data_ordered() ? "26" : "13");
  ++rit;
  EXPECT_EQ(*rit, m.is_data_ordered() ? "13" : "1");
  ++rit;
  EXPECT_EQ(*rit, m.is_data_ordered() ? "1" : "26");
  ++rit;
  EXPECT_EQ(rit, m.rend());
}

TEST(MapIntTest, LinearIteratorsIntKeyWithConsecutivePolicy) {
  LinearFlatMap<int32_t, std::string, MapKeyPolicyConsecutiveIntegers<int32_t>>
      m{{26, "26"}, {1, "1"}, {13, "13"}};

  auto it = m.begin();
  EXPECT_EQ(it->first, m.is_data_ordered() ? 1 : 26);
  EXPECT_EQ(it->second, m.is_data_ordered() ? "1" : "26");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? 13 : 1);
  EXPECT_EQ(it->second, m.is_data_ordered() ? "13" : "1");
  ++it;
  EXPECT_EQ(it->first, m.is_data_ordered() ? 26 : 13);
  EXPECT_EQ(it->second, m.is_data_ordered() ? "26" : "13");
  ++it;
  EXPECT_EQ(it, m.end());

  auto rit = m.rbegin();
  EXPECT_EQ(*rit, m.is_data_ordered() ? "26" : "13");
  ++rit;
  EXPECT_EQ(*rit, m.is_data_ordered() ? "13" : "1");
  ++rit;
  EXPECT_EQ(*rit, m.is_data_ordered() ? "1" : "26");
  ++rit;
  EXPECT_EQ(rit, m.rend());
}

TEST(MapIntTest, LinearInsertOrAssignIntKeyWithConsecutivePolicy) {
  LinearFlatMap<int16_t, std::string, MapKeyPolicyConsecutiveIntegers<int16_t>>
      m;

  {
    auto [it, inserted] = m.insert_or_assign(10, "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(*it, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.insert_or_assign(10, "banana");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(*it, "banana");
    EXPECT_EQ(m.size(), 1);
  }

  m.insert_or_assign(11, "11");
  EXPECT_EQ(m[11], "11");

  auto [it, _] = m.insert_or_assign(12, "orange");
  EXPECT_EQ(*it, "orange");
}

TEST(MapIntTest, LinearEmplaceOrAssignIntKeyWithConsecutivePolicy) {
  LinearFlatMap<int16_t, std::string, MapKeyPolicyConsecutiveIntegers<int16_t>>
      m;

  {
    auto [it, inserted] = m.emplace_or_assign(10, "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(*it, "apple");
    EXPECT_EQ(m.size(), 1);
  }

  {
    auto [it, inserted] = m.emplace_or_assign(10, "banana", 4);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(*it, "bana");
    EXPECT_EQ(m[10], "bana");
    EXPECT_EQ(m.size(), 1);
  }

  m.emplace_or_assign(11, "11");
  EXPECT_EQ(m[11], "11");

  auto [it, _] = m.emplace_or_assign(12, "orange");
  EXPECT_EQ(*it, "orange");
}

TEST(MapIntTest, LinearEmplacePiecewiseIntKeyWithConsecutivePolicy) {
  LinearFlatMap<uint32_t, std::string,
                MapKeyPolicyConsecutiveIntegers<uint32_t>>
      m;

  auto emp_it = m.emplace(std::piecewise_construct, std::forward_as_tuple(99u),
                          std::forward_as_tuple(5, 'X'));
  ASSERT_TRUE(emp_it.second);
  EXPECT_EQ(*emp_it.first, "XXXXX");

  m.emplace(std::piecewise_construct, std::forward_as_tuple(199u),
            std::forward_as_tuple(3, 'k'));
  EXPECT_EQ(m[199], "kkk");

  auto emp_fail = m.emplace(std::piecewise_construct, std::forward_as_tuple(99),
                            std::forward_as_tuple("new_value"));
  EXPECT_FALSE(emp_fail.second);
  EXPECT_EQ(m[99], "XXXXX");
}

TEST(MapIntTest, LinearMixedInlineSizeIntKeyWithConsecutivePolicy) {
  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      m_src{{1, "1"}, {2, "2"}, {3, "3"}};
  EXPECT_TRUE(AssertMapContent_123_123(m_src));
  InlineLinearFlatMap<int8_t, std::string, 3,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m_src2{{1, "1"}, {2, "2"}, {3, "3"}};
  EXPECT_TRUE(AssertMapContent_123_123(m_src2));
  EXPECT_TRUE(m_src2.is_static_buffer());
  EXPECT_TRUE(m_src == m_src2);

  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      m1(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m1));
  EXPECT_TRUE(m1 == m_src);
  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      m2(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m2));
  EXPECT_TRUE(m2 == m_src2);
  InlineLinearFlatMap<int8_t, std::string, 2,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m3(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m3));
  EXPECT_FALSE(m3.is_static_buffer());
  EXPECT_TRUE(m3 == m_src);
  InlineLinearFlatMap<int8_t, std::string, 2,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m4(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m4));
  EXPECT_FALSE(m4.is_static_buffer());
  EXPECT_TRUE(m4 == m_src2);
  InlineLinearFlatMap<int8_t, std::string, 5,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m5(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m5));
  EXPECT_TRUE(m5.is_static_buffer());
  EXPECT_TRUE(m5 == m_src);
  InlineLinearFlatMap<int8_t, std::string, 5,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m6(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m6));
  EXPECT_TRUE(m6.is_static_buffer());
  EXPECT_TRUE(m6 == m_src2);

  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      m7{{1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m7 == m_src);
  EXPECT_TRUE(m7 != m_src);
  m7 = m_src;
  EXPECT_TRUE(m7 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m7));

  InlineLinearFlatMap<int8_t, std::string, 3,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m8{{1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m8 == m_src);
  EXPECT_TRUE(m8 != m_src);
  m8 = m_src;
  EXPECT_TRUE(m8 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m8));
  EXPECT_TRUE(m8.is_static_buffer());

  InlineLinearFlatMap<int8_t, std::string, 2,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m9{{1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m9 == m_src);
  EXPECT_TRUE(m9 != m_src);
  m9 = m_src;
  EXPECT_TRUE(m9 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m9));
  EXPECT_FALSE(m9.is_static_buffer());

  InlineLinearFlatMap<int8_t, std::string, 5,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m10{{1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m10 == m_src);
  EXPECT_TRUE(m10 != m_src);
  m10 = m_src;
  EXPECT_TRUE(m10 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m10));
  EXPECT_TRUE(m10.is_static_buffer());

  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      m11(std::move(m7));
  EXPECT_TRUE(m11 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m11));
  EXPECT_TRUE(m7.empty());

  InlineLinearFlatMap<int8_t, std::string, 3,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m12(std::move(m8));
  EXPECT_TRUE(m12 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m12));
  EXPECT_TRUE(m12.is_static_buffer());
  EXPECT_TRUE(m8.empty());

  InlineLinearFlatMap<int8_t, std::string, 2,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m13(std::move(m9));
  EXPECT_TRUE(m13 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m13));
  EXPECT_FALSE(m13.is_static_buffer());
  EXPECT_TRUE(m9.empty());

  InlineLinearFlatMap<int8_t, std::string, 5,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m14(std::move(m10));
  EXPECT_TRUE(m14 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m14));
  EXPECT_TRUE(m14.is_static_buffer());
  EXPECT_TRUE(m10.empty());

  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      m15{{1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m15 == m_src);
  EXPECT_TRUE(m15 != m_src);
  m15 = std::move(m11);
  EXPECT_TRUE(m15 == m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m15));
  EXPECT_TRUE(m11.empty());

  InlineLinearFlatMap<int8_t, std::string, 3,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m16{{1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m16 == m_src);
  EXPECT_TRUE(m16 != m_src);
  m16 = std::move(m_src);
  EXPECT_TRUE(AssertMapContent_123_123(m16));
  EXPECT_TRUE(m_src.empty());

  InlineLinearFlatMap<int8_t, std::string, 2,
                      MapKeyPolicyConsecutiveIntegers<int8_t>>
      m17{{1, "11"}, {2, "22"}, {3, "33"}};
  EXPECT_FALSE(m17 == m_src);
  EXPECT_TRUE(m17 != m_src);
  m17 = std::move(m_src2);
  EXPECT_TRUE(AssertMapContent_123_123(m17));
  EXPECT_TRUE(m_src2.empty());
}

TEST(LinearMap, SwapIntKeyWithConsecutivePolicy) {
  {
    LinearFlatMap<int32_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<int32_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<int32_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<int32_t>>
        m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<int32_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 2,
                        MapKeyPolicyConsecutiveIntegers<int32_t>>
        m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<int32_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5,
                        MapKeyPolicyConsecutiveIntegers<int32_t>>
        m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    InlineLinearFlatMap<int32_t, std::string, 3,
                        MapKeyPolicyConsecutiveIntegers<int32_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5,
                        MapKeyPolicyConsecutiveIntegers<int32_t>>
        m2;
    m1.swap(m2);
    EXPECT_TRUE(m1.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<int32_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<int32_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<int32_t>>
        m2{{-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<int32_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 2,
                        MapKeyPolicyConsecutiveIntegers<int32_t>>
        m2{{-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<int32_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<int32_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5,
                        MapKeyPolicyConsecutiveIntegers<int32_t>>
        m2{{-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    InlineLinearFlatMap<int32_t, std::string, 3,
                        MapKeyPolicyConsecutiveIntegers<int32_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<int32_t, std::string, 5,
                        MapKeyPolicyConsecutiveIntegers<int32_t>>
        m2{{-1, "1"}, {-2, "2"}, {-3, "3"}};
    m1.swap(m2);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));

    m2.swap(m1);
    EXPECT_TRUE(AssertMapContent_n1n2n3_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }
}

TEST(LinearMap, MergeIntKeyWithConsecutivePolicy) {
  {
    LinearFlatMap<uint8_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    LinearFlatMap<uint8_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m2{{1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    LinearFlatMap<uint8_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m2{{1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_1232040(m1));
    EXPECT_TRUE(AssertMapContent_13_1030(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3,
                        MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string,
                  MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(m1.empty());
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3,
                        MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 3,
                        MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m2{{1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3,
                        MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 4,
                        MapKeyPolicyConsecutiveIntegers<uint8_t>>
        m2{{1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_1232040(m1));
    EXPECT_TRUE(AssertMapContent_13_1030(m2));
  }
}

template <class K>
struct MergeAssignKeyPolicyConsecutiveIntegers
    : public MapKeyPolicyConsecutiveIntegers<K> {
  static constexpr auto assign_existing_for_merge = true;
};

TEST(LinearMap, MergeAssignIntKeyWithConsecutivePolicy) {
  {
    LinearFlatMap<uint8_t, std::string,
                  MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string,
                  MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    LinearFlatMap<uint8_t, std::string,
                  MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string,
                  MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m2{{1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    LinearFlatMap<uint8_t, std::string,
                  MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string,
                  MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m2{{1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_102302040(m1));
    EXPECT_TRUE(AssertMapContent_1n234_10203040(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3,
                        MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    LinearFlatMap<uint8_t, std::string,
                  MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m2;
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(m2.empty());
    m2.merge(m1);
    EXPECT_TRUE(AssertMapContent_123_123(m2));
    EXPECT_TRUE(AssertMapContent_123_123(m1));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3,
                        MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 3,
                        MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m2{{1, "1"}, {2, "2"}, {3, "3"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123_123(m1));
    EXPECT_TRUE(AssertMapContent_123_123(m2));
  }

  {
    InlineLinearFlatMap<uint8_t, std::string, 3,
                        MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m1{{1, "1"}, {2, "2"}, {3, "3"}};
    InlineLinearFlatMap<uint8_t, std::string, 4,
                        MergeAssignKeyPolicyConsecutiveIntegers<uint8_t>>
        m2{{1, "10"}, {-2, "20"}, {3, "30"}, {4, "40"}};
    m1.merge(m2);
    EXPECT_TRUE(AssertMapContent_123n24_102302040(m1));
    EXPECT_TRUE(AssertMapContent_1n234_10203040(m2));
  }
}

template <class MAP>
static void IntKeyMapComprehensiveTest() {
  const int num_elements = 1001;
  MAP original_map;
  std::vector<std::pair<const int, std::string>> data_vector;
  data_vector.reserve(num_elements);
  for (int i = 0; i < num_elements; ++i) {
    std::string value = "Value_" + std::to_string(i);
    original_map[i] = value;
    data_vector.emplace_back(i, value);
  }
  // Make sure data is correct
  ASSERT_EQ(original_map.size(), num_elements);
  ASSERT_EQ(data_vector.size(), num_elements);
  // =========================================================================
  // 2. Test Constructors
  // =========================================================================
  // 2.1 Test default constructors
  MAP default_constructed_map;
  ASSERT_TRUE(default_constructed_map.empty());
  // 2.3 Copy Constructor
  MAP copy_constructed_map(original_map);
  ASSERT_EQ(copy_constructed_map.size(), num_elements);
  ASSERT_EQ(copy_constructed_map, original_map);
  copy_constructed_map.erase(0);
  ASSERT_NE(copy_constructed_map, original_map);
  ASSERT_EQ(original_map.size(), num_elements);
  // 2.4 Move Constructor
  MAP map_to_move = original_map;
  MAP move_constructed_map(std::move(map_to_move));
  ASSERT_EQ(move_constructed_map.size(), num_elements);
  ASSERT_EQ(move_constructed_map, original_map);
  ASSERT_TRUE(map_to_move.empty());
  // Assignment Operators
  MAP copy_assigned_map;
  copy_assigned_map[9999] = "some_value";
  copy_assigned_map = original_map;
  ASSERT_EQ(copy_assigned_map.size(), num_elements);
  ASSERT_EQ(copy_assigned_map, original_map);
  copy_assigned_map.erase(1);
  ASSERT_NE(copy_assigned_map, original_map);
  ASSERT_EQ(original_map.size(), num_elements);
  // 3.2 test move
  MAP map_to_move_assign = original_map;
  MAP move_assigned_map;
  move_assigned_map[8888] = "another_value";
  move_assigned_map = std::move(map_to_move_assign);
  ASSERT_EQ(move_assigned_map.size(), num_elements);
  ASSERT_EQ(move_assigned_map, original_map);
  ASSERT_TRUE(map_to_move_assign.empty());
  // 4. Equality/Inequality
  MAP map_for_comparison = original_map;
  ASSERT_TRUE(map_for_comparison == original_map);
  map_for_comparison[0] = "Modified_Value";
  ASSERT_TRUE(map_for_comparison != original_map);
  ASSERT_NE(map_for_comparison, original_map);

  MAP loop_test_map = original_map;
  ASSERT_EQ(loop_test_map.size(), num_elements);
  for (int i = 0; i < num_elements; ++i) {
    // 5.1 test find
    auto it = loop_test_map.find(i);
    ASSERT_NE(it, loop_test_map.end()) << "Failed to find key " << i;
    ASSERT_EQ(it->first, i);
    ASSERT_EQ(it->second, "Value_" + std::to_string(i));
    // 5.2 test erase
    size_t size_before_erase = loop_test_map.size();
    loop_test_map.erase(it);
    ASSERT_EQ(loop_test_map.size(), size_before_erase - 1);
    // make sure erased
    ASSERT_EQ(loop_test_map.find(i), loop_test_map.end())
        << "Key " << i << " should have been erased.";
    // 5.3 test insert
    std::string value_to_insert = "Value_" + std::to_string(i);
    auto insert_result = loop_test_map.insert({i, value_to_insert});
    // test inserted
    ASSERT_TRUE(insert_result.second) << "Failed to re-insert key " << i;
    // test size
    ASSERT_EQ(loop_test_map.size(), size_before_erase);
    // test content of inserted value
    if constexpr (MAP::consecutive_key) {
      ASSERT_EQ(*insert_result.first, value_to_insert);
    } else {
      ASSERT_EQ(insert_result.first->second, value_to_insert);
    }
  }
  // Map unchanged
  ASSERT_EQ(loop_test_map, original_map);
}

template <class MAP>
static void IntKeyMapRandomInsertEraseTest() {
  const int num_elements = 1000;
  std::vector<int> keys(num_elements);
  std::iota(keys.begin(), keys.end(), 0);
  ASSERT_EQ(keys.size(), num_elements);
  ASSERT_EQ(keys[0], 0);
  ASSERT_EQ(keys[num_elements - 1], 999);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(keys.begin(), keys.end(), g);
  MAP map_under_test;
  for (int key : keys) {
    map_under_test[key] = "Value_" + std::to_string(key);
  }
  ASSERT_FALSE(map_under_test.empty());
  ASSERT_EQ(map_under_test.size(), num_elements);
  ASSERT_EQ(map_under_test.at(0), "Value_0");
  ASSERT_EQ(map_under_test.at(500), "Value_500");
  ASSERT_EQ(map_under_test.at(999), "Value_999");
  std::shuffle(keys.begin(), keys.end(), g);
  size_t expected_size = num_elements;
  for (int key_to_erase : keys) {
    ASSERT_EQ(map_under_test.size(), expected_size);
    size_t erased_count = map_under_test.erase(key_to_erase);
    ASSERT_EQ(erased_count, 1) << "Failed to erase key: " << key_to_erase;
    expected_size--;
  }
  ASSERT_TRUE(map_under_test.empty())
      << "Map should be empty after erasing all elements.";
  ASSERT_EQ(map_under_test.size(), 0)
      << "Map size should be 0 after erasing all elements.";
}

TEST(IntKeyMap, ComprehensiveTestLinearFlat) {
  IntKeyMapComprehensiveTest<LinearFlatMap<int, std::string>>();
  IntKeyMapComprehensiveTest<LinearFlatMap<
      int16_t, std::string, MapKeyPolicyConsecutiveIntegers<int16_t>>>();
  IntKeyMapComprehensiveTest<LinearFlatMap<
      int32_t, std::string, MapKeyPolicyConsecutiveIntegers<int16_t>>>();
  IntKeyMapRandomInsertEraseTest<LinearFlatMap<int, std::string>>();
  IntKeyMapRandomInsertEraseTest<LinearFlatMap<
      int16_t, std::string, MapKeyPolicyConsecutiveIntegers<int16_t>>>();
  IntKeyMapRandomInsertEraseTest<LinearFlatMap<
      int32_t, std::string, MapKeyPolicyConsecutiveIntegers<int16_t>>>();
}

TEST(IntKeyMap, ComprehensiveTestOrderedFlat) {
  IntKeyMapComprehensiveTest<OrderedFlatMap<int, std::string>>();
  IntKeyMapRandomInsertEraseTest<OrderedFlatMap<int, std::string>>();
}

TEST(IntKeyMap, ComprehensiveTestLinearFlatWithConsecutivePolicy) {
  IntKeyMapRandomInsertEraseTest<
      LinearFlatMap<int, std::string, MapKeyPolicyConsecutiveIntegers<int>>>();
}

TEST(IntKeyMap, ConsecutivePolicyRangedLoop) {
  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      map{{3, "33"}, {2, "22"}, {1, "11"}};
  int index = 3;
  for (auto& it : map) {
    EXPECT_TRUE(it.first == index);
    EXPECT_TRUE(it.second == std::to_string(index) + std::to_string(index));
    it.second =
        std::to_string(index) + std::to_string(index) + std::to_string(index);
    index--;
  }
  EXPECT_TRUE(index == 0);
  EXPECT_TRUE(map[3] == "333");
  EXPECT_TRUE(map[2] == "222");
  EXPECT_TRUE(map[1] == "111");
}

TEST(IntKeyMap, ConsecutivePolicyRangedLoop2) {
  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      map{{3, "33"}, {2, "22"}, {1, "11"}};
  int index = 3;
  for (auto& [key, value] : map) {
    EXPECT_TRUE(key == index);
    EXPECT_TRUE(value == std::to_string(index) + std::to_string(index));
    value =
        std::to_string(index) + std::to_string(index) + std::to_string(index);
    index--;
  }
  EXPECT_TRUE(index == 0);
  EXPECT_TRUE(map[3] == "333");
  EXPECT_TRUE(map[2] == "222");
  EXPECT_TRUE(map[1] == "111");
}

TEST(IntKeyMap, ConsecutivePolicyIteratorImplicitToPair) {
  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      map{{3, "33"}, {2, "22"}, {1, "11"}};
  std::map<int8_t, std::string> ordered_map(map.begin(), map.end());

  int index = 1;
  for (auto& [key, value] : ordered_map) {
    EXPECT_TRUE(key == index);
    EXPECT_TRUE(value == std::to_string(index) + std::to_string(index));
    value =
        std::to_string(index) + std::to_string(index) + std::to_string(index);
    index++;
  }
}

TEST(IntKeyMap, ConsecutivePolicyFrontBack) {
  {
    LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
        m{{3, "33"}, {2, "22"}, {1, "11"}};
    EXPECT_EQ(m.front().first, 3);
    EXPECT_EQ(m.front().second, "33");
    EXPECT_EQ(m.back().first, 1);
    EXPECT_EQ(m.back().second, "11");
    m.erase(3);
    m.erase(1);
    EXPECT_EQ(m.front().first, 2);
    EXPECT_EQ(m.front().second, "22");
    EXPECT_EQ(m.back().first, 2);
    EXPECT_EQ(m.back().second, "22");

    m.front().second = "22222";
    EXPECT_EQ(m[2], "22222");
    m.back().second = "222";
    EXPECT_EQ(m[2], "222");
  }

  {
    const LinearFlatMap<int8_t, std::string,
                        MapKeyPolicyConsecutiveIntegers<int8_t>>
        m{{3, "33"}, {2, "22"}, {1, "11"}};
    EXPECT_EQ(m.front().first, 3);
    EXPECT_EQ(m.front().second, "33");
    EXPECT_EQ(m.back().first, 1);
    EXPECT_EQ(m.back().second, "11");
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(m)>>&>(m))
        .erase(3);
    (const_cast<std::remove_const_t<std::remove_reference_t<decltype(m)>>&>(m))
        .erase(1);
    EXPECT_EQ(m.front().first, 2);
    EXPECT_EQ(m.front().second, "22");
    EXPECT_EQ(m.back().first, 2);
    EXPECT_EQ(m.back().second, "22");
  }
}

TEST(IntKeyMap, ConsecutivePolicyFindEqEnd) {
  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      empty_map;
  EXPECT_TRUE(empty_map.find(2) == empty_map.end());
  EXPECT_TRUE(empty_map.begin() == empty_map.end());

  LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
      map{{3, "33"}, {2, "22"}, {1, "11"}};
  auto it = map.find(3);
  auto it2 = map.find(5);
  auto it3 = map.find(2);
  EXPECT_TRUE(it == map.begin());
  EXPECT_TRUE(map.begin() == it);
  EXPECT_TRUE(it3 != map.begin());
  EXPECT_TRUE(map.begin() != it3);
  EXPECT_TRUE(it != map.end());
  EXPECT_TRUE(map.end() != it);
  EXPECT_TRUE(it3 != map.end());
  EXPECT_TRUE(map.end() != it3);
  EXPECT_TRUE(it2 == map.end());
  EXPECT_TRUE(map.end() == it2);
  map.clear();
  EXPECT_TRUE(map.begin() == map.end());
}

TEST(IntKeyMap, ConsecutivePolicyErase) {
  {
    LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
        map{{3, "33"}, {2, "22"}, {1, "11"}};
    EXPECT_TRUE(map.size() == 3);
    auto it = map.find(3);
    auto it2 = map.erase(it);
    EXPECT_TRUE(*it2 == "22");
    EXPECT_TRUE(map.size() == 2);
    it2 = map.erase(it2);
    EXPECT_TRUE(*it2 == "11");
    EXPECT_TRUE(map.size() == 1);
    it2 = map.erase(it2);
    EXPECT_TRUE(map.empty());
    EXPECT_TRUE(it2 == map.end());
    EXPECT_TRUE(map.begin() == map.end());
  }

  {
    LinearFlatMap<int8_t, std::string, MapKeyPolicyConsecutiveIntegers<int8_t>>
        map{{3, "33"}, {2, "22"}, {1, "11"}};
    auto it = map.begin();
    EXPECT_TRUE(it == map.find(3));
    EXPECT_TRUE(map.size() == 3);
    auto it2 = map.erase(it);
    EXPECT_TRUE(*it2 == "22");
    EXPECT_TRUE(map.size() == 2);
    it = map.begin();
    it2 = map.erase(it);
    EXPECT_TRUE(*it2 == "11");
    EXPECT_TRUE(map.size() == 1);
    it = map.begin();
    it2 = map.erase(it);
    EXPECT_TRUE(map.empty());
    EXPECT_TRUE(it2 == map.end());
    EXPECT_TRUE(map.begin() == map.end());
  }
}

template <class MAP>
static MAP GenerateStringStringMapRandomInsert() {
  const int num_elements = 1000;
  std::vector<int> keys(num_elements);
  std::iota(keys.begin(), keys.end(), 0);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(keys.begin(), keys.end(), g);
  MAP map_under_test;
  for (int key : keys) {
    map_under_test[std::string("Key_") + std::to_string(key)] =
        "Value_" + std::to_string(key);
  }
  return map_under_test;
}

template <class MAP>
static MAP GenerateIntStringMapRandomInsert() {
  const int num_elements = 1000;
  std::vector<int> keys(num_elements);
  std::iota(keys.begin(), keys.end(), 0);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(keys.begin(), keys.end(), g);
  MAP map_under_test;
  for (int key : keys) {
    map_under_test[key] = "Value_" + std::to_string(key);
  }
  return map_under_test;
}

TEST(Map, EqualityCheck) {
  {
    auto map1 = GenerateStringStringMapRandomInsert<
        OrderedFlatMap<std::string, std::string>>();
    auto map2 = GenerateStringStringMapRandomInsert<
        InlineOrderedFlatMap<std::string, std::string, 500>>();
    EXPECT_TRUE(map1 == map2);
    map2["Key_500"] = "0";
    EXPECT_TRUE(map1 != map2);
    map2["Key_500"] = "Value_500";
    EXPECT_TRUE(map1 == map2);
    map1.erase("Key_100");
    EXPECT_TRUE(map1 != map2);
    map1["Key_100"] = "Value_100";
    EXPECT_TRUE(map1 == map2);
  }

  {
    auto map1 =
        GenerateIntStringMapRandomInsert<OrderedFlatMap<int, std::string>>();
    auto map2 = GenerateIntStringMapRandomInsert<
        InlineOrderedFlatMap<int, std::string, 500>>();
    EXPECT_TRUE(map1 == map2);
    map2[500] = "0";
    EXPECT_TRUE(map1 != map2);
    map2[500] = "Value_500";
    EXPECT_TRUE(map1 == map2);
    map1.erase(100);
    EXPECT_TRUE(map1 != map2);
    map1[100] = "Value_100";
    EXPECT_TRUE(map1 == map2);
  }

  {
    auto map1 = GenerateStringStringMapRandomInsert<
        LinearFlatMap<std::string, std::string>>();
    auto map2 = GenerateStringStringMapRandomInsert<
        InlineLinearFlatMap<std::string, std::string, 500>>();
    EXPECT_TRUE(map1 == map2);
    map2["Key_500"] = "0";
    EXPECT_TRUE(map1 != map2);
    map2["Key_500"] = "Value_500";
    EXPECT_TRUE(map1 == map2);
    map1.erase("Key_100");
    EXPECT_TRUE(map1 != map2);
    map1["Key_100"] = "Value_100";
    EXPECT_TRUE(map1 == map2);
  }

  {
    auto map1 =
        GenerateIntStringMapRandomInsert<LinearFlatMap<int, std::string>>();
    auto map2 = GenerateIntStringMapRandomInsert<
        InlineLinearFlatMap<int, std::string, 500>>();
    EXPECT_TRUE(map1 == map2);
    map2[500] = "0";
    EXPECT_TRUE(map1 != map2);
    map2[500] = "Value_500";
    EXPECT_TRUE(map1 == map2);
    map1.erase(100);
    EXPECT_TRUE(map1 != map2);
    map1[100] = "Value_100";
    EXPECT_TRUE(map1 == map2);
  }

  {
    auto map1 = GenerateIntStringMapRandomInsert<LinearFlatMap<
        int, std::string, MapKeyPolicyConsecutiveIntegers<int>>>();
    auto map2 = GenerateIntStringMapRandomInsert<InlineLinearFlatMap<
        int, std::string, 500, MapKeyPolicyConsecutiveIntegers<int>>>();
    EXPECT_TRUE(map1 == map2);
    map2[500] = "0";
    EXPECT_TRUE(map1 != map2);
    map2[500] = "Value_500";
    EXPECT_TRUE(map1 == map2);
    map1.erase(100);
    EXPECT_TRUE(map1 != map2);
    map1[100] = "Value_100";
    EXPECT_TRUE(map1 == map2);
  }
}

TEST(LinearFlatMap, for_each) {
  {
    LinearFlatMap<std::string, std::string> map{
        {"A", "1"}, {"B", "2"}, {"C", "3"}};
    std::string out;
    map.for_each([&](const std::string& key, std::string& value) {
      out += key;
      out += value;
      if (key == "B") {
        value = "22";
      }
    });
    EXPECT_TRUE(map["B"] == "22");
    EXPECT_TRUE(out == "A1B2C3");

    const auto map2 = map;
    map2.for_each([&](const std::string& key, const std::string& value) {
      out += key;
      out += value;
    });
    EXPECT_TRUE(out == "A1B2C3A1B22C3");
  }
  {
    LinearFlatMap<int, std::string> map{{1, "A"}, {2, "B"}, {3, "C"}};
    std::string out;
    map.for_each([&](int key, std::string& value) {
      out += std::to_string(key);
      out += value;
      if (key == 2) {
        value = "BB";
      }
    });
    EXPECT_TRUE(map[2] == "BB");
    EXPECT_TRUE(out == "1A2B3C");

    const auto map2 = map;
    map2.for_each([&](int key, const std::string& value) {
      out += std::to_string(key);
      out += value;
    });
    EXPECT_TRUE(out == "1A2B3C1A2BB3C");
  }
  {
    LinearFlatMap<int, std::string, MapKeyPolicyConsecutiveIntegers<int>> map{
        {1, "A"}, {2, "B"}, {3, "C"}};
    std::string out;
    map.for_each([&](int key, std::string& value) {
      out += std::to_string(key);
      out += value;
      if (key == 2) {
        value = "BB";
      }
    });
    EXPECT_TRUE(map[2] == "BB");
    EXPECT_TRUE(out == "1A2B3C");

    const auto map2 = map;
    map2.for_each([&](int key, const std::string& value) {
      out += std::to_string(key);
      out += value;
    });
    EXPECT_TRUE(out == "1A2B3C1A2BB3C");
  }
}

}  // namespace base
}  // namespace lynx

#pragma clang diagnostic pop
