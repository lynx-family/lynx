// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdio>
#include <string>
#include <vector>

#define private public

#include "base/include/bundled_optional.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace test {

template <typename T>
struct CountedWrapper {
  static int32_t g_instance_count;

  CountedWrapper() { g_instance_count++; }
  ~CountedWrapper() { g_instance_count--; }
  CountedWrapper(const CountedWrapper& other) : value(other.value) {
    g_instance_count++;
  }
  CountedWrapper(CountedWrapper&& other) : value(std::move(other.value)) {
    g_instance_count++;
  }
  CountedWrapper& operator=(const CountedWrapper& other) = default;
  CountedWrapper& operator=(CountedWrapper&& other) = default;

  T value;
};

template <typename T>
int32_t CountedWrapper<T>::g_instance_count = 0;

struct NameField {
  using Type = CountedWrapper<std::string>;
};

struct SchoolsField {
  using Type = CountedWrapper<std::vector<std::string>>;
};

struct AgeField {
  using Type = int;
};

struct Person {
  char id;
  BundledOptionals<NameField, SchoolsField, AgeField> optionals;

  Person() : id(-1) {}
  Person(const Person& other) = default;
  Person(Person&& other) : id(other.id), optionals(std::move(other.optionals)) {
    other.id = -1;
  }
  Person& operator=(const Person& other) = default;
  Person& operator=(Person&& other) {
    if (this != &other) {
      id = other.id;
      optionals = std::move(other.optionals);
      other.id = -1;
    }
    return *this;
  }
};

template <typename T>
static void AssertInstanceCount(int32_t value) {
  EXPECT_EQ(T::Type::g_instance_count, value);
}

TEST(BundledOptional, Empty) {
  Person p;
  EXPECT_FALSE(p.optionals.HasValue<NameField>());
  EXPECT_FALSE(p.optionals.HasValue<SchoolsField>());
  EXPECT_FALSE(p.optionals.HasValue<AgeField>());
  EXPECT_TRUE(p.optionals.GetOrNull<NameField>() == nullptr);
  EXPECT_TRUE(p.optionals.GetOrNull<SchoolsField>() == nullptr);
  EXPECT_TRUE(p.optionals.GetOrNull<AgeField>() == nullptr);

  const Person p2;
  EXPECT_TRUE(p2.optionals.GetOrNull<NameField>() == nullptr);
  EXPECT_TRUE(p2.optionals.GetOrNull<SchoolsField>() == nullptr);
  EXPECT_TRUE(p2.optionals.GetOrNull<AgeField>() == nullptr);

  const Person p3 = p;
  EXPECT_TRUE(p3.optionals.GetOrNull<NameField>() == nullptr);
  EXPECT_TRUE(p3.optionals.GetOrNull<SchoolsField>() == nullptr);
  EXPECT_TRUE(p3.optionals.GetOrNull<AgeField>() == nullptr);

  Person p4(std::move(p));
  EXPECT_TRUE(p4.optionals.GetOrNull<NameField>() == nullptr);
  EXPECT_TRUE(p4.optionals.GetOrNull<SchoolsField>() == nullptr);
  EXPECT_TRUE(p4.optionals.GetOrNull<AgeField>() == nullptr);

  Person p5;
  p5 = p3;
  EXPECT_FALSE(p5.optionals.HasValue<NameField>());
  EXPECT_FALSE(p5.optionals.HasValue<SchoolsField>());
  EXPECT_FALSE(p5.optionals.HasValue<AgeField>());

  Person p6;
  p6 = std::move(p4);
  EXPECT_FALSE(p6.optionals.HasValue<NameField>());
  EXPECT_FALSE(p6.optionals.HasValue<SchoolsField>());
  EXPECT_FALSE(p6.optionals.HasValue<AgeField>());

  AssertInstanceCount<NameField>(0);
  AssertInstanceCount<SchoolsField>(0);
}

