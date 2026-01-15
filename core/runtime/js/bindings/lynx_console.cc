// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <math.h>

#include <chrono>
#include <cmath>
#include <memory>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "base/include/to_underlying.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/lynx_console_helper.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/js/bindings/console.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace piper {

Console::Console(std::shared_ptr<ConsoleMessagePostMan> post_man,
                 bool debuggable)
    : post_man_(post_man), debuggable_(debuggable) {
  Init();
}

void Console::Init() {
  auto make_log_function = [this](int level, const char* name_str) {
    return [this, level, name_str](Runtime* rt) {
      return Function::createFromHostFunction(
          *rt, PropNameID::forAscii(*rt, name_str), 0,
          [this, level, name_str](
              Runtime& rt, const Value&, const Value* args,
              size_t count) -> base::expected<Value, JSINativeException> {
            return this->LogWithLevel(&rt, level, args, count, name_str);
          });
    };
  };

  methods_map_["log"] = make_log_function(CONSOLE_LOG_INFO, "log");
  methods_map_["report"] = make_log_function(CONSOLE_LOG_REPORT, "log");

  methods_map_["alog"] = make_log_function(CONSOLE_LOG_ALOG, "log");
  methods_map_["error"] = make_log_function(CONSOLE_LOG_ERROR, "error");
  methods_map_["warn"] = make_log_function(CONSOLE_LOG_WARNING, "warn");
  methods_map_["info"] = make_log_function(CONSOLE_LOG_INFO, "info");
  methods_map_["debug"] = make_log_function(CONSOLE_LOG_INFO, "debug");

  methods_map_["assert"] = [this](Runtime* rt) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "assert"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) {
          return Assert(&rt, CONSOLE_LOG_ERROR, args, count, "assert");
        });
  };

  methods_map_["test"] = [](Runtime* rt) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "test"), 0,
        [](Runtime& rt, const Value& thisVal, const Value* args,
           size_t count) -> base::expected<Value, JSINativeException> {
          rapidjson::StringBuffer s;
          rapidjson::Writer<rapidjson::StringBuffer> writer(s);
          writer.StartObject();
          writer.Key("errMsg");
          writer.String("ok");
          writer.Key("path");
          writer.String("page/component/index");
          writer.EndObject();

          return Value(String::createFromUtf8(rt, s.GetString()));
          //  return LogWithLevel(&rt, logging::LOG_WARNING, args, count);
        });
  };

  methods_map_["profile"] = [](Runtime* rt) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "profile"), 0,
        [](Runtime& rt, const Value& thisVal, const Value* args, size_t count) {
          std::string trace_name = "JavaScript::";
          if (count > 0 && (*args).isString()) {
            trace_name += (*args).getString(rt).utf8(rt);
          }
          TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JAVASCRIPT, nullptr,
                            [&](lynx::perfetto::EventContext ctx) {
                              ctx.event()->set_name(trace_name);
                            });
          return piper::Value::undefined();
        });
  };
  methods_map_["profileEnd"] = [](Runtime* rt) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "profileEnd"), 0,
        [](Runtime& rt, const Value& thisVal, const Value* args, size_t count) {
          TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JAVASCRIPT);
          return piper::Value::undefined();
        });
  };

  auto make_call_js_function = [this](const char* prop_name) {
    return [this, prop_name](Runtime* rt) {
      return Function::createFromHostFunction(
          *rt, PropNameID::forAscii(*rt, prop_name), 0,
          [this, prop_name](
              Runtime& rt, const Value&, const Value* args,
              size_t count) -> base::expected<Value, JSINativeException> {
            return this->CallJSEngineConsole(&rt, args, count, prop_name);
          });
    };
  };
  methods_map_["count"] = make_call_js_function("count");
  methods_map_["countReset"] = make_call_js_function("countReset");
  methods_map_["group"] = make_call_js_function("group");
  methods_map_["groupCollapsed"] = make_call_js_function("groupCollapsed");
  methods_map_["groupEnd"] = make_call_js_function("groupEnd");
  methods_map_["time"] = make_call_js_function("time");
  methods_map_["timeLog"] = make_call_js_function("timeLog");
  methods_map_["timeEnd"] = make_call_js_function("timeEnd");
  methods_map_["table"] = make_call_js_function("table");
}

Value Console::get(Runtime* rt, const PropNameID& name) {
  auto method_name = name.utf8(*rt);
  auto it = methods_map_.find(method_name);
  if (it != methods_map_.end()) {
    return it->second(rt);
  }
  return piper::Value::undefined();
}

void Console::set(Runtime* rt, const PropNameID& name, const Value& value) {}

std::vector<PropNameID> Console::getPropertyNames(Runtime& rt) {
  std::vector<PropNameID> vec;
  vec.reserve(methods_map_.size());
  for (auto& pair : methods_map_) {
    vec.push_back(piper::PropNameID::forUtf8(rt, pair.first));
  }
  return vec;
}

