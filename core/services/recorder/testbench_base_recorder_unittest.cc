// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>

#define private public
#include "core/services/recorder/testbench_base_recorder.h"
#undef private

#include <mutex>

#include "core/services/recorder/fixture_writer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/modp_b64/modp_b64.h"
#include "third_party/zlib/zlib.h"

namespace lynx {
namespace tasm {
namespace recorder {

void wait(fml::Thread& thread) {
  std::condition_variable condition;
  std::mutex local_mutex;
  std::unique_lock<std::mutex> lock(local_mutex);
  thread.GetTaskRunner()->PostTask([&condition, &local_mutex]() {
    std::unique_lock<std::mutex> lock(local_mutex);
    condition.notify_one();
  });
  condition.wait(lock);
}

namespace {

// Reverses CompressToBase64String's encode half (base64 -> compressed bytes).
std::string DecodeBase64(const std::string& input) {
  std::string out(lynx_modp_b64_decode_len(input.size()), '\0');
  const size_t decoded_len = lynx_modp_b64_decode(
      &out[0], input.c_str(), static_cast<int>(input.size()));
  if (decoded_len == MODP_B64_ERROR) {
    return "";
  }
  out.resize(decoded_len);
  return out;
}

// Reverses CompressToBase64String's compress half (zlib wrapper stream).
// Grows the output buffer until the payload fits.
std::string ZlibDecompress(const std::string& compressed) {
  if (compressed.empty()) {
    return "";
  }
  std::string out(compressed.size() * 8 + 1024, '\0');
  for (int attempt = 0; attempt < 8; ++attempt) {
    uLongf dest_len = static_cast<uLongf>(out.size());
    const int ret =
        uncompress(reinterpret_cast<Bytef*>(&out[0]), &dest_len,
                   reinterpret_cast<const Bytef*>(compressed.data()),
                   static_cast<uLong>(compressed.size()));
    if (ret == Z_OK) {
      out.resize(dest_len);
      return out;
    }
    if (ret != Z_BUF_ERROR) {
      return "";
    }
    out.resize(out.size() * 4);
  }
  return "";
}

int64_t NowMillis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::string ReadFileContent(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

TEST(TestBenchBaseRecorder, RecordScripts) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  rapidjson::Value& scripts_table = ark.GetRecordedFile(record_id)[kScripts];
  ASSERT_TRUE(scripts_table.IsObject());
  EXPECT_EQ(scripts_table.MemberCount(), 0);
  // enable record
  ark.is_recording_ = true;
  std::string url = "url";
  std::string content = "content";
  ark.RecordScripts(url.c_str(), content.c_str(), record_id);
  wait(ark.thread_);

  ASSERT_TRUE(scripts_table.IsObject());
  EXPECT_EQ(scripts_table.MemberCount(), 1);
  ASSERT_TRUE(scripts_table.HasMember(url));
  EXPECT_STREQ((scripts_table[url]).GetString(), "eJxLzs8rSc0rAQALywL8");
}

void CheckLynxViewTable(TestBenchBaseRecorder& ark, int64_t record_id) {
  rapidjson::Value& recorded_file = ark.lynx_view_table_[record_id];

  ASSERT_TRUE(recorded_file.IsObject());
  EXPECT_EQ(recorded_file.MemberCount(), 7);

  rapidjson::Value& action_list = recorded_file[kActionList];
  ASSERT_TRUE(action_list.IsArray());
  EXPECT_EQ(action_list.Size(), 0);

  rapidjson::Value& invoked_method_data = recorded_file[kInvokedMethodData];
  ASSERT_TRUE(invoked_method_data.IsArray());
  EXPECT_EQ(invoked_method_data.Size(), 0);

  rapidjson::Value& callback = recorded_file[kCallback];
  ASSERT_TRUE(callback.IsObject());
  EXPECT_EQ(callback.MemberCount(), 0);

  rapidjson::Value& component_list_value = recorded_file[kComponentList];
  ASSERT_TRUE(component_list_value.IsArray());
  EXPECT_EQ(component_list_value.Size(), 0);

  rapidjson::Value& debugInfo = recorded_file[kDebugInfo];
  ASSERT_TRUE(debugInfo.IsArray());
  EXPECT_EQ(debugInfo.Size(), 0);

  rapidjson::Value& shared_data = recorded_file[kSharedData];
  ASSERT_TRUE(shared_data.IsObject());
  EXPECT_EQ(shared_data.MemberCount(), 0);

  rapidjson::Value& scripts = recorded_file[kScripts];
  ASSERT_TRUE(scripts.IsObject());
  EXPECT_EQ(scripts.MemberCount(), 0);
}

TEST(TestBenchBaseRecorder, Clear) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;

  ark.GetRecordedFile(record_id);
  ark.SetScreenSize(record_id, 123, 456);
  wait(ark.thread_);
  ark.AddLynxViewSessionID(record_id, 42);
  ark.url_map_[record_id] = "url";

