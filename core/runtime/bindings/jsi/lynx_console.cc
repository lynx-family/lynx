// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <math.h>

#include <chrono>
#include <memory>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "base/include/to_underlying.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/console.h"
#include "core/runtime/common/lynx_console_helper.h"
#include "core/runtime/common/utils.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace piper {

Console::Console(Runtime* rt, std::shared_ptr<ConsoleMessagePostMan> post_man)
    : rt_(rt), post_man_(post_man) {}

Value Console::get(Runtime* rt, const PropNameID& name) {
  auto methodName = name.utf8(*rt);

  if (methodName == "log") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "log"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          return LogWithLevel(&rt, CONSOLE_LOG_INFO, args, count, "log");
        });
  }

  if (methodName == "report") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "report"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          return LogWithLevel(&rt, CONSOLE_LOG_REPORT, args, count, "log");
        });
  }

  if (methodName == "alog") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "alog"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          return LogWithLevel(&rt, CONSOLE_LOG_ALOG, args, count, "log");
        });
  }

  if (methodName == "assert") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "assert"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) {
          return Assert(&rt, CONSOLE_LOG_ERROR, args, count, "assert");
        });
  }

  if (methodName == "error") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "error"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          return LogWithLevel(&rt, CONSOLE_LOG_ERROR, args, count, "error");
        });
  }

  if (methodName == "warn") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "warn"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          return LogWithLevel(&rt, CONSOLE_LOG_WARNING, args, count, "warn");
        });
  }
  if (methodName == "info") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "info"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          return LogWithLevel(&rt, CONSOLE_LOG_INFO, args, count, "info");
        });
  }
  if (methodName == "debug") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "debug"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          return LogWithLevel(&rt, CONSOLE_LOG_INFO, args, count, "debug");
        });
  }
  if (methodName == "test") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "test"), 0,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          rapidjson::StringBuffer s;
          rapidjson::Writer<rapidjson::StringBuffer> writer(s);
          writer.StartObject();
          writer.Key("errMsg");
          writer.String("ok");
          writer.Key("path");
          writer.String("page/component/index");
          writer.EndObject();

          return Value(String::createFromUtf8(*rt_, s.GetString()));
          //  return LogWithLevel(&rt, logging::LOG_WARNING, args, count);
        });
  }
  if (methodName == "profile") {
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
  }
  if (methodName == "profileEnd") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "profileEnd"), 0,
        [](Runtime& rt, const Value& thisVal, const Value* args, size_t count) {
          TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JAVASCRIPT);
          return piper::Value::undefined();
        });
  }
  return piper::Value::undefined();
}

void Console::set(Runtime* rt, const PropNameID& name, const Value& value) {}

std::vector<PropNameID> Console::getPropertyNames(Runtime& rt) {
  std::vector<PropNameID> vec;
  vec.push_back(piper::PropNameID::forUtf8(rt, "log"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "error"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "warn"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "info"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "debug"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "report"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "alog"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "assert"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "profile"));
  vec.push_back(piper::PropNameID::forUtf8(rt, "profileEnd"));
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

piper::Value Console::LogWithLevel(Runtime* rt, const int level,
                                   const Value* args, size_t count,
                                   const std::string& func_name) {
  Scope scope(*rt);

  bool is_devtool_enabled = tasm::LynxEnv::GetInstance().IsDevToolEnabled();
  if (count > 0) {
    if (is_devtool_enabled) {
      piper::Object global = rt->global();
      JSRuntimeType rt_type = rt->type();
      auto console = global.getProperty(
          *rt, rt_type == JSRuntimeType::quickjs ? "lynxConsole" : "console");
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