TEST(BundledOptional, Construct) {
  {
    Person p_empty;
    Person p_empty2 = p_empty;
    EXPECT_FALSE(p_empty2.optionals.HasValue<NameField>());
    EXPECT_FALSE(p_empty2.optionals.HasValue<SchoolsField>());
    EXPECT_FALSE(p_empty2.optionals.HasValue<AgeField>());

    Person p_empty3(std::move(p_empty));
    EXPECT_FALSE(p_empty3.optionals.HasValue<NameField>());
    EXPECT_FALSE(p_empty3.optionals.HasValue<SchoolsField>());
    EXPECT_FALSE(p_empty3.optionals.HasValue<AgeField>());
  }

  Person p0;
  p0.optionals.Get<NameField>().value = "name0";
  p0.optionals.Get<SchoolsField>().value.push_back("elementary school");
  p0.optionals.Get<SchoolsField>().value.push_back("middle school");
  EXPECT_TRUE(p0.optionals.GetOrNull<NameField>()->value == "name0");
  EXPECT_TRUE(p0.optionals.HasValue<NameField>());
  EXPECT_TRUE(p0.optionals.HasValue<SchoolsField>());
  EXPECT_FALSE(p0.optionals.HasValue<AgeField>());
  AssertInstanceCount<NameField>(1);
  AssertInstanceCount<SchoolsField>(1);

  {
    Person p1(p0);
    EXPECT_TRUE(p1.optionals.HasValue<NameField>());
    EXPECT_TRUE(p1.optionals.HasValue<SchoolsField>());
    EXPECT_FALSE(p1.optionals.HasValue<AgeField>());
    EXPECT_TRUE(p1.optionals.Get<NameField>().value == "name0");
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value.size() == 2);
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value[0] ==
                "elementary school");
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value[1] == "middle school");
    AssertInstanceCount<NameField>(2);
    AssertInstanceCount<SchoolsField>(2);
  }

  {
    Person p1 = p0;
    EXPECT_TRUE(p1.optionals.HasValue<NameField>());
    EXPECT_TRUE(p1.optionals.HasValue<SchoolsField>());
    EXPECT_FALSE(p1.optionals.HasValue<AgeField>());
    EXPECT_TRUE(p1.optionals.Get<NameField>().value == "name0");
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value.size() == 2);
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value[0] ==
                "elementary school");
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value[1] == "middle school");
    AssertInstanceCount<NameField>(2);
    AssertInstanceCount<SchoolsField>(2);
  }

  EXPECT_TRUE(p0.optionals.Get<NameField>().value == "name0");
  EXPECT_TRUE(p0.optionals.Get<SchoolsField>().value.size() == 2);
  EXPECT_TRUE(p0.optionals.Get<SchoolsField>().value[0] == "elementary school");
  EXPECT_TRUE(p0.optionals.Get<SchoolsField>().value[1] == "middle school");
  AssertInstanceCount<NameField>(1);
  AssertInstanceCount<SchoolsField>(1);

  {
    Person p2(std::move(p0));
    EXPECT_TRUE(p2.optionals.HasValue<NameField>());
    EXPECT_TRUE(p2.optionals.HasValue<SchoolsField>());
    EXPECT_FALSE(p2.optionals.HasValue<AgeField>());
    EXPECT_TRUE(p2.optionals.Get<NameField>().value == "name0");
    EXPECT_TRUE(p2.optionals.Get<SchoolsField>().value.size() == 2);
    EXPECT_TRUE(p2.optionals.Get<SchoolsField>().value[0] ==
                "elementary school");
    EXPECT_TRUE(p2.optionals.Get<SchoolsField>().value[1] == "middle school");
    AssertInstanceCount<NameField>(1);
    AssertInstanceCount<SchoolsField>(1);
  }

  EXPECT_FALSE(p0.optionals.HasValue<NameField>());
  EXPECT_FALSE(p0.optionals.HasValue<SchoolsField>());
  AssertInstanceCount<NameField>(0);
  AssertInstanceCount<SchoolsField>(0);
}