  EXPECT_EQ(ark.lynx_view_table_.size(), 1);
  EXPECT_EQ(ark.replay_config_map_.size(), 1);
  EXPECT_EQ(ark.url_map_.size(), 1);
  EXPECT_EQ(ark.session_ids_.size(), 1);

  ark.Clear();

  EXPECT_EQ(ark.lynx_view_table_.size(), 0);
  EXPECT_EQ(ark.replay_config_map_.size(), 0);
  EXPECT_EQ(ark.url_map_.size(), 0);
  EXPECT_EQ(ark.session_ids_.size(), 0);
}

TEST(TestBenchBaseRecorder, RemoveRecord) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;

  ark.GetRecordedFile(record_id);
  ark.SetScreenSize(record_id, 123, 456);
  wait(ark.thread_);
  ark.AddLynxViewSessionID(record_id, 42);

  EXPECT_EQ(ark.lynx_view_table_.count(record_id), 1);
  EXPECT_EQ(ark.replay_config_map_.count(record_id), 1);
  EXPECT_EQ(ark.session_ids_.count(record_id), 1);

  ark.RemoveRecord(record_id);
  wait(ark.thread_);

  EXPECT_EQ(ark.lynx_view_table_.count(record_id), 0);
  EXPECT_EQ(ark.replay_config_map_.count(record_id), 0);
  EXPECT_EQ(ark.url_map_.count(record_id), 0);
  EXPECT_EQ(ark.session_ids_.count(record_id), 0);

  ark.Clear();
}

TEST(TestBenchBaseRecorder, CreateRecordedFile) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  EXPECT_EQ(ark.lynx_view_table_.size(), 0);
  int64_t record_id = 1;
  ark.CreateRecordedFile(record_id);
  EXPECT_EQ(ark.lynx_view_table_.size(), 1);
  CheckLynxViewTable(ark, record_id);
  ark.Clear();
}

TEST(TestBenchBaseRecorder, GetRecordedFile) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  ark.GetRecordedFile(1);
  CheckLynxViewTable(ark, 1);
  EXPECT_EQ(ark.lynx_view_table_.size(), 1);
  ark.GetRecordedFile(2);
  CheckLynxViewTable(ark, 2);
  EXPECT_EQ(ark.lynx_view_table_.size(), 2);
  ark.Clear();
}

TEST(TestBenchBaseRecorder, GetRecordedFileField) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  rapidjson::Value& value = ark.GetRecordedFileField(record_id, kActionList);
  ASSERT_TRUE(value.IsArray());
  EXPECT_EQ(value.Size(), 0);

  value = ark.GetRecordedFileField(record_id, kInvokedMethodData);
  ASSERT_TRUE(value.IsArray());
  EXPECT_EQ(value.Size(), 0);

  value = ark.GetRecordedFileField(record_id, kCallback);
  ASSERT_TRUE(value.IsObject());
  EXPECT_EQ(value.MemberCount(), 0);

  value = ark.GetRecordedFileField(record_id, kComponentList);
  ASSERT_TRUE(value.IsArray());
  EXPECT_EQ(value.Size(), 0);
  ark.Clear();
}

TEST(TestBenchBaseRecorder, SetRecorderPath) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  ark.SetRecorderPath("/your/local/path");
  EXPECT_STREQ(ark.file_path_.c_str(), "/your/local/path/");
}

TEST(TestBenchBaseRecorder, SetScreenSize) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  ark.SetScreenSize(record_id, 123, 456);
  wait(ark.thread_);
  EXPECT_EQ(ark.replay_config_map_.size(), 1);

  rapidjson::Value& config = ark.replay_config_map_[record_id];
  ASSERT_TRUE(config.IsObject());
  EXPECT_EQ(config.MemberCount(), 4);

  rapidjson::Value& jsb_ignored_info = config["jsbIgnoredInfo"];
  ASSERT_TRUE(jsb_ignored_info.IsArray());
  EXPECT_EQ(jsb_ignored_info.Size(), 0);

  rapidjson::Value& jsb_settings = config["jsbSettings"];
  ASSERT_TRUE(jsb_settings.IsObject());
  EXPECT_EQ(jsb_settings.MemberCount(), 1);

  rapidjson::Value& strict = jsb_settings["strict"];
  ASSERT_TRUE(strict.IsBool());
  ASSERT_TRUE(strict.GetBool());
}

TEST(TestBenchBaseRecorder, StartRecord) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  ark.StartRecord();
  ASSERT_TRUE(ark.is_recording_);
}

