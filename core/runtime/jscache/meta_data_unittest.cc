// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/meta_data.h"

#if !defined(_MSC_VER)
#include <cxxabi.h>
#endif

#include <iostream>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace cache {
namespace testing {

TEST(BytecodeMetaData, Create) {
  MetaData meta_data("2.4", "2.2.0-inspector");
  EXPECT_EQ(
      meta_data.ToJson(),
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"}})");
  auto meta2 = MetaData::ParseJson(meta_data.ToJson());
  EXPECT_EQ(
      meta2->ToJson(),
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"}})");
  EXPECT_EQ(meta2->GetLynxVersion(), "2.4");
  EXPECT_EQ(meta2->GetBytecodeGenerateEngineVersion(), "2.2.0-inspector");
}

TEST(BytecodeMetaData, Create2) {
  auto meta_broken_str =
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"})";
  auto meta2 = MetaData::ParseJson(meta_broken_str);
  EXPECT_EQ(meta2, nullptr);
}

TEST(BytecodeMetaData, Create3) {
  auto meta_broken_str = R"({"lynx":{"version":"2.4"}})";
  auto meta2 = MetaData::ParseJson(meta_broken_str);
  EXPECT_EQ(meta2, nullptr);
}

TEST(BytecodeMetaData, Create4) {
  auto meta_broken_str = R"({"lynx":{"version":"2.4"}})";
  auto meta2 = MetaData::ParseJson(meta_broken_str);
  EXPECT_EQ(meta2, nullptr);
}

TEST(BytecodeMetaData, UpdateFileInfoPackaged) {
  MetaData meta_data("2.4", "2.2.0-inspector");
  JsFileIdentifier id;
  id.category = MetaData::PACKAGED;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.js";
  meta_data.UpdateFileInfo(id, "md5_1", 1024);

  id.url = "/app-service-2.js";
  meta_data.UpdateFileInfo(id, "md5_2", 1025);
  std::cout << meta_data.ToJson() << std::endl;

  auto info = meta_data.GetFileInfo(id);
  EXPECT_EQ(info.has_value(), true);
  EXPECT_EQ(info->cache_size, static_cast<uint64_t>(1025));
  EXPECT_EQ(info->md5, "md5_2");
  EXPECT_NE(info->last_accessed, 0);
}

TEST(BytecodeMetaData, UpdateFileInfoPackaged2) {
  MetaData meta_data("2.4", "2.2.0-inspector");
  JsFileIdentifier id;
  id.category = MetaData::DYNAMIC;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.js";
  meta_data.UpdateFileInfo(id, "md5_1", 1024);
  meta_data.UpdateFileInfo(id, "md5_2", 2048);
  std::cout << meta_data.ToJson() << std::endl;

  auto info = meta_data.GetFileInfo(id);
  EXPECT_EQ(info->cache_size, static_cast<uint64_t>(2048));
  EXPECT_EQ(info->md5, "md5_2");
  EXPECT_NE(info->last_accessed, 0);
}

TEST(BytecodeMetaData, UpdateFileInfoCoreJs) {
  MetaData meta_data("2.4", "2.2.0-inspector");
  JsFileIdentifier id;
  id.category = MetaData::CORE_JS;
  meta_data.UpdateFileInfo(id, "md5_1", 1024);
  std::cout << meta_data.ToJson() << std::endl;

  auto info = meta_data.GetFileInfo(id);
  EXPECT_EQ(info->cache_size, static_cast<uint64_t>(1024));
  EXPECT_EQ(info->md5, "md5_1");
  EXPECT_NE(info->last_accessed, 0);
}

TEST(BytecodeMetaData, UpdateFileInfoDynamic) {
  MetaData meta_data("2.4", "2.2.0-inspector");
  JsFileIdentifier id;
  id.category = MetaData::DYNAMIC;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.js";
  meta_data.UpdateFileInfo(id, "md5_1", 1024);
  std::cout << meta_data.ToJson() << std::endl;

  auto info = meta_data.GetFileInfo(id);
  EXPECT_EQ(info->cache_size, static_cast<uint64_t>(1024));
  EXPECT_EQ(info->md5, "md5_1");
  EXPECT_NE(info->last_accessed, 0);
}

TEST(BytecodeMetaData, UpdateFileInfoDynamic2) {
  MetaData meta_data("2.4", "2.2.0-inspector");
  JsFileIdentifier id;
  id.category = MetaData::DYNAMIC;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.js";
  meta_data.UpdateFileInfo(id, "md5_1", 1024);

  id.url = "/app-service-2.js";
  meta_data.UpdateFileInfo(id, "md5_2", 1026);
  std::cout << meta_data.ToJson() << std::endl;

  auto info = meta_data.GetFileInfo(id);
  EXPECT_EQ(info->cache_size, static_cast<uint64_t>(1026));
  EXPECT_EQ(info->md5, "md5_2");
  EXPECT_NE(info->last_accessed, 0);
}

TEST(BytecodeMetaData, UpdateLastAccessTime) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service.js":{"md5":"md5_1","cache_size":1024,"last_accessed":1646304382}}}}})");
  JsFileIdentifier id;
  id.category = MetaData::PACKAGED;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.js";

  auto old_timestamp = meta_data->GetFileInfo(id)->last_accessed;
  meta_data->UpdateLastAccessTimeIfExists(id);
  std::cout << meta_data->ToJson() << std::endl;

  auto info = meta_data->GetFileInfo(id);
  EXPECT_EQ(info->cache_size, static_cast<uint64_t>(1024));
  EXPECT_EQ(info->md5, "md5_1");
  EXPECT_NE(info->last_accessed, old_timestamp);
}

