// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/base/file_stream.h"

#include <fcntl.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include "base/include/file_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace {

class FileStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    directory_ =
        std::filesystem::temp_directory_path() /
        std::filesystem::u8path(u8"lynx file stream \u5e94\u7528 \U0001f680");
    std::filesystem::remove_all(directory_);
    std::filesystem::create_directories(directory_);
    path_ = (directory_ /
             std::filesystem::u8path(u8"\u4e8c\u8fdb\u5236 trace \U0001f680"))
                .u8string();
    std::ofstream output(std::filesystem::u8path(path_), std::ios::binary);
    ASSERT_TRUE(output.is_open());
    output.write(payload_.data(), payload_.size());
    ASSERT_TRUE(output.good());
  }

  void TearDown() override {
    for (int handle : handles_) {
      FileStream::Close(handle);
    }
    std::filesystem::remove_all(directory_);
  }

  int Open(std::ios::openmode mode = std::ios::in | std::ios::binary) {
    const auto handle = FileStream::Open(path_, mode);
    handles_.push_back(handle);
    return handle;
  }

  std::filesystem::path directory_;
  std::string path_;
  const std::string payload_{"abc\0\x1a\r\nxyz", 10};
  std::vector<int> handles_;
};

TEST_F(FileStreamTest, ReadsUnicodeBinaryFileInChunks) {
  const auto handle = Open();
  ASSERT_GT(handle, 0);
  char buffer[4] = {};
  ASSERT_EQ(FileStream::Read(handle, buffer, sizeof(buffer)), 4);
  EXPECT_EQ(std::string(buffer, sizeof(buffer)), payload_.substr(0, 4));
  std::ostringstream output;
  EXPECT_EQ(FileStream::Read(handle, output, 4), 4);
  EXPECT_EQ(FileStream::Read(handle, output, 4), 2);
  EXPECT_EQ(output.str(), payload_.substr(4));
  EXPECT_EQ(FileStream::Read(handle, buffer, sizeof(buffer)), 0);
  EXPECT_EQ(FileStream::Read(handle, output, 4), 0);
  EXPECT_EQ(output.str(), payload_.substr(4));
}

TEST_F(FileStreamTest, ClosedAndInvalidHandlesReturnError) {
  const auto first = Open();
  const auto second = Open();
  ASSERT_GT(first, 0);
  ASSERT_GT(second, first);
  FileStream::Close(first);
  FileStream::Close(first);
  char buffer[16] = {};
  EXPECT_EQ(FileStream::Read(first, buffer, sizeof(buffer)), -1);
  EXPECT_EQ(FileStream::Read(-1, buffer, sizeof(buffer)), -1);
  std::ostringstream output;
  EXPECT_EQ(FileStream::Read(first, output, sizeof(buffer)), -1);
  EXPECT_TRUE(output.str().empty());
  ASSERT_EQ(FileStream::Read(second, buffer, sizeof(buffer)), 10);
  EXPECT_EQ(std::string(buffer, payload_.size()), payload_);
}

TEST_F(FileStreamTest, MissingFilesAndUnsupportedModesFail) {
  EXPECT_EQ(FileStream::Open(path_ + "/missing"), -1);
  EXPECT_EQ(FileStream::Open(""), -1);
  EXPECT_EQ(Open(std::ios::openmode{}), -1);
  EXPECT_EQ(Open(std::ios::trunc), -1);
  EXPECT_EQ(Open(std::ios::out | std::ios::app | std::ios::trunc), -1);
  EXPECT_EQ(std::filesystem::file_size(std::filesystem::u8path(path_)),
            payload_.size());
}

TEST_F(FileStreamTest, EmptyFilesAndZeroLengthReadsPreserveEOF) {
  const auto handle = Open();
  ASSERT_GT(handle, 0);
  char buffer[16] = {};
  EXPECT_EQ(FileStream::Read(handle, buffer, 0), 0);
  ASSERT_EQ(FileStream::Read(handle, buffer, sizeof(buffer)), 10);
  EXPECT_EQ(std::string(buffer, payload_.size()), payload_);
  FileStream::Close(handle);

  std::ofstream empty(std::filesystem::u8path(path_),
                      std::ios::binary | std::ios::trunc);
  empty.close();
  const auto empty_handle = Open();
  ASSERT_GT(empty_handle, 0);
  EXPECT_EQ(FileStream::Read(empty_handle, buffer, sizeof(buffer)), 0);
  EXPECT_EQ(FileStream::Read(empty_handle, buffer, sizeof(buffer)), 0);
}

TEST_F(FileStreamTest, SupportedModesMatchStandardStreamPosition) {
  const std::ios::openmode modes[] = {
      std::ios::in, std::ios::in | std::ios::out, std::ios::in | std::ios::app,
      std::ios::in | std::ios::out | std::ios::app};
  for (auto mode : modes) {
    SCOPED_TRACE(static_cast<int>(mode));
    std::fstream reference(std::filesystem::u8path(path_), mode);
    char expected[16] = {};
    reference.read(expected, sizeof(expected));
    const auto handle = Open(mode);
    ASSERT_GT(handle, 0);
    char buffer[16] = {};
    ASSERT_EQ(FileStream::Read(handle, buffer, sizeof(buffer)),
              reference.gcount());
    EXPECT_EQ(std::string(buffer, reference.gcount()),
              std::string(expected, reference.gcount()));
  }
  const auto at_end = Open(std::ios::in | std::ios::binary | std::ios::ate);
  ASSERT_GT(at_end, 0);
  char buffer[16] = {};
  EXPECT_EQ(FileStream::Read(at_end, buffer, sizeof(buffer)), 0);
}

TEST_F(FileStreamTest, TruncateModeCreatesEmptyOutput) {
  const auto handle = Open(std::ios::in | std::ios::out | std::ios::trunc);
  ASSERT_GT(handle, 0);
  char buffer[16] = {};
  EXPECT_EQ(FileStream::Read(handle, buffer, sizeof(buffer)), 0);
  EXPECT_EQ(std::filesystem::file_size(std::filesystem::u8path(path_)), 0u);
}

TEST_F(FileStreamTest, FileDescriptorPreservesUnicodePathAndBinaryContent) {
  const auto fd =
      base::OpenFileDescriptor(path_, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  ASSERT_GE(fd, 0);
#if defined(_WIN32)
  const auto written = _write(fd, payload_.data(), payload_.size());
  const auto closed = _close(fd);
#else
  const auto written = write(fd, payload_.data(), payload_.size());
  const auto closed = close(fd);
#endif
  EXPECT_EQ(written, static_cast<decltype(written)>(payload_.size()));
  EXPECT_EQ(closed, 0);
  std::ifstream input(std::filesystem::u8path(path_), std::ios::binary);
  const std::string actual((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
  EXPECT_EQ(actual, payload_);
  EXPECT_EQ(base::OpenFileDescriptor(path_ + "/missing", O_RDONLY, 0), -1);
}

}  // namespace
}  // namespace devtool
}  // namespace lynx