TEST(TestBenchBaseRecorder, RecordDebugInfo) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  rapidjson::Value& debug_info_array =
      ark.GetRecordedFile(record_id)[kDebugInfo];
  EXPECT_EQ(debug_info_array.Size(), 0);
  ark.StartRecord();

  const char* test_url = "debug/test_url";
  const char* test_content =
      "Sample debug information with special characters: test content 🚀";

  ark.RecordDebugInfo(record_id, test_url, test_content);
  wait(ark.thread_);

  debug_info_array = ark.GetRecordedFile(record_id)[kDebugInfo];
  ASSERT_TRUE(debug_info_array.IsArray());
  EXPECT_EQ(debug_info_array.Size(), 1);

  rapidjson::Value& debug_entry = debug_info_array[0];
  ASSERT_TRUE(debug_entry.IsObject());
  EXPECT_STREQ(debug_entry[kParamDebugInfoUrl].GetString(), test_url);

  std::string encoded_content = debug_entry[kParamContent].GetString();
  EXPECT_FALSE(encoded_content.empty());
  EXPECT_EQ(
      encoded_content,
      "eJwFwcERgCAMBMBWrg7bsIIYg2QGAgPn+"
      "LUOP7ZoCe6uUnsx7LadBzxSG1XoLXA5M2Y3dSnQLEOUNuYC2iS0BS2I733uH2JDGms=");

  ark.Clear();
}

TEST(TestBenchBaseRecorder, RecordAction) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  rapidjson::Value& action_list = ark.GetRecordedFile(record_id)[kActionList];
  EXPECT_EQ(action_list.Size(), 0);
  ark.StartRecord();
  rapidjson::Value params(rapidjson::kObjectType);

  ark.RecordAction("TestFunction", params, record_id);
  ark.RecordAction("TestGlobalFunction", params, record_id);

  wait(ark.thread_);

  EXPECT_EQ(action_list.Size(), 2);
  rapidjson::Value& test_action = action_list[0];
  ASSERT_TRUE(test_action.IsObject());
  EXPECT_EQ(test_action.MemberCount(), 4);
  rapidjson::Value& function_name = test_action[kFunctionName];
  EXPECT_STREQ(function_name.GetString(), "TestFunction");

  ASSERT_TRUE(test_action.HasMember(kParamRecordTime));
  ASSERT_TRUE(test_action.HasMember(kParamRecordMillisecond));
  ASSERT_TRUE(test_action.HasMember(kParams));

  rapidjson::Value& recorded_params = test_action[kParams];
  ASSERT_TRUE(recorded_params.IsObject());

  test_action = action_list[1];
  ASSERT_TRUE(test_action.IsObject());
  EXPECT_EQ(test_action.MemberCount(), 4);
  function_name = test_action[kFunctionName];
  EXPECT_STREQ(function_name.GetString(), "TestGlobalFunction");

  ASSERT_TRUE(test_action.HasMember(kParamRecordTime));
  ASSERT_TRUE(test_action.HasMember(kParamRecordMillisecond));
  ASSERT_TRUE(test_action.HasMember(kParams));

  recorded_params = test_action[kParams];
  ASSERT_TRUE(recorded_params.IsObject());

  ark.Clear();
}

TEST(TestBenchBaseRecorder, RecordInvokedMethodData) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int record_id = 1;
  ark.StartRecord();
  ark.CreateRecordedFile(record_id);
  rapidjson::Value& invoked_method_data =
      ark.GetRecordedFileField(record_id, kInvokedMethodData);
  EXPECT_EQ(invoked_method_data.Size(), 0);
  rapidjson::Value params(rapidjson::kObjectType);
  ark.RecordInvokedMethodData("bridge", "call", params, record_id);
  wait(ark.thread_);
  EXPECT_EQ(invoked_method_data.Size(), 1);
  rapidjson::Value& invoked_module = invoked_method_data[0];
  ASSERT_TRUE(invoked_module.IsObject());
  EXPECT_EQ(invoked_module.MemberCount(), 5);
  ASSERT_TRUE(invoked_module.HasMember(kModuleName));
  EXPECT_STREQ(invoked_module[kModuleName].GetString(), "bridge");

  ASSERT_TRUE(invoked_module.HasMember(kMethodName));
  EXPECT_STREQ(invoked_module[kMethodName].GetString(), "call");

  ASSERT_TRUE(invoked_module.HasMember(kParamRecordTime));
  ASSERT_TRUE(invoked_module.HasMember(kParamRecordMillisecond));
  ASSERT_TRUE(invoked_module.HasMember(kParams));
  ASSERT_TRUE(invoked_module[kParams].IsObject());

  ark.Clear();
}

TEST(TestBenchBaseRecorder, RecordCallback) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  int64_t callback_id = 2;
  ark.StartRecord();
  ark.CreateRecordedFile(record_id);
  rapidjson::Value& callback = ark.GetRecordedFileField(record_id, kCallback);
  EXPECT_EQ(callback.MemberCount(), 0);
  rapidjson::Value params(rapidjson::kObjectType);
  ark.RecordCallback("bridge", "call", params, callback_id, record_id);
  wait(ark.thread_);
  EXPECT_EQ(callback.MemberCount(), 1);
  ASSERT_TRUE(callback.HasMember("2"));
  rapidjson::Value& callback_info = callback["2"];

  ASSERT_TRUE(callback_info.IsObject());
  EXPECT_EQ(callback_info.MemberCount(), 5);
  ASSERT_TRUE(callback_info.HasMember(kModuleName));
  EXPECT_STREQ(callback_info[kModuleName].GetString(), "bridge");

  ASSERT_TRUE(callback_info.HasMember(kMethodName));
  EXPECT_STREQ(callback_info[kMethodName].GetString(), "call");

  ASSERT_TRUE(callback_info.HasMember(kParamRecordTime));
  ASSERT_TRUE(callback_info.HasMember(kParamRecordMillisecond));
  ASSERT_TRUE(callback_info.HasMember(kParams));
  ASSERT_TRUE(callback_info[kParams].IsObject());
  ark.Clear();
}