TEST(BundledOptional, Assign) {
  {
    Person p_empty;
    Person p_empty2;
    p_empty2.optionals.Get<NameField>().value = "name_empty2";
    p_empty2.optionals.Get<AgeField>() = 13;
    AssertInstanceCount<NameField>(1);
    p_empty2 = p_empty;
    EXPECT_FALSE(p_empty2.optionals.HasValue<NameField>());
    EXPECT_FALSE(p_empty2.optionals.HasValue<SchoolsField>());
    EXPECT_FALSE(p_empty2.optionals.HasValue<AgeField>());
    AssertInstanceCount<NameField>(0);

    Person p_empty3;
    p_empty3.optionals.Get<NameField>().value = "name_empty3";
    p_empty3.optionals.Get<AgeField>() = 13;
    AssertInstanceCount<NameField>(1);
    p_empty3 = std::move(p_empty);
    EXPECT_FALSE(p_empty3.optionals.HasValue<NameField>());
    EXPECT_FALSE(p_empty3.optionals.HasValue<SchoolsField>());
    EXPECT_FALSE(p_empty3.optionals.HasValue<AgeField>());
    EXPECT_FALSE(p_empty.optionals.HasValue<NameField>());
    EXPECT_FALSE(p_empty.optionals.HasValue<SchoolsField>());
    EXPECT_FALSE(p_empty.optionals.HasValue<AgeField>());
    AssertInstanceCount<NameField>(0);
  }

  Person p0;
  p0.optionals.Get<NameField>().value = "name0";
  p0.optionals.Get<SchoolsField>().value.push_back("elementary school");
  p0.optionals.Get<SchoolsField>().value.push_back("middle school");
  EXPECT_TRUE(p0.optionals.HasValue<NameField>());
  EXPECT_TRUE(p0.optionals.HasValue<SchoolsField>());
  EXPECT_FALSE(p0.optionals.HasValue<AgeField>());
  AssertInstanceCount<NameField>(1);
  AssertInstanceCount<SchoolsField>(1);

  {
    Person p1;
    p1.optionals.Get<NameField>().value = "name1";
    p1.optionals.Get<AgeField>() = 13;
    EXPECT_TRUE(p1.optionals.HasValue<AgeField>());
    p1 = p0;
    EXPECT_TRUE(p1.optionals.Get<NameField>().value == "name0");
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value.size() == 2);
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value[0] ==
                "elementary school");
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value[1] == "middle school");
    EXPECT_FALSE(p1.optionals.HasValue<AgeField>());
    p1.optionals.Get<NameField>().value = "name1";
    EXPECT_TRUE(p0.optionals.Get<NameField>().value == "name0");
    AssertInstanceCount<NameField>(2);
    AssertInstanceCount<SchoolsField>(2);
  }

  EXPECT_TRUE(p0.optionals.Get<NameField>().value == "name0");
  EXPECT_TRUE(p0.optionals.Get<SchoolsField>().value.size() == 2);
  EXPECT_TRUE(p0.optionals.Get<SchoolsField>().value[0] == "elementary school");
  EXPECT_TRUE(p0.optionals.Get<SchoolsField>().value[1] == "middle school");
  AssertInstanceCount<NameField>(1);
  AssertInstanceCount<SchoolsField>(1);

  {
    Person p1;
    p1.optionals.Get<NameField>().value = "name1";
    p1.optionals.Get<AgeField>() = 13;
    EXPECT_TRUE(p1.optionals.HasValue<AgeField>());
    p1 = std::move(p0);
    EXPECT_TRUE(p1.optionals.Get<NameField>().value == "name0");
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value.size() == 2);
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value[0] ==
                "elementary school");
    EXPECT_TRUE(p1.optionals.Get<SchoolsField>().value[1] == "middle school");
    EXPECT_FALSE(p1.optionals.HasValue<AgeField>());
    EXPECT_FALSE(p0.optionals.HasValue<NameField>());
    EXPECT_FALSE(p0.optionals.HasValue<SchoolsField>());
    AssertInstanceCount<NameField>(1);
    AssertInstanceCount<SchoolsField>(1);
  }

  AssertInstanceCount<NameField>(0);
  AssertInstanceCount<SchoolsField>(0);
}

TEST(BundledOptional, Release) {
  Person p0;
  p0.optionals.Get<NameField>().value = "name0";
  p0.optionals.Get<SchoolsField>().value.push_back("elementary school");
  p0.optionals.Get<SchoolsField>().value.push_back("middle school");
  EXPECT_TRUE(p0.optionals.HasValue<NameField>());
  EXPECT_TRUE(p0.optionals.HasValue<SchoolsField>());
  EXPECT_FALSE(p0.optionals.HasValue<AgeField>());
  AssertInstanceCount<NameField>(1);
  AssertInstanceCount<SchoolsField>(1);

  p0.optionals.Release<NameField>();
  EXPECT_FALSE(p0.optionals.HasValue<NameField>());
  AssertInstanceCount<NameField>(0);
  EXPECT_TRUE(p0.optionals.HasValue<SchoolsField>());
  EXPECT_FALSE(p0.optionals.bundled_data_ == nullptr);
  p0.optionals.Release<SchoolsField>();
  EXPECT_FALSE(p0.optionals.HasValue<SchoolsField>());
  AssertInstanceCount<SchoolsField>(0);
  EXPECT_TRUE(p0.optionals.bundled_data_ == nullptr);

  p0.optionals.Get<NameField>().value = "name0";
  p0.optionals.Get<SchoolsField>().value.push_back("elementary school");
  p0.optionals.Get<SchoolsField>().value.push_back("middle school");
  p0.optionals.Get<AgeField>() = 13;
  EXPECT_TRUE(p0.optionals.HasValue<NameField>());
  EXPECT_TRUE(p0.optionals.HasValue<SchoolsField>());
  EXPECT_TRUE(p0.optionals.HasValue<AgeField>());
  EXPECT_FALSE(p0.optionals.bundled_data_ == nullptr);
  AssertInstanceCount<NameField>(1);
  AssertInstanceCount<SchoolsField>(1);

  auto name = p0.optionals.ReleaseTransfer<NameField>().value;
  EXPECT_TRUE(name == "name0");
  EXPECT_FALSE(p0.optionals.HasValue<NameField>());
  EXPECT_TRUE(p0.optionals.HasValue<SchoolsField>());
  EXPECT_TRUE(p0.optionals.HasValue<AgeField>());
  AssertInstanceCount<NameField>(0);
  AssertInstanceCount<SchoolsField>(1);

  auto schools = p0.optionals.ReleaseTransfer<SchoolsField>().value;
  EXPECT_TRUE(schools.size() == 2);
  EXPECT_TRUE(schools[0] == "elementary school");
  EXPECT_TRUE(schools[1] == "middle school");
  EXPECT_FALSE(p0.optionals.HasValue<NameField>());
  EXPECT_FALSE(p0.optionals.HasValue<SchoolsField>());
  EXPECT_TRUE(p0.optionals.HasValue<AgeField>());
  AssertInstanceCount<NameField>(0);
  AssertInstanceCount<SchoolsField>(0);

  p0.optionals.Clear();
  EXPECT_FALSE(p0.optionals.HasValue<NameField>());
  EXPECT_FALSE(p0.optionals.HasValue<SchoolsField>());
  EXPECT_FALSE(p0.optionals.HasValue<AgeField>());
  EXPECT_TRUE(p0.optionals.bundled_data_ == nullptr);
}

