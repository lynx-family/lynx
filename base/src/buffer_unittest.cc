// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/buffer.h"

#include <cstdlib>
#include <cstring>

#include "base/include/path_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#if OS_WIN
static const char kChars[] =
    "0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static char *mkdtemp(char *tmpl) {
  int len = (int)strlen(tmpl);
  if (len < 6 || strcmp(&tmpl[len - 6], "XXXXXX")) {
    return NULL;
  }
  char *XXXXXX = &tmpl[len - 6];
  unsigned long num = rand();
  for (int i = 0; i < 1000; i++) {
    unsigned long v = num;
    for (int j = 0; j < 6; j++) {
      XXXXXX[j] = kChars[v % 63];
      v /= 63;
    }
    int fd = _mkdir(tmpl);
    if (fd == 0) {
      return tmpl;
    }
    if (errno != EEXIST) {
      break;
    }
    num += 137;
  }
  return NULL;
}
#endif

namespace lynx {
namespace base {
namespace testing {

class BufferTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() {
    // make temp testing dir
    char *dir = mkdtemp(path.data());
    ASSERT_NE(dir, nullptr);
  }

  static void TearDownTestSuite() {
    // cleanup testing dir
#if OS_WIN
    int ret = _rmdir(path.data());
#else
    int ret = remove(path.data());
#endif
    ASSERT_EQ(ret, 0);
  }

  static std::string path;

 protected:
  void SetUp() override {
    file_path_ = PathUtils::JoinPaths(
        {path,
         ::testing::UnitTest::GetInstance()->current_test_info()->name()});
  }

  void TearDown() override { remove(file_path_.c_str()); }

  std::string file_path_;
};

std::string BufferTest::path = "BufferTestXXXXXX";

TEST_F(BufferTest, ReadWriteFile) {
  StringBuffer write_data("FirstFirstFirstWriteData");
  ASSERT_TRUE(write_data.WriteToFile(file_path_));
  auto read_data = Buffer::ReadFromFile(file_path_);
  ASSERT_NE(read_data, nullptr);
  ASSERT_EQ(read_data->size(), write_data.size());
  ASSERT_EQ(memcmp(read_data->data(), write_data.data(), write_data.size()), 0);
  // Script consumers can also use the terminating NUL.
  ASSERT_EQ(read_data->data()[read_data->size()], 0);
}

TEST_F(BufferTest, ReadWriteBinaryFile) {
  std::vector<uint8_t> bytes;
  for (size_t i = 0; i < 256; ++i) {
    bytes.push_back(static_cast<uint8_t>(i));
  }
  ByteBuffer write_data(
      std::make_shared<const std::vector<uint8_t>>(std::move(bytes)));
  ASSERT_TRUE(write_data.WriteToFile(file_path_));
  auto read_data = Buffer::ReadFromFile(file_path_);
  ASSERT_NE(read_data, nullptr);
  ASSERT_EQ(read_data->size(), write_data.size());
  ASSERT_EQ(memcmp(read_data->data(), write_data.data(), write_data.size()), 0);
}

TEST_F(BufferTest, ReadWriteEmptyFile) {
  ASSERT_TRUE(ByteRefBuffer(nullptr, 0).WriteToFile(file_path_));
  auto read_data = Buffer::ReadFromFile(file_path_, 0);
  ASSERT_NE(read_data, nullptr);
  ASSERT_EQ(read_data->size(), 0u);
}

TEST_F(BufferTest, WriteFileToExisted) {
  ASSERT_TRUE(StringBuffer("FirstFirstFirstWriteData").WriteToFile(file_path_));
  const std::string write_data = "AnotherWriteData";
  ASSERT_TRUE(StringRefBuffer(write_data).WriteToFile(file_path_));
  auto read_data = Buffer::ReadFromFile(file_path_);
  ASSERT_NE(read_data, nullptr);
  ASSERT_EQ(read_data->size(), write_data.size());
  ASSERT_EQ(memcmp(read_data->data(), write_data.data(), write_data.size()), 0);
}

TEST_F(BufferTest, WriteToNonExistedDir) {
  auto file_path = PathUtils::JoinPaths({path, "not_existed", "ReadWriteFile"});
  ASSERT_FALSE(StringBuffer("WriteData").WriteToFile(file_path));
}

TEST_F(BufferTest, ReadNonExistedFile) {
  ASSERT_EQ(Buffer::ReadFromFile(file_path_), nullptr);
}

TEST_F(BufferTest, ReadFileTooLarge) {
  StringBuffer write_data("FirstFirstFirstWriteData");
  ASSERT_TRUE(write_data.WriteToFile(file_path_));
  ASSERT_EQ(Buffer::ReadFromFile(file_path_, write_data.size() - 1), nullptr);
}

TEST_F(BufferTest, ReadFileAtSizeLimit) {
  const uint8_t bytes[] = {0, 1, 127, 255};
  ASSERT_TRUE(ByteRefBuffer(bytes, sizeof(bytes)).WriteToFile(file_path_));
  auto read_data = Buffer::ReadFromFile(file_path_, sizeof(bytes));
  ASSERT_NE(read_data, nullptr);
  ASSERT_EQ(read_data->size(), sizeof(bytes));
  ASSERT_EQ(memcmp(read_data->data(), bytes, sizeof(bytes)), 0);
}

TEST_F(BufferTest, InvalidBufferDoesNotTruncateFile) {
  StringBuffer write_data("WriteData");
  ASSERT_TRUE(write_data.WriteToFile(file_path_));
  ASSERT_FALSE(ByteRefBuffer(nullptr, 1).WriteToFile(file_path_));
  auto read_data = Buffer::ReadFromFile(file_path_);
  ASSERT_NE(read_data, nullptr);
  ASSERT_EQ(read_data->size(), write_data.size());
  ASSERT_EQ(memcmp(read_data->data(), write_data.data(), write_data.size()), 0);
}

}  // namespace testing
}  // namespace base
}  // namespace lynx