TEST(TestBenchBaseRecorder, RecordComponent) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  int64_t type = 66;
  const char* component_name = "test-ui";
  ark.StartRecord();
  rapidjson::Value& component_list =
      ark.GetRecordedFileField(record_id, kComponentList);
  EXPECT_EQ(component_list.Size(), 0);

  ark.RecordComponent(component_name, type, record_id);
  wait(ark.thread_);
  EXPECT_EQ(component_list.Size(), 1);
  rapidjson::Value& component = component_list[0];
  ASSERT_TRUE(component.IsObject());
  EXPECT_EQ(component.MemberCount(), 2);
  ASSERT_TRUE(component.HasMember(kComponentName));
  EXPECT_STREQ(component[kComponentName].GetString(), component_name);
  ASSERT_TRUE(component.HasMember(kComponentType));
  EXPECT_EQ(component[kComponentType].GetInt(), type);
  ark.Clear();
}

TEST(TestBenchBaseRecorder, CompressToBase64String) {
  const std::string source =
      "recorder codec round-trip payload with utf8 chars "
      "\xe4\xb8\xad\xe6\x96\x87 "
      "and symbols";
  const std::string encoded = TestBenchBaseRecorder::CompressToBase64String(
      source.data(), source.size());
  EXPECT_FALSE(encoded.empty());
  EXPECT_EQ(ZlibDecompress(DecodeBase64(encoded)), source);

  // Empty input still yields a valid, decodable encoding.
  const std::string empty_encoded =
      TestBenchBaseRecorder::CompressToBase64String("", 0);
  EXPECT_FALSE(empty_encoded.empty());
  EXPECT_EQ(ZlibDecompress(DecodeBase64(empty_encoded)), "");

  // Multi-megabyte payload.
  std::string large(4u * 1024 * 1024, 'x');
  large.append("tail");
  EXPECT_EQ(
      ZlibDecompress(DecodeBase64(TestBenchBaseRecorder::CompressToBase64String(
          large.data(), large.size()))),
      large);
}

TEST(TestBenchBaseRecorder, WriteRecordJsonRoundTrip) {
  rapidjson::Document doc(rapidjson::kObjectType);
  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  rapidjson::Value action_list(rapidjson::kArrayType);
  doc.AddMember(rapidjson::StringRef(kActionList), action_list, allocator);

  const std::string path = (std::filesystem::temp_directory_path() /
                            "lynx_recorder_write_record_json_roundtrip.json")
                               .string();
  std::remove(path.c_str());
  ASSERT_TRUE(TestBenchBaseRecorder::WriteRecordJson(path, doc));

  const std::string encoded = ReadFileContent(path);
  ASSERT_FALSE(encoded.empty());
  const std::string plain = ZlibDecompress(DecodeBase64(encoded));
  ASSERT_FALSE(plain.empty());
  rapidjson::Document parsed;
  parsed.Parse(plain.c_str());
  ASSERT_FALSE(parsed.HasParseError());
  ASSERT_TRUE(parsed.IsObject());
  EXPECT_TRUE(parsed.HasMember(kActionList));
  std::remove(path.c_str());

  // Unwritable path fails without crashing.
  EXPECT_FALSE(TestBenchBaseRecorder::WriteRecordJson(
      "/nonexistent_dir_for_lynx_ut/sub/file.json", doc));
}

TEST(TestBenchBaseRecorder, RecordTime) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  const int64_t before = NowMillis();
  rapidjson::Document doc(rapidjson::kObjectType);
  ark.RecordTime(doc);
  const int64_t after = NowMillis();

  ASSERT_TRUE(doc.IsObject());
  EXPECT_EQ(doc.MemberCount(), 2);
  ASSERT_TRUE(doc.HasMember(kParamRecordTime));
  EXPECT_TRUE(doc[kParamRecordTime].IsString());
  const int64_t seconds = std::stoll(doc[kParamRecordTime].GetString());
  EXPECT_GE(seconds, before / 1000 - 1);
  EXPECT_LE(seconds, after / 1000 + 1);

  ASSERT_TRUE(doc.HasMember(kParamRecordMillisecond));
  ASSERT_TRUE(doc[kParamRecordMillisecond].IsInt64());
  const int64_t millis = doc[kParamRecordMillisecond].GetInt64();
  EXPECT_GE(millis, before);
  EXPECT_LE(millis, after);
  // Both fields come from the same clock sample.
  EXPECT_EQ(seconds, millis / 1000);
}