TEST(BundledOptional, InVector) {
  std::vector<Person> people;
  for (int i = 0; i < 100; i++) {
    people.emplace_back();
    Person& p = people[i];
    p.optionals.Get<NameField>().value =
        std::string("name") + std::to_string(i);
    p.optionals.Get<AgeField>() = i;
  }
  AssertInstanceCount<NameField>(100);
  AssertInstanceCount<SchoolsField>(0);

  for (int i = 0; i < 100; i++) {
    Person& p = people[i];
    EXPECT_TRUE(p.optionals.HasValue<NameField>());
    EXPECT_TRUE(p.optionals.HasValue<AgeField>());
    EXPECT_FALSE(p.optionals.HasValue<SchoolsField>());
    EXPECT_TRUE(p.optionals.Get<NameField>().value ==
                std::string("name") + std::to_string(i));
    EXPECT_TRUE(p.optionals.Get<AgeField>() == i);
  }

  for (int i = 0; i < 100; i++) {
    Person& p = people[i];
    p.optionals.Get<SchoolsField>().value.push_back(std::string("school") +
                                                    std::to_string(i));
  }
  AssertInstanceCount<NameField>(100);
  AssertInstanceCount<SchoolsField>(100);

  for (int i = 0; i < 100; i++) {
    Person& p = people[i];
    EXPECT_TRUE(p.optionals.HasValue<NameField>());
    EXPECT_TRUE(p.optionals.HasValue<AgeField>());
    EXPECT_TRUE(p.optionals.HasValue<SchoolsField>());
    EXPECT_TRUE(p.optionals.Get<NameField>().value ==
                std::string("name") + std::to_string(i));
    EXPECT_TRUE(p.optionals.Get<SchoolsField>().value[0] ==
                std::string("school") + std::to_string(i));
    EXPECT_TRUE(p.optionals.Get<AgeField>() == i);
  }

  for (int i = 0; i < 100; i++) {
    Person& p = people[i];
    p.optionals.Release<NameField>();
  }
  AssertInstanceCount<NameField>(0);
  AssertInstanceCount<SchoolsField>(100);
  for (int i = 0; i < 100; i++) {
    Person& p = people[i];
    EXPECT_FALSE(p.optionals.HasValue<NameField>());
    EXPECT_TRUE(p.optionals.HasValue<AgeField>());
    EXPECT_TRUE(p.optionals.HasValue<SchoolsField>());
    EXPECT_TRUE(p.optionals.Get<SchoolsField>().value[0] ==
                std::string("school") + std::to_string(i));
    EXPECT_TRUE(p.optionals.Get<AgeField>() == i);
  }

  people.back().optionals.Clear();
  AssertInstanceCount<NameField>(0);
  AssertInstanceCount<SchoolsField>(99);

  people.erase(people.begin());
  AssertInstanceCount<NameField>(0);
  AssertInstanceCount<SchoolsField>(98);

  people.clear();
  AssertInstanceCount<NameField>(0);
  AssertInstanceCount<SchoolsField>(0);
}

}  // namespace test
}  // namespace base
}  // namespace lynx