piper::Value Console::Assert(Runtime* rt, const int level, const Value* args,
                             size_t count, const std::string& func_name) {
  Scope scope(*rt);

  piper::Object global = rt->global();
  auto console_opt = global.getProperty(*rt, "console");
  if (!console_opt) {
    return piper::Value::undefined();
  }
  piper::Value console = std::move(*console_opt);
  if (console.isObject() && !console.getObject(*rt).isHostObject(*rt) &&
      console.getObject(*rt).hasProperty(*rt, func_name.c_str())) {
    auto func_value =
        console.getObject(*rt).getProperty(*rt, func_name.c_str());
    if (func_value && func_value->isObject() &&
        func_value->getObject(*rt).isFunction(*rt)) {
      piper::Function func = func_value->getObject(*rt).getFunction(*rt);
      func.callWithThis(*rt, console.getObject(*rt), args, count);
    }
  }

  base::logging::LogChannel channel_type = GetChannelType(rt, args);

  if (count < 2) {
    std::string msg = "Assertion error: Arguments number error";
    JSLOG(ERROR, rt->getRuntimeId(), channel_type) << msg;
    return piper::Value::undefined();
  }
  // In different JS runtime bool args might be parsed to BooleanKing or
  // StringKing Value
  if ((args[0].isString() && args[0].getString(*rt).utf8(*rt) != "false") ||
      (args[0].isBool() && args[0].getBool())) {
    return piper::Value::undefined();
  }
  std::string msg = "Assertion failed: ";
  for (size_t i = 1; i < count; ++i) {
    msg += LogObject_(rt, &args[i]);
  }
  JSLOG(ERROR, rt->getRuntimeId(), channel_type) << msg;
  auto post_man = post_man_.lock();
  if (post_man != nullptr) {
    int64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
    post_man->OnMessagePosted({msg, level, ts});
  }
  return piper::Value::undefined();
}

piper::Value Console::CallJSEngineConsole(Runtime* rt, const Value* args,
                                          size_t count,
                                          const std::string& func_name) {
  Scope scope(*rt);
  bool is_devtool_enabled =
      tasm::LynxEnv::GetInstance().IsDevToolEnabled() || debuggable_;
  if (is_devtool_enabled) {
    piper::Object global = rt->global();
    auto console = global.getProperty(*rt, "console");
    if (console && console->isObject() &&
        !console->getObject(*rt).isHostObject(*rt) &&
        console->getObject(*rt).hasProperty(*rt, func_name.c_str())) {
      auto func_value =
          console->getObject(*rt).getProperty(*rt, func_name.c_str());
      if (func_value && func_value->isObject() &&
          func_value->getObject(*rt).isFunction(*rt)) {
        piper::Function func = func_value->getObject(*rt).getFunction(*rt);
        func.callWithThis(*rt, console->getObject(*rt), args, count);
      }
    }
  }
  return piper::Value::undefined();
}

piper::Value Console::LogWithLevel(Runtime* rt, const int level,
                                   const Value* args, size_t count,
                                   const std::string& func_name) {
  Scope scope(*rt);
  CallJSEngineConsole(rt, args, count, func_name);
  if (count > 0) {
    std::string msg;
    for (size_t i = 0; i < count; ++i) {
      msg += LogObject_(rt, &args[i]);
      if (i != count - 1) {
        msg += "   ||   ";
      }
    }

    base::logging::LogChannel channel_type = GetChannelType(rt, args);

    switch (level) {
      case CONSOLE_LOG_VERBOSE:
        JSLOG(VERBOSE, rt->getRuntimeId(), channel_type) << msg;
        break;
      case CONSOLE_LOG_INFO:
      case CONSOLE_LOG_LOG:
        JSLOG(INFO, rt->getRuntimeId(), channel_type) << msg;
        break;
      case CONSOLE_LOG_WARNING:
        JSLOG(WARNING, rt->getRuntimeId(), channel_type) << msg;
        break;
      case CONSOLE_LOG_ERROR:
        JSLOG(ERROR, rt->getRuntimeId(), channel_type) << msg;
        break;
      case CONSOLE_LOG_REPORT:
        JSALOG(ERROR, rt->getRuntimeId(), channel_type) << msg;
        break;
      case CONSOLE_LOG_ALOG:
        JSALOG(INFO, rt->getRuntimeId(), channel_type) << msg;
        break;
      default:
        break;
    }

    bool is_devtool_enabled =
        tasm::LynxEnv::GetInstance().IsDevToolEnabled() || debuggable_;
    if (is_devtool_enabled) {
      auto post_man = post_man_.lock();
      if (post_man != nullptr) {
        long long ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
        post_man->OnMessagePosted({msg, level, ts});
      }
    }
  }
  return piper::Value::undefined();
}

std::string Console::LogObject(Runtime* rt, const Value* value) {
  Scope scope(*rt);

  std::string msg;
  msg = LogObject_(rt, value);
  LOGE(msg);
  return msg;
}

std::string Console::LogObject(Runtime* rt, const piper::Object* obj) {
  Scope scope(*rt);

  piper::Value vv(*rt, *obj);
  std::string msg = LogObject_(rt, &vv);
  LOGE(msg);
  return msg;
}