TEST(TestBenchBaseRecorder, EndRecordOutput) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  wait(ark.thread_);
  ark.Clear();
  wait(ark.thread_);
  const std::string temp_dir = std::filesystem::temp_directory_path().string();
  ark.SetRecorderPath(temp_dir);
  ark.StartRecord();

  const int64_t recorded_id = 4242;
  const int64_t bare_id = 4243;
  ark.AddLynxViewSessionID(recorded_id, 777);
  rapidjson::Value params(rapidjson::kObjectType);
  ark.RecordAction("TestFunction", params, recorded_id);
  ark.RecordAction("TestFunction", params, bare_id);
  wait(ark.thread_);

  std::vector<std::string> filenames;
  std::vector<int64_t> sessions;
  ark.EndRecord(
      [&](std::vector<std::string>& files, std::vector<int64_t>& ids) {
        filenames = files;
        sessions = ids;
      });
  wait(ark.thread_);

  ASSERT_EQ(filenames.size(), 2u);
  ASSERT_EQ(sessions.size(), 2u);
  for (size_t i = 0; i < filenames.size(); ++i) {
    const int64_t expected_id = sessions[i] == 777 ? recorded_id : bare_id;
    EXPECT_EQ(filenames[i],
              temp_dir + "/" + std::to_string(expected_id) + ".json");
    const std::string encoded = ReadFileContent(filenames[i]);
    EXPECT_FALSE(encoded.empty());
    const std::string plain = ZlibDecompress(DecodeBase64(encoded));
    EXPECT_FALSE(plain.empty());
    std::remove(filenames[i].c_str());
  }
  std::vector<int64_t> sorted_sessions = sessions;
  std::sort(sorted_sessions.begin(), sorted_sessions.end());
  EXPECT_EQ(sorted_sessions[0], -1);
  EXPECT_EQ(sorted_sessions[1], 777);

  // EndRecord stops recording and clears the tables.
  EXPECT_FALSE(ark.is_recording_);
  EXPECT_TRUE(ark.lynx_view_table_.empty());
  EXPECT_TRUE(ark.session_ids_.empty());
}

TEST(TestBenchBaseRecorder, EndRecordSkipsFailedWrites) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  wait(ark.thread_);
  ark.Clear();
  wait(ark.thread_);
  // Unwritable directory: WriteRecordJson fails, so the shell must not be
  // reported in the recordingComplete payload at all.
  ark.SetRecorderPath("/nonexistent_dir_for_lynx_ut");
  ark.StartRecord();

  rapidjson::Value params(rapidjson::kObjectType);
  ark.RecordAction("TestFunction", params, 1);
  wait(ark.thread_);

  std::vector<std::string> filenames;
  std::vector<int64_t> sessions;
  ark.EndRecord(
      [&](std::vector<std::string>& files, std::vector<int64_t>& ids) {
        filenames = files;
        sessions = ids;
      });
  wait(ark.thread_);

  EXPECT_TRUE(filenames.empty());
  EXPECT_TRUE(sessions.empty());
  EXPECT_FALSE(ark.is_recording_);
  EXPECT_TRUE(ark.lynx_view_table_.empty());

  // Restore a writable path for any later tests on this singleton.
  ark.SetRecorderPath(std::filesystem::temp_directory_path().string());
}

constexpr const char* kTestBase64Source =
    "dGVtcGxhdGUtYmluYXJ5";  // "template-binary"

bool ReadZipEntries(const std::string& path,
                    std::map<std::string, std::string>* entries) {
  std::ifstream ifs(path, std::ios::binary);
  const std::string data((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
  auto read16 = [&data](size_t at) -> uint32_t {
    return static_cast<uint32_t>(static_cast<uint8_t>(data[at])) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[at + 1])) << 8);
  };
  auto read32 = [&read16](size_t at) -> uint32_t {
    return read16(at) | (read16(at + 2) << 16);
  };
  size_t pos = 0;
  while (pos + 30 <= data.size() && read32(pos) == 0x04034b50) {
    const uint32_t method = read16(pos + 8);
    const uint32_t compressed_size = read32(pos + 18);
    const uint32_t uncompressed_size = read32(pos + 22);
    const uint32_t name_len = read16(pos + 26);
    const uint32_t extra_len = read16(pos + 28);
    const size_t content_at = pos + 30 + name_len + extra_len;
    if (content_at + compressed_size > data.size()) {
      return false;
    }
    const std::string name = data.substr(pos + 30, name_len);
    std::string content;
    if (method == 0) {  // store
      content = data.substr(content_at, compressed_size);
    } else if (method == 8) {  // raw deflate
      content.resize(uncompressed_size);
      z_stream strm = {};
      if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
        return false;
      }
      strm.next_in =
          reinterpret_cast<Bytef*>(const_cast<char*>(&data[content_at]));
      strm.avail_in = compressed_size;
      strm.next_out = reinterpret_cast<Bytef*>(&content[0]);
      strm.avail_out = uncompressed_size;
      const int ret = inflate(&strm, Z_FINISH);
      inflateEnd(&strm);
      if (ret != Z_STREAM_END) {
        return false;
      }
      content.resize(strm.total_out);
    } else {
      return false;
    }
    (*entries)[name] = std::move(content);
    pos = content_at + compressed_size;
  }
  return !entries->empty();
}