TEST(BytecodeMetaData, UpdateLastAccessTimeToNotExistedPackagedRecord) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service.js":{"md5":"md5_1","cache_size":1024,"last_accessed":1646304382}}}}})");
  JsFileIdentifier id;
  id.category = MetaData::PACKAGED;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.jsx";

  EXPECT_FALSE(meta_data->UpdateLastAccessTimeIfExists(id));

  id.template_url = "http://www.xxx.com/template.jsx";
  id.url = "/app-service.js";
  EXPECT_FALSE(meta_data->UpdateLastAccessTimeIfExists(id));
}

TEST(BytecodeMetaData, UpdateLastAccessTimeToNotExistedDynamicRecord) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service.js":{"md5":"md5_1","cache_size":1024,"last_accessed":1646304382}}}}})");
  JsFileIdentifier id;
  id.category = MetaData::DYNAMIC;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.js";

  EXPECT_FALSE(meta_data->UpdateLastAccessTimeIfExists(id));
}

TEST(BytecodeMetaData, GetFileInfo) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service.js":{"md5":"md5_1","cache_size":1024,"last_accessed":1646304382}}}}})");
  JsFileIdentifier id;
  id.category = MetaData::PACKAGED;
  id.template_url = "http://www.xxx.com/template.jsx";
  id.url = "/app-service.js";

  auto info = meta_data->GetFileInfo(id);
  EXPECT_EQ(info.has_value(), false);
}

TEST(BytecodeMetaData, RemoveFileInfo) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service.js":{"md5":"md5_x","cache_size":1024,"last_accessed":1146304382}}}}})");
  JsFileIdentifier id;
  id.category = MetaData::PACKAGED;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.js";

  meta_data->RemoveFileInfo(id);
  EXPECT_EQ(
      meta_data->ToJson(),
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{}}})");
}

TEST(BytecodeMetaData, RemoveFileInfo2) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service.js":{"md5":"md5_x","cache_size":1024,"last_accessed":1146304382}}}}})");
  JsFileIdentifier id;
  id.category = MetaData::PACKAGED;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.jsx";

  meta_data->RemoveFileInfo(id);
  EXPECT_EQ(
      meta_data->ToJson(),
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service.js":{"md5":"md5_x","cache_size":1024,"last_accessed":1146304382}}}}})");
}

TEST(BytecodeMetaData, RemoveFileInfo3) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service.js":{"md5":"md5_1","cache_size":1024,"last_accessed":1646386614},"/app-service-2.js":{"md5":"md5_2","cache_size":1025,"last_accessed":1646386614}}}}})");
  JsFileIdentifier id;
  id.category = MetaData::PACKAGED;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.js";

  meta_data->RemoveFileInfo(id);
  std::cout << meta_data->ToJson() << std::endl;
  EXPECT_EQ(
      meta_data->ToJson(),
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service-2.js":{"md5":"md5_2","cache_size":1025,"last_accessed":1646386614}}}}})");
}

TEST(BytecodeMetaData, RemoveFileInfoCoreJs) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"core_js":{"md5":"md5_1","cache_size":1024,"last_accessed":1646386790}}})");
  JsFileIdentifier id;
  id.category = MetaData::CORE_JS;

  meta_data->RemoveFileInfo(id);
  std::cout << meta_data->ToJson() << std::endl;
  EXPECT_EQ(
      meta_data->ToJson(),
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{}})");
}

TEST(BytecodeMetaData, RemoveFileInfoCoreJs2) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"core_js":{"md5":"md5_1","cache_size":1024,"last_accessed":1646386790}}})");
  JsFileIdentifier id;
  id.category = MetaData::CORE_JS;

  meta_data->RemoveFileInfo(id);
  std::cout << meta_data->ToJson() << std::endl;
  EXPECT_EQ(
      meta_data->ToJson(),
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{}})");
}

TEST(BytecodeMetaData, GetAllCacheFileInfo) {
  auto meta_data = MetaData::ParseJson(
      R"({"lynx":{"version":"2.4","engine_sdk_version":"2.2.0-inspector"},"cache_files":{"packaged":{"http://www.xxx.com/template.js":{"/app-service.js":{"md5":"md5_x","cache_size":1024,"last_accessed":1146304382}}}}})");
  JsFileIdentifier id;
  id.category = MetaData::DYNAMIC;
  id.template_url = "http://www.xxx.com/template.js";
  id.url = "/app-service.js";

  meta_data->UpdateFileInfo(id, "md5_1", 1024);

  JsFileIdentifier id2;
  id2.category = MetaData::PACKAGED;
  id2.template_url = "http://www.foo.com/template.js";
  id2.url = "/app-service.js";

  meta_data->UpdateFileInfo(id2, "md5_foo", 1024);

  auto info_vec = meta_data->GetAllCacheFileInfo();
  EXPECT_EQ(info_vec.size(), static_cast<size_t>(3));
  EXPECT_EQ(info_vec[0].md5, "md5_x");
  EXPECT_EQ(info_vec[1].md5, "md5_foo");
  EXPECT_EQ(info_vec[2].md5, "md5_1");

  auto filter_info_vec =
      meta_data->GetAllCacheFileInfo("http://www.foo.com/template.js");
  EXPECT_EQ(filter_info_vec.size(), 1);
  EXPECT_EQ(filter_info_vec[0].md5, "md5_foo");
}
}  // namespace testing
}  // namespace cache
}  // namespace piper
}  // namespace lynx