std::string Console::LogObject_(Runtime* rt, const Value* value) {
  piper::Object global = rt->global();
  auto log_depth = global.getProperty(*rt, "__LOG_DEPTH__");
  JSValueCircularArray pre_object_vector;
  std::string msg = LogObject_(rt, value, pre_object_vector,
                               log_depth && log_depth->isNumber()
                                   ? static_cast<int>(log_depth->getNumber())
                                   : 1);
  return msg;
}

std::string Console::LogObject_(Runtime* rt, const Value* value,
                                JSValueCircularArray& pre_object_vector,
                                int depth) {
  Scope scope(*rt);
  std::string msg;
  depth--;
  if (value->isString()) {
    msg += "\"";
    msg += value->getString(*rt).utf8(*rt);
    msg += "\"";
  } else if (value->isObject()) {
    if (IsCircularJSObject(*rt, value->getObject(*rt), pre_object_vector)) {
      msg += "\"[Circular ~]\"";
      return msg;
    }
    // As Object is Movable, not copyable, do not push the Object you will use
    // later to vector! You need clone a new one.
    ScopedJSObjectPushPopHelper scoped_push_pop_helper(pre_object_vector,
                                                       value->getObject(*rt));

    if (value->getObject(*rt).isFunction(*rt)) {
      msg += "f";
    } else if (value->getObject(*rt).isArray(*rt)) {
      msg += "[";

      piper::Array ary = value->getObject(*rt).getArray(*rt);
      auto length = ary.length(*rt);
      if (length) {
        for (size_t i = 0; i < *length; i++) {
          auto property = ary.getValueAtIndex(*rt, i);
          if (!property) {
            return msg;
          }
          msg += LogObject_(rt, &(*property), pre_object_vector, depth);
          if (i != *length - 1) {
            msg += ",";
          }
        }
      }
      msg += "]";
    } else {
      piper::Object cur = value->getObject(*rt);
      msg += "{";
      auto ary = value->getObject(*rt).getPropertyNames(*rt);
      if (!ary) {
        return msg;
      }
      auto length = (*ary).length(*rt);
      if (length) {
        for (size_t i = 0; i < *length; i++) {
          auto propertyName = (*ary).getValueAtIndex(*rt, i);
          if (!propertyName) {
            return msg;
          }
          if (propertyName->isString()) {
            std::string pro_name = propertyName->getString(*rt).utf8(*rt);
            msg += pro_name;
            auto pro = value->getObject(*rt).getProperty(*rt, pro_name.c_str());
            msg += ": ";
            if (pro && pro->isObject() && !pro->getObject(*rt).isArray(*rt) &&
                !pro->getObject(*rt).isFunction(*rt)) {
              if (depth <= 0) {
                msg += "{...}";
              } else {
                msg += LogObject_(rt, &(*pro), pre_object_vector, depth);
              }
            } else {
              msg += LogObject_(rt, &(*pro), pre_object_vector, depth);
            }
            if (i != *length - 1) {
              msg += ",";
            }
          }
        }
      }
      msg += "}";
    }
  } else if (value->isNumber()) {
    double number = value->getNumber();
    char buffer[1024];
    if (abs(round(number) - number) < 0.000000000000001) {
      snprintf(buffer, sizeof(buffer), "%" PRId64,
               static_cast<int64_t>(number));
    } else {
      snprintf(buffer, sizeof(buffer), "%f", number);
    }
    msg += buffer;
  } else if (value->isBool()) {
    bool number = value->getBool();
    std::string bool_value = number == 0 ? "false" : "true";
    msg += bool_value;
  } else if (value->isNull()) {
    msg += "null";
  } else if (value->isUndefined()) {
    msg += "undefined";
  } else if (value->isSymbol()) {
    // toString output will look like Symbol(description)
    msg += value->getSymbol(*rt).toString(*rt).value_or("Symbol()");
  } else {
    char val[100];
    int type = base::to_underlying(value->kind());
    snprintf(val, sizeof(val), "%d", type);
    msg += "Type:";
    msg += val;
  }

  return msg;
}  // namespace piper

// TODO(wangqingyu.c0l1n)
// Due to historical reasons, the current identification method for external
// channels is to judge whether the JS log contains runtimeID when devtool is
// turned on, and it will be changed to another one in the future, no longer
// depending on devtool
base::logging::LogChannel Console::GetChannelType(Runtime* rt,
                                                  const Value* args) {
  if (args[0].isString()) {
    auto arg0 = args[0].asString(*rt);
    if (arg0) {
      return base::BeginsWith(arg0->utf8(*rt), "runtimeId")
                 ? base::logging::LOG_CHANNEL_LYNX_EXTERNAL
                 : base::logging::LOG_CHANNEL_LYNX_INTERNAL;
    }
  }

  return base::logging::LOG_CHANNEL_LYNX_INTERNAL;
}

}  // namespace piper
}  // namespace lynx