// Builds a JSON object string with the given member count, used to cross the
// pretty-printed line thresholds (>50 for global props, >30 for events) that
// trigger asset extraction.
std::string MakeObjectJson(int members) {
  std::string json = "{";
  for (int i = 0; i < members; ++i) {
    json +=
        (i ? ",\"k" : "\"k") + std::to_string(i) + "\":" + std::to_string(i);
  }
  return json + "}";
}

FixtureAction MakeAction(const char* fn, std::string params, int64_t ms) {
  return {fn, std::move(params), ms};
}

// Writes data to a fresh zip in the test temp dir and reads its entries back.
bool WriteZipForTest(const std::string& name, const FixtureData& data,
                     std::map<std::string, std::string>* entries) {
  const std::string path =
      (std::filesystem::temp_directory_path() / name).string();
  return WriteFixtureZip(path, data) && ReadZipEntries(path, entries);
}

TEST(FixtureWriter, DeterministicZipBytes) {
  auto make_data = [](bool reverse) {
    FixtureData data;
    data.actions.push_back(MakeAction("setThreadStrategy", "{\"id\":0}", 1000));
    const char* modules[] = {"alpha", "beta", "gamma", "delta"};
    for (int i = 0; i < 4; ++i) {
      const int idx = reverse ? 3 - i : i;
      FixtureCall call{
          "[\"" + std::string(modules[idx]) + "\"]", "{\"ok\":true}", {}, 1000};
      const std::string key = std::string(modules[idx]) + '\0' + "call";
      data.calls[key].push_back(call);
    }
    data.config_json = "{}";
    return data;
  };

  // Same recording content, different calls insertion order. unordered_map
  // iteration is insertion-dependent, so without sorting the zips differ.
  const std::string dir = std::filesystem::temp_directory_path().string();
  const std::string zip_a = dir + "/lynx_recorder_determinism_a.zip";
  const std::string zip_b = dir + "/lynx_recorder_determinism_b.zip";
  ASSERT_TRUE(WriteFixtureZip(zip_a, make_data(false)));
  ASSERT_TRUE(WriteFixtureZip(zip_b, make_data(true)));
  EXPECT_EQ(ReadFileContent(zip_a), ReadFileContent(zip_b));
}

