// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/replay_controller.h"

#include <algorithm>
#include <memory>
#include <vector>

#if ENABLE_TESTBENCH_REPLAY
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/services/replay/layout_tree_testbench.h"
#include "core/services/replay/testbench_test_replay.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"
#endif  // ENABLE_TESTBENCH_REPLAY

namespace lynx {
namespace tasm {
namespace replay {

bool ReplayController::Enable() {
#if ENABLE_TESTBENCH_REPLAY
  return true;
#else
  return false;
#endif
}

void ReplayController::StartTest() {
#if ENABLE_TESTBENCH_REPLAY
  lynx::tasm::replay::TestBenchTestReplay::GetInstance().StartTest();
#endif
}

void ReplayController::EndTest(const std::string& file_path) {
#if ENABLE_TESTBENCH_REPLAY
  lynx::tasm::replay::TestBenchTestReplay::GetInstance().EndTest(file_path);
#endif
}

void ReplayController::SetDevToolObserver(
    const std::shared_ptr<lynx::tasm::InspectorCommonObserver>& observer) {
#if ENABLE_TESTBENCH_REPLAY
  lynx::tasm::replay::TestBenchTestReplay::GetInstance().SetDevToolObserver(
      observer);
#endif
}

void ReplayController::SendFileByAgent(const std::string& type,
                                       const std::string& file) {
#if ENABLE_TESTBENCH_REPLAY
  LOGI("SendFileByAgent: type: " + type + ", file: " + file);
  if (!file.empty()) {
    lynx::tasm::replay::TestBenchTestReplay::GetInstance().SendFileByAgent(
        type, file);
  }
#endif
}

std::string ReplayController::GetLayoutTree(SLNode* slnode) {
#if ENABLE_TESTBENCH_REPLAY
  if (slnode) {
    return lynx::tasm::replay::LayoutTreeTestBench::GetLayoutTree(slnode);
  }
  return "";
#endif
  return "";
}

std::string ReplayController::ConvertEventInfo(const lepus::Value& info) {
#if ENABLE_TESTBENCH_REPLAY
  BASE_STATIC_STRING_DECL(kType, "type");
  if (info.IsObject() && info.GetProperty(kType).IsString()) {
    const auto& type = info.GetProperty(kType).StdString();
    constexpr const static char* kLoad = "load";
    constexpr const static char* kError = "error";
    constexpr const static char* kScroll = "scroll";
    constexpr const static char* kNodeAppear = "nodeappear";
    constexpr const static char* kImpression = "impression";
    constexpr const static char* kContentSizeChanged = "contentsizechanged";
    if (type == kLoad || type == kError || type == kScroll ||
        type == kNodeAppear || type == kImpression ||
        type == kContentSizeChanged) {
      return "";
    }
  }

  const static auto& get_json_string_f = [](const rapidjson::Document& d) {
    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    d.Accept(writer);
    return std::string(buffer.GetString());
  };

  const static std::function<rapidjson::Value(
      rapidjson::MemoryPoolAllocator<> & allocator, const lepus::Value&)>&
      to_json_f = [](rapidjson::MemoryPoolAllocator<>& allocator,
                     const lepus::Value& value) {
        rapidjson::Value v;
        auto type = value.Type();
        switch (type) {
          case lepus::ValueType::Value_Int64:
            v.Set(value.Int64());
            break;
          case lepus::ValueType::Value_UInt64:
            v.Set(value.UInt64());
            break;
          case lepus::ValueType ::Value_Int32:
            v.Set(value.Int32());
            break;
          case lepus::ValueType ::Value_UInt32:
            v.Set(value.UInt32());
            break;
          case lepus::ValueType::Value_Double:
            v.Set(value.Double());
            break;
          case lepus::ValueType::Value_Bool:
            v.Set(value.Bool());
            break;
          case lepus::ValueType::Value_String: {
            rapidjson::StringBuffer buf;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
            writer.String(value.CString());
            v.SetString(buf.GetString(), allocator);
          } break;
          case lepus::ValueType::Value_Table: {
            auto table = value.Table();
            std::vector<base::String> temp_v;
            for (auto& it : *table) {
              constexpr const static char* kTimeStamp = "timestamp";
              constexpr const static char* kUid = "uid";
              constexpr const static char* kIdentifier = "identifier";
              if (it.first.str().compare(kTimeStamp) != 0 &&
                  it.first.str().compare(kUid) != 0 &&
                  it.first.str().compare(kIdentifier) != 0) {
                temp_v.push_back(it.first);
              }
            }
            std::sort(temp_v.begin(), temp_v.end(),
                      [](const base::String& left, const base::String& right) {
                        return left.str() < right.str();
                      });
            v.SetObject();
            for (auto& it : temp_v) {
              v.AddMember(rapidjson::Value(it.c_str(), allocator),
                          to_json_f(allocator, table->GetValue(it)), allocator);
            }
            break;
          }
          case lepus::ValueType::Value_Array: {
            auto array = value.Array();
            v.SetArray();
            for (size_t i = 0; i < array->size(); ++i) {
              v.PushBack(to_json_f(allocator, array->get(i)), allocator);
            }
            break;
          }
          case lepus::ValueType::Value_NaN: {
            v.Set(NAN);
          } break;
          default:
            v.SetNull();
            break;
        }
        return v;
      };

  rapidjson::Document d;
  d.CopyFrom(to_json_f(d.GetAllocator(), info), d.GetAllocator());
  return get_json_string_f(d);
#else
  return "";
#endif
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
