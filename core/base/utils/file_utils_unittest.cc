// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/utils/file_utils.h"

#include <cstdlib>

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

class FileUtilsTest : public ::testing::Test {
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
};

std::string FileUtilsTest::path = "FileUtilsTestXXXXXX";

TEST_F(FileUtilsTest, ReadWriteFile) {
  std::string filename = "ReadWriteFile";
  std::string file_path = PathUtils::JoinPaths({path, filename});
  std::string write_data = "FirstFirstFirstWriteData";
  bool ret = FileUtils::WriteFileBinary(
      file_path, (unsigned char *)write_data.data(), write_data.size());
  ASSERT_TRUE(ret);
  std::string read_data;
  ret = FileUtils::ReadFileBinary(file_path, 10'000'000, read_data);
  ASSERT_TRUE(ret);
  ASSERT_EQ(read_data, write_data);
}

TEST_F(FileUtilsTest, ReadWriteEmptyFile) {
  std::string filename = "ReadWriteEmptyFile";
  std::string file_path = PathUtils::JoinPaths({path, filename});
  std::string write_data = "";
  bool ret = FileUtils::WriteFileBinary(
      file_path, (unsigned char *)write_data.data(), write_data.size());
  ASSERT_TRUE(ret);
  std::string read_data;
  ret = FileUtils::ReadFileBinary(file_path, 10'000'000, read_data);
  ASSERT_TRUE(ret);
  ASSERT_EQ(read_data, write_data);
  remove(file_path.data());
}

TEST_F(FileUtilsTest, WriteFileToExisted) {
  std::string filename = "ReadWriteFile";
  std::string file_path = PathUtils::JoinPaths({path, filename});
  std::string write_data = "AnotherWriteData";
  bool ret = FileUtils::WriteFileBinary(
      file_path, (unsigned char *)write_data.data(), write_data.size());
  ASSERT_TRUE(ret);
  std::string read_data;
  ret = FileUtils::ReadFileBinary(file_path, 10'000'000, read_data);
  ASSERT_TRUE(ret);
  ASSERT_EQ(read_data, write_data);
  remove(file_path.data());
}

TEST_F(FileUtilsTest, WriteToNonExistedDir) {
  std::string filename = "ReadWriteFile";
  std::string file_path = PathUtils::JoinPaths({path, "not_existed", filename});
  std::string write_data = "WriteData";
  bool ret = FileUtils::WriteFileBinary(
      file_path, (unsigned char *)write_data.data(), write_data.size());
  ASSERT_FALSE(ret);
}

TEST_F(FileUtilsTest, ReadNonExistedFile) {
  std::string filename = "ReadWriteFile";
  std::string file_path = PathUtils::JoinPaths({path, "not_existed", filename});
  std::string read_data;
  bool ret = FileUtils::ReadFileBinary(file_path, 10'000'000, read_data);
  ASSERT_FALSE(ret);
}

TEST_F(FileUtilsTest, ReadFileTooLarge) {
  std::string filename = "ReadWriteFile";
  std::string file_path = PathUtils::JoinPaths({path, filename});
  std::string write_data = "FirstFirstFirstWriteData";
  bool ret = FileUtils::WriteFileBinary(
      file_path, (unsigned char *)write_data.data(), write_data.size());
  ASSERT_TRUE(ret);
  std::string read_data;
  ret = FileUtils::ReadFileBinary(file_path, 10, read_data);
  ASSERT_FALSE(ret);
  remove(file_path.data());
}

}  // namespace testing
}  // namespace base
}  // namespace lynx