TEST(FixtureWriter, FullFixtureGeneration) {
  const int64_t t0 = 1000;
  const std::string template_params =
      std::string("{\"url\":\"https://example.com/t.js\",\"source\":\"") +
      kTestBase64Source + "\"}";

  FixtureData data;
  data.actions = {
      MakeAction("loadTemplate", template_params, t0),
      // Immediate lifecycle actions (< 100ms from the first action).
      MakeAction("setThreadStrategy", "{\"id\":0}", t0 + 10),
      MakeAction("updateViewPort", "{\"width\":375}", t0 + 20),
      MakeAction("setGlobalProps", "{\"global_props\":{\"theme\":\"dark\"}}",
                 t0 + 30),
      // Same asset name: the last large global-props value wins.
      MakeAction("setGlobalProps",
                 "{\"global_props\":{\"marker\":\"first\",\"data\":" +
                     MakeObjectJson(60) + "}}",
                 t0 + 40),
      MakeAction("setGlobalProps",
                 "{\"global_props\":{\"marker\":\"last\",\"data\":" +
                     MakeObjectJson(60) + "}}",
                 t0 + 50),
      // Timed events cover lifecycle, decapitalization, asset, and fallback.
      MakeAction("sendGlobalEvent", "{\"name\":\"n\"}", t0 + 1000),
      MakeAction("SendCustomEvent", "{\"c\":1}", t0 + 1100),
      MakeAction("updateViewPort", "{\"width\":750}", t0 + 1200),
      MakeAction("setGlobalProps", "{\"global_props\":{\"x\":1}}", t0 + 1300),
      MakeAction("sendGlobalEvent", "{\"big\":" + MakeObjectJson(40) + "}",
                 t0 + 1400),
      MakeAction("myCustomAction", "{\"z\":1}", t0 + 1500)};
  data.has_load_template = true;
  data.load_template_url = "https://example.com/t.js";
  data.load_template_source_base64 = kTestBase64Source;
  data.load_template_data_json =
      "{\"value\":{\"web_id\":7},\"preprocessorName\":\"p\",\"readOnly\":true}";

  data.shared_data.emplace_back("skey", "{\"v\":1}");
  data.config_json = "{\"jsbIgnoredInfo\":[\"foo\",123,\"bar\"]}";

  // JSB calls: callback with delay, malformed args, malformed callback value.
  const std::string call_key = std::string("bridge") + '\0' + "callX";
  FixtureCall c1{"[1]", "{\"r\":1}", {{0, "{\"msg\":\"ok\"}", 250}}, t0 + 100};
  data.calls[call_key].push_back(c1);
  FixtureCall c2{"not json", "", {{1, "bad{", 0}}, t0 + 200};
  data.calls[call_key].push_back(c2);
  // A key without the module\0method separator is skipped.
  data.calls["orphan"].push_back(c1);

  std::map<std::string, std::string> entries;
  ASSERT_TRUE(WriteZipForTest("lynx_recorder_full.zip", data, &entries));
  ASSERT_TRUE(entries.count("fixture.js") == 1);
  const std::string& fx = entries["fixture.js"];

  const char* expected_fx[] = {
      // Immediate lifecycle actions; setGlobalProps unwraps its envelope, and
      // each large one reads its own uniquely-named extracted asset.
      "ctx.setThreadStrategy({\"id\":0});",
      "ctx.updateViewPort({\"width\":375});",
      "ctx.setGlobalProps({\"theme\":\"dark\"});",
      "ctx.setGlobalProps(ctx.readAsset(\"lifecycle/global_props_0.json\"));",
      "ctx.setGlobalProps(ctx.readAsset(\"lifecycle/global_props_1.json\"));",
      // loadTemplate: template.bin + flattened legacy-wrapped templateData.
      "ctx.loadTemplate(\"https://example.com/t.js\"",
      "ctx.readAsset(\"lifecycle/template_data.json\")",
      // Timed events with delays.
      "ctx.after(1000, () => ctx.sendGlobalEvent({\"name\":\"n\"}));",
      "ctx.after(1100, () => ctx.sendCustomEvent(",
      "ctx.after(1200, () => ctx.updateViewPort(",
      "ctx.after(1300, () => ctx.setGlobalProps({\"x\":1}));",
      "ctx.after(1400, () => ctx.sendGlobalEvent(ctx.readAsset(",
      "ctx.after(1500, () => ctx.dispatch(\"myCustomAction\", {\"z\":1}));",
      "ctx.sharedData(\"skey\", {\"v\":1});",
      // Config jsbIgnoredInfo merged into the matcher; non-strings skipped.
      ",\"foo\",\"bar\"",
      "ctx.register(\"bridge\", \"callX\"",
  };
  for (const char* snippet : expected_fx) {
    EXPECT_NE(fx.find(snippet), std::string::npos) << snippet;
  }
  // Ordering: immediate actions, then timed events, then mock handlers.
  EXPECT_LT(fx.find("ctx.setThreadStrategy"), fx.find("--- Timed events ---"));
  EXPECT_LT(fx.find("--- Timed events ---"), fx.find("ctx.register("));

  const char* expected_files[] = {
      "config.json",
      "assets/lifecycle/global_props_0.json",
      "assets/lifecycle/global_props_1.json",
      "assets/lifecycle/template_data.json",
      "assets/events/global_4.json",
      "assets/bridge/callX.json",
      "assets/template/template.bin",
  };
  for (const char* name : expected_files) {
    EXPECT_TRUE(entries.count(name) == 1) << name;
  }
  // Each large immediate setGlobalProps keeps its own data under a unique
  // asset name; a later value must not overwrite an earlier one.
  EXPECT_NE(entries["assets/lifecycle/global_props_0.json"].find("first"),
            std::string::npos);
  EXPECT_EQ(entries["assets/lifecycle/global_props_0.json"].find("last"),
            std::string::npos);
  EXPECT_NE(entries["assets/lifecycle/global_props_1.json"].find("last"),
            std::string::npos);
  EXPECT_EQ(entries["assets/lifecycle/global_props_1.json"].find("first"),
            std::string::npos);
  // Legacy-wrapped templateData is flattened into the asset.
  EXPECT_NE(
      entries["assets/lifecycle/template_data.json"].find("\"web_id\": 7"),
      std::string::npos);
  EXPECT_NE(
      entries["assets/lifecycle/template_data.json"].find("preprocessorName"),
      std::string::npos);
  // Mock call asset: callback delay kept, malformed fallbacks applied.
  const std::string& asset = entries["assets/bridge/callX.json"];
  EXPECT_NE(asset.find("\"delay\": 250"), std::string::npos);
  EXPECT_NE(asset.find("\"args\": []"), std::string::npos);
  EXPECT_NE(asset.find("\"value\": null"), std::string::npos);
  EXPECT_EQ(entries["assets/template/template.bin"], "template-binary");

  // loadTemplate with no recorded templateData still emits the third
  // argument, as an empty object, so replay never sees an undefined value.
  {
    FixtureData bare;
    bare.actions.push_back(MakeAction("loadTemplate", template_params, t0));
    bare.has_load_template = true;
    bare.load_template_url = "https://example.com/t.js";
    bare.load_template_source_base64 = kTestBase64Source;
    bare.config_json = "{}";
    std::map<std::string, std::string> bare_entries;
    ASSERT_TRUE(WriteZipForTest("lynx_recorder_bare_template.zip", bare,
                                &bare_entries));
    EXPECT_NE(bare_entries["fixture.js"].find(
                  "ctx.loadTemplate(\"https://example.com/t.js\", "
                  "\"template/template.bin\", {})"),
              std::string::npos);
  }
}

TEST(FixtureWriter, WriteFailures) {
  const std::string temp = std::filesystem::temp_directory_path().string();
  // No actions: nothing to replay, no zip produced.
  FixtureData empty;
  EXPECT_FALSE(WriteFixtureZip(temp + "/lynx_recorder_empty.zip", empty));
  // Unwritable output path -> false, no file left behind.
  FixtureData data;
  data.actions.push_back(MakeAction("setThreadStrategy", "{\"id\":0}", 1000));
  const std::string bad_path = "/no/such/dir/fixture.zip";
  EXPECT_FALSE(WriteFixtureZip(bad_path, data));
  EXPECT_FALSE(std::filesystem::exists(bad_path));

  // A template whose base64 fails to decode must fail the whole write rather
  // than shipping an empty template.bin that fixture.js still references.
  FixtureData bad_template;
  bad_template.actions.push_back(MakeAction(
      "loadTemplate", "{\"url\":\"https://example.com/t.js\"}", 1000));
  bad_template.has_load_template = true;
  bad_template.load_template_url = "https://example.com/t.js";
  bad_template.load_template_source_base64 = "@@@@";  // invalid base64
  bad_template.config_json = "{}";
  const std::string bad_template_path =
      temp + "/lynx_recorder_bad_template.zip";
  EXPECT_FALSE(WriteFixtureZip(bad_template_path, bad_template));
  EXPECT_FALSE(std::filesystem::exists(bad_template_path));
}

// Actions are keyed to the smallest record_ms, so an out-of-order stream never
// yields a negative ctx.after() delay or scrambles the immediate/timed split.
TEST(FixtureWriter, UnsortedActionsUseMinTimestamp) {
  FixtureData data;
  // First element is not the earliest: the true zero point is t=1000.
  data.actions = {
      MakeAction("sendGlobalEvent", "{\"name\":\"late\"}", 3000),
      MakeAction("setThreadStrategy", "{\"id\":0}", 1000),
      MakeAction("sendGlobalEvent", "{\"name\":\"early\"}", 1500),
  };
  data.config_json = "{}";

  std::map<std::string, std::string> entries;
  ASSERT_TRUE(WriteZipForTest("lynx_recorder_unsorted.zip", data, &entries));
  const std::string& fx = entries["fixture.js"];
  // The immediate action still lands up front (delay 0 < 100), not demoted to
  // a timed event by a wrong front()-based origin.
  EXPECT_NE(fx.find("ctx.setThreadStrategy({\"id\":0});"), std::string::npos);
  // Delays are relative to the min timestamp and never negative.
  EXPECT_NE(fx.find("ctx.after(500, () => ctx.sendGlobalEvent("),
            std::string::npos);
  EXPECT_NE(fx.find("ctx.after(2000, () => ctx.sendGlobalEvent("),
            std::string::npos);
  EXPECT_EQ(fx.find("ctx.after(-"), std::string::npos);
}

// Recorded (module, method) names flow straight from JS bridge calls, so a
// hostile name must not be able to inject a path separator or traverse out of
// assets/ (zip-slip). Path components are percent-encoded before becoming zip
// entry names, and fixture.js's readAsset references the same encoded path.
TEST(FixtureWriter, SanitizesZipEntryPaths) {
  FixtureData data;
  data.actions.push_back(MakeAction("setThreadStrategy", "{\"id\":0}", 1000));
  FixtureCall call{"[]", "{\"ok\":true}", {}, 1000};
  // module contains a traversal escape, method contains a separator and NUL is
  // exercised via the map key delimiter handling elsewhere.
  const std::string key = std::string("../../etc") + '\0' + "a/b";
  data.calls[key].push_back(call);
  data.config_json = "{}";

  std::map<std::string, std::string> entries;
  ASSERT_TRUE(WriteZipForTest("lynx_recorder_sanitize.zip", data, &entries));

  // The traversal chars are percent-encoded, so the entry is a single flat
  // component under assets/ rather than escaping it.
  const std::string expected = "assets/..%2F..%2Fetc/a%2Fb.json";
  EXPECT_TRUE(entries.count(expected)) << "missing sanitized entry";
  // No entry has a path component equal to ".." or "." (real zip-slip escape).
  for (const auto& pair : entries) {
    std::stringstream ss(pair.first);
    std::string component;
    while (std::getline(ss, component, '/')) {
      EXPECT_NE(component, "..") << "traversal component in " << pair.first;
      EXPECT_NE(component, ".") << "traversal component in " << pair.first;
    }
  }
  // The raw unsanitized path was never written.
  EXPECT_EQ(entries.count("assets/../../etc/a/b.json"), 0u);

  // fixture.js references the encoded asset path but registers the raw
  // (module, method) so replay dispatch still matches the recorded call.
  const std::string& fx = entries["fixture.js"];
  EXPECT_NE(fx.find("ctx.readAsset(\"..%2F..%2Fetc/a%2Fb.json\")"),
            std::string::npos);
  EXPECT_NE(fx.find("ctx.register(\"../../etc\", \"a/b\""), std::string::npos);
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
