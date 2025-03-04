// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/runtime/bindings/jsi/modules/ios/lynx_module_darwin.h"
#import "core/runtime/bindings/jsi/modules/ios/lynx_module_interceptor.h"

#import <objc/message.h>
#import <objc/runtime.h>
#include <memory>
#include <optional>

#import "LynxLog.h"
#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/darwin/lynx_env_darwin.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/interceptor/ios/request_interceptor_darwin.h"
#include "core/runtime/common/utils.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/recorder/recorder_controller.h"

namespace lynx {
namespace piper {

namespace {
std::string genExceptionErrorMessage(NSException *exception) {
  auto message = std::string{" throws an uncaught exception: "}
                     .append([exception.name UTF8String])
                     .append(". Reason: ")
                     .append([exception.reason UTF8String]);

  if (exception.userInfo != nil) {
    message.append(". UserInfo:\n");
    for (id key in exception.userInfo) {
      NSString *pair =
          [[NSString alloc] initWithFormat:@"%@, %@", key, [exception.userInfo objectForKey:key]];
      message.append([pair UTF8String]);
      message.append("\n");
    }
  }
  return message;
}

std::string typeToString(pub::Value *value) {
  if (!value) {
    return "nullptr";
  }
  value->backend_type();
  if (value->IsUndefined()) {
    return "Undefined";
  } else if (value->IsNil()) {
    return "Null";
  } else if (value->IsNumber()) {
    return "Number";
  } else if (value->IsString()) {
    return "String";
  } else if (value->IsMap()) {
    return "Object";
  } else {
    return "Unknown";
  }
}
}  // namespace

#pragma mark - LynxModuleDarwin

void LynxModuleDarwin::buildLookupMap(NSDictionary<NSString *, NSString *> *lookup) {
  for (NSString *methodName in lookup) {
    SEL selector = NSSelectorFromString(lookup[methodName]);
    NSMethodSignature *msig = [[instance_ class] instanceMethodSignatureForSelector:selector];
    if (msig == nil) {
      // TODO (liujilong): Log this.
      continue;
    }
    std::string method = std::string([methodName UTF8String]);
    methods_.emplace(method, NativeModuleMethod(method, msig.numberOfArguments - 2));
  }
}

LynxModuleDarwin::LynxModuleDarwin(id<LynxModule> instance)
    : LynxNativeModule(std::make_shared<pub::PubValueFactoryDarwin>()),
      instance_(instance),
      module_name_(std::string([[[instance class] name] UTF8String])) {
  methodLookup = [[instance class] methodLookup];
  buildLookupMap(methodLookup);
  if ([[instance class] respondsToSelector:@selector(attributeLookup)]) {
    attributeLookup = [[instance class] attributeLookup];
  }
  methodAuthBlocks_ = [[NSMutableArray alloc] init];
  methodSessionBlocks_ = [NSMutableArray array];
}

void LynxModuleDarwin::Destroy() {
#if !defined(OS_OSX)
  if (instance_ == nil || [instance_ isEqual:[NSNull null]]) {
    LOGI("lynx LynxModule Destroy: " << module_name_ << ", module is empty.");
    return;
  }
  LOGI("lynx LynxModule Destroy: " << module_name_);
  if ([instance_ respondsToSelector:@selector(destroy)]) {
    [instance_ destroy];
  }
#endif  // !defined(OS_OSX)
}

#if OS_IOS || OS_TVOS || OS_OSX
void LynxModuleDarwin::EnterInvokeScope(Runtime *rt,
                                        std::shared_ptr<ModuleDelegate> module_delegate) {
  scope_rts_.push_back(rt);
  scope_module_delegates_.push_back(std::move(module_delegate));
}
void LynxModuleDarwin::ExitInvokeScope() {
  if (!scope_rts_.empty()) {
    scope_rts_.pop_back();
  }
  if (!scope_module_delegates_.empty()) {
    scope_module_delegates_.pop_back();
  }
}

std::optional<piper::Value> LynxModuleDarwin::TryGetPromiseRet() {
  if (!scope_native_promise_rets_.empty()) {
    auto ret = std::move(scope_native_promise_rets_.back());
    scope_native_promise_rets_.pop_back();
    return ret;
  }
  return std::nullopt;
}

#endif

base::expected<std::unique_ptr<pub::Value>, std::string> LynxModuleDarwin::InvokeMethod(
    const std::string &method_name, std::unique_ptr<pub::Value> args, size_t count,
    const CallbackMap &callbacks) {
  base::expected<std::unique_ptr<pub::Value>, std::string> res;
  std::string first_arg_str;
  if (count > 0 && args && args->IsArray() && args->GetValueAtIndex(0)->IsString()) {
    first_arg_str = args->GetValueAtIndex(0)->str();
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "CallJSB", [&](lynx::perfetto::EventContext ctx) {
    ctx.event()->add_debug_annotations("module_name", module_name_);
    ctx.event()->add_debug_annotations("method_name", method_name);
    ctx.event()->add_debug_annotations("first_arg", first_arg_str);
  });
  // issue: #1510
  int32_t callErrorCode = error::E_SUCCESS;
  uint64_t start_time = lynx::base::CurrentTimeMilliseconds();
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_func_call_start",
                      [&first_arg_str, start_time](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_debug_annotations("first_arg", first_arg_str);
                        ctx.event()->add_debug_annotations("timestamp", std::to_string(start_time));
                      });
  std::ostringstream invoke_session;
  invoke_session << start_time;
  @try {
    NSString *jsMethodNameNSString = [NSString stringWithUTF8String:method_name.c_str()];
    if (methodLookup[jsMethodNameNSString] != nil) {
      LOGI("LynxModuleDarwin will invokeMethod: " << module_name_ << "." << method_name
                                                  << " , args: " << first_arg_str << " " << this);
      SEL selector = NSSelectorFromString(methodLookup[jsMethodNameNSString]);
      auto invoke_res = invokeObjCMethod(method_name, start_time, selector, args.get(), count,
                                         callErrorCode, callbacks);
      if (!invoke_res.has_value()) {
        return base::unexpected(
            "Exception happen in LynxModuleDarwin invokeMethod: " + module_name_ + "." +
            method_name + " , args: " + first_arg_str + " errorInfo:" + invoke_res.error());
      }
      res = std::move(invoke_res);
      LOGI("LynxModuleDarwin did invokeMethod: " << module_name_ << "." << method_name
                                                 << " , args: " << first_arg_str << " " << this);
    } else {
      LOGE("LynxModuleDarwin::invokeMethod module: "
           << module_name_ << ", method: " << method_name << " , args: " << first_arg_str
           << " failed in invokeMethod(), cannot find method in methodLookup: " <<
           [[methodLookup description] UTF8String]);
    }
  } @catch (NSException *exception) {
    _LogE(@"Exception '%@' was thrown while invoking function %s on target %@\n call stack: %@",
          exception, method_name.c_str(), instance_, exception.callStackSymbols);
    // NSInvalidArgumentException is already handle in iOS.3
    callErrorCode = error::E_NATIVE_MODULES_EXCEPTION;
    auto lock_delegate = delegate_.lock();
    if (lock_delegate) {
      lock_delegate->OnErrorOccurred(
          module_name_, method_name,
          base::LynxError{
              callErrorCode,
              LynxModuleUtils::GenerateErrorMessage(module_name_, method_name,
                                                    genExceptionErrorMessage(exception)),
              "This error is caught by native, please ask RD of Lynx or client for help.",
              base::LynxErrorLevel::Error});
    }
  }
  if (tasm::LynxEnv::GetInstance().IsPiperMonitorEnabled()) {
    tasm::LynxEnvDarwin::onPiperInvoked(module_name_, method_name, first_arg_str, schema_,
                                        invoke_session.str());
  }
  return res;
}

NSInvocation *LynxModuleDarwin::getMethodInvocation(
    const id module, const std::string &methodName, SEL selector, const pub::Value *args,
    size_t count, NSMutableArray *retainedObjectsForInvocation, int32_t &callErrorCode,
    uint64_t start_time, NSDictionary *extra, const CallbackMap &callbacks) {
  NSMethodSignature *methodSignature = [[module class] instanceMethodSignatureForSelector:selector];
  NSInvocation *inv = [NSInvocation invocationWithMethodSignature:methodSignature];
  [inv setSelector:selector];

  NSMutableArray *objcParams = [[NSMutableArray alloc] init];
  static char STRING_KEY;
  objc_setAssociatedObject(inv, &STRING_KEY, objcParams, OBJC_ASSOCIATION_RETAIN);

  // error message generator
  auto expectedButGot = [this, &methodName, &callErrorCode](size_t pos, const std::string &expected,
                                                            const std::string &but_got) mutable {
    callErrorCode = error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_TYPE;
    auto message =
        LynxModuleUtils::ExpectedButGotAtIndexError(expected, but_got, static_cast<int>(pos));
    auto lock_delegate = delegate_.lock();
    if (lock_delegate) {
      lock_delegate->OnErrorOccurred(
          module_name_, methodName,
          base::LynxError{callErrorCode,
                          LynxModuleUtils::GenerateErrorMessage(module_name_, methodName, message),
                          "Please check the arguments.", base::LynxErrorLevel::Error});
    }
  };
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "JSValueToObjCValue");
  // index: i + 2 ==> objc arguments: [this, _cmd, args..., resoledBlock, rejectedBlock]
  for (size_t i = 0; i < count; i++) {
    auto arg = args->GetValueAtIndex(static_cast<int>(i));
    const char *objCArgType = [methodSignature getArgumentTypeAtIndex:i + 2];
    // issue: #1510
    auto reportError = [i, expectedButGot, &arg](const std::string &expected) mutable {
      expectedButGot(i, expected, typeToString(arg.get()));
    };
    NSNumber *num = nil;
    if (arg->IsBool()) {
      num = [NSNumber numberWithBool:arg->Bool()];
    } else if (arg->IsNumber()) {
      num = [NSNumber numberWithDouble:arg->Number()];
    }

    if (objCArgType[0] == _C_ID) {
      id obj;
      if (arg->IsInt64() && callbacks.find(i) != callbacks.end()) {
        LOGV("LynxModuleDarwin::getMethodInvocation, "
             << " module: " << module_name_ << " method: " << methodName
             << " |JS FUNCTION| found at argument " << i);
        obj =
            ConvertModuleCallbackToCallbackBlock(callbacks.at(i), methodName, &args[0], start_time);
      } else if (arg->IsNil()) {
        obj = nil;
      } else {
        obj = pub::ValueUtilsDarwin::ConvertPubValueToOCValue(*arg.get());
      }
      if (obj != nil) {
        if ([obj isKindOfClass:[NSDictionary class]]) {
          NSMutableDictionary *temp = [NSMutableDictionary dictionary];
          LynxModuleInterceptor::CheckModuleIfNeed(module, temp, extra);
          [temp addEntriesFromDictionary:obj];
          [objcParams addObject:temp];
          [inv setArgument:(void *)&temp atIndex:i + 2];
        } else {
          [objcParams addObject:obj];
          [inv setArgument:(void *)&obj atIndex:i + 2];
        }
        continue;
      } else  // obj == nil
      {
        // issue: #1510
        reportError("sub class of NSObject");
      }

    } else {
      // issue: #1510
      if (num == nil) {
        switch (objCArgType[0]) {
          case _C_CHR:
            reportError("short");
            break;
          case _C_UCHR:
            reportError("unsigned char");
            break;
          case _C_SHT:
            reportError("short");
            break;
          case _C_USHT:
            reportError("unsigned short");
            break;
          case _C_INT:
            reportError("int");
            break;
          case _C_UINT:
            reportError("unsigned int");
            break;
          case _C_LNG:
            reportError("long");
            break;
          case _C_ULNG:
            reportError("unsigned long");
            break;
          case _C_LNG_LNG:
            reportError("long long");
            break;
          case _C_ULNG_LNG:
            reportError("unsigned long long");
            break;
          case _C_BOOL:
            reportError("bool");
            break;
          case _C_DBL:
            reportError("double");
            break;
          case _C_FLT:
            reportError("float");
            break;
          default:
            reportError("unknown number type");
            break;
        }
      }

      if (objCArgType[0] == _C_CHR) {
        char c = [num charValue];
        [inv setArgument:(void *)&c atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_UCHR) {
        unsigned char uc = [num unsignedCharValue];
        [inv setArgument:(void *)&uc atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_SHT) {
        short s = [num shortValue];
        [inv setArgument:(void *)&s atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_USHT) {
        unsigned short us = [num unsignedShortValue];
        [inv setArgument:(void *)&us atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_INT) {
        int ii = [num intValue];
        [inv setArgument:(void *)&ii atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_UINT) {
        unsigned int ui = [num unsignedIntValue];
        [inv setArgument:(void *)&ui atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_LNG) {
        long l = [num longValue];
        [inv setArgument:(void *)&l atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_ULNG) {
        unsigned long ul = [num unsignedLongValue];
        [inv setArgument:(void *)&ul atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_LNG_LNG) {
        long long ll = [num longLongValue];
        [inv setArgument:(void *)&ll atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_ULNG_LNG) {
        unsigned long long ull = [num unsignedLongLongValue];
        [inv setArgument:(void *)&ull atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_BOOL) {
        BOOL b = [num boolValue];
        [inv setArgument:(void *)&b atIndex:i + 2];
        continue;
      } else if (objCArgType[0] == _C_FLT) {
        float f = [num floatValue];
        [inv setArgument:(void *)&f atIndex:i + 2];
      } else if (objCArgType[0] == _C_DBL) {
        double d = [num doubleValue];
        [inv setArgument:(void *)&d atIndex:i + 2];
      }
    }
  }
  return inv;
}

base::expected<std::unique_ptr<pub::Value>, std::string> LynxModuleDarwin::invokeObjCMethod(
    const std::string &methodName, uint64_t invoke_session, SEL selector, const pub::Value *args,
    size_t count, int32_t &callErrorCode, const CallbackMap &callbacks) {
  NSMutableArray *retainedObjectsForInvocation = [NSMutableArray arrayWithCapacity:count + 2];
  NSMethodSignature *methodSignature =
      [[instance_ class] instanceMethodSignatureForSelector:selector];
  NSUInteger argumentsCount = methodSignature.numberOfArguments;

  std::string jsb_func_name;

  if (count > 0 && args) {
    jsb_func_name =
        LynxModuleInterceptor::GetJSBFuncName(instance_, args->GetValueAtIndex(0).get());
  }

  auto &inst = instance_;
  auto lock_delegate = delegate_.lock();

  // issue: #1510
  // TODO: argumentsCount - 4 == count means this is a promise
  // THUS THIS ARGUMENT CHECK CANNOT DETECT IF __argumentsCount - 4 != count__ !!
  if (argumentsCount - 2 != count && argumentsCount - 4 != count) {
    // issue: #1510
    // if #arg is __MORE__, an exception will be thrown on parsing type of args,
    // thus the function will not be invoked
    callErrorCode = error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_NUM;
    auto invokedErrorMessage = std::string{" invoked with wrong number of arguments, expected "}
                                   .append(std::to_string(argumentsCount - 2))
                                   .append(" but got ")
                                   .append(std::to_string(count))
                                   .append(".");
    if (argumentsCount - 2 < count) {
      if (lock_delegate) {
        lock_delegate->OnErrorOccurred(
            module_name_, methodName,
            base::LynxError{callErrorCode,
                            LynxModuleUtils::GenerateErrorMessage(module_name_, methodName,
                                                                  invokedErrorMessage),
                            "Please check the arguments.", base::LynxErrorLevel::Error});
      }
      return base::unexpected("invoke LynxModule:" + module_name_ + " method:" + methodName +
                              " parameter error");
    }
  }

  NSMutableDictionary *extra = [NSMutableDictionary dictionary];
  std::ostringstream time_s;
  time_s << invoke_session;
  @try {
    if (methodSessionBlocks_.count > 0) {
      for (LynxMethodSessionBlock sessionBlock in methodSessionBlocks_) {
        [extra
            addEntriesFromDictionary:sessionBlock(
                                         jsb_func_name.size() > 0
                                             ? [NSString stringWithUTF8String:jsb_func_name.c_str()]
                                             : NSStringFromSelector(selector),
                                         NSStringFromClass([inst class]),
                                         [NSString stringWithUTF8String:time_s.str().c_str()],
                                         namescope_)
                                         ?: @{}];
      }
    }
  } @catch (NSException *exception) {
    LOGE("Exception happened in LynxMethodSessionBlocks! Error message: "
         << genExceptionErrorMessage(exception));
    if (lock_delegate) {
      auto error_msg = std::string{" has been rejected by LynxMethodSessionBlock!"};
      lock_delegate->OnErrorOccurred(
          module_name_, methodName,
          base::LynxError{
              error::E_NATIVE_MODULES_COMMON_SYSTEM_AUTHORIZATION_ERROR,
              LynxModuleUtils::GenerateErrorMessage(module_name_, methodName, error_msg),
              "Please check the arguments.", base::LynxErrorLevel::Error});
    }
  }

  NSInvocation *inv = getMethodInvocation(instance_, methodName, selector, args, count,
                                          retainedObjectsForInvocation, callErrorCode,
                                          invoke_session, extra, callbacks);
  @try {
    if (methodAuthBlocks_.count > 0 && tasm::LynxEnv::GetInstance().IsPiperMonitorEnabled()) {
      std::ostringstream time_s;
      time_s << invoke_session;
      for (LynxMethodBlock authBlock in methodAuthBlocks_) {
        if (!authBlock(jsb_func_name.size() > 0
                           ? [NSString stringWithUTF8String:jsb_func_name.c_str()]
                           : NSStringFromSelector(selector),
                       NSStringFromClass([inst class]),
                       [NSString stringWithUTF8String:time_s.str().c_str()], inv)) {
          auto error_msg = std::string{" has been rejected by LynxMethodAuthBlocks!"};
          LOGE(module_name_ << "." << methodName << error_msg);
          if (lock_delegate) {
            lock_delegate->OnErrorOccurred(
                module_name_, methodName,
                base::LynxError{
                    error::E_NATIVE_MODULES_COMMON_AUTHORIZATION_ERROR,
                    LynxModuleUtils::GenerateErrorMessage(module_name_, methodName, error_msg),
                    "please ask RD of Lynx or client for help.", base::LynxErrorLevel::Error});
          }
          return std::make_unique<lynx::pub::ValueImplDarwin>(nil);
        }
      }
    }
  } @catch (NSException *exception) {
    auto error_msg = genExceptionErrorMessage(exception);
    LOGE("Exception happened in LynxMethodAuthBlocks! Error message: " << error_msg);
    if (lock_delegate) {
      lock_delegate->OnErrorOccurred(
          module_name_, methodName,
          base::LynxError{
              error::E_NATIVE_MODULES_COMMON_AUTHORIZATION_ERROR,
              LynxModuleUtils::GenerateErrorMessage(module_name_, methodName, error_msg),
              "please ask RD of Lynx or client for help.", base::LynxErrorLevel::Error});
    }
  }

  if (inv == nil) {
    return base::unexpected(
        "Exception happened in getMethodInvocation when convert js value to objc value.");
  }

  if (argumentsCount - 4 == count) {
    LOGE("LynxModule, invokeObjCMethod, module: " << module_name_ << " method: " << methodName
                                                  << " is a promise");
    tasm::report::FeatureCounter::Instance()->Count(
        tasm::report::LynxFeature::CPP_USE_NATIVE_PROMISE);
    // objc arguments: [this, _cmd, args..., resoledBlock, rejectedBlock]
    auto promise_ret = createPromise(*scope_rts_.back(), ^(Runtime &rt,
                                                           LynxPromiseResolveBlock resolveBlock,
                                                           LynxPromiseRejectBlock rejectBlock) {
      @try {
        [inv setArgument:(void *)&resolveBlock atIndex:count + 2];
        [inv setArgument:(void *)&rejectBlock atIndex:count + 3];
        [retainedObjectsForInvocation addObject:resolveBlock];
        [retainedObjectsForInvocation addObject:rejectBlock];
        auto opt_value = PerformMethodInvocation(inv, instance_);
        // issue: #1510
        LOGV("LynxModuleDarwin::invokeObjCMethod, module: "
             << module_name_ << " method: " << methodName
             << " |PROMISE|, did PerformMethodInvocation, success:" << opt_value.has_value());
      } @catch (NSException *exception) {
        _LogE(@"Exception '%@' was thrown while invoking function %s on target %@\n call stack: %@",
              exception, methodName.c_str(), instance_, exception.callStackSymbols);
        if (lock_delegate) {
          lock_delegate->OnErrorOccurred(
              module_name_, methodName,
              base::LynxError{
                  error::E_NATIVE_MODULES_EXCEPTION,
                  LynxModuleUtils::GenerateErrorMessage(module_name_, methodName,
                                                        genExceptionErrorMessage(exception)),
                  "This error is caught by native, please ask RD of Lynx or client for help.",
                  base::LynxErrorLevel::Error});
        }
      }
    });
    if (promise_ret.has_value()) {
      scope_native_promise_rets_.push_back(
          std::optional<piper::Value>(std::move(promise_ret.value())));
      // hack here, this will be delete later.
      return base::unexpected<std::string>("__IS_NATIVE_PROMISE__");
    } else {
      return base::unexpected<std::string>(std::move(promise_ret.error()));
    }
  }
  auto res = PerformMethodInvocation(inv, instance_);
  if (!res.has_value()) {
    if (lock_delegate) {
      lock_delegate->OnErrorOccurred(
          module_name_, methodName,
          base::LynxError{
              error::E_NATIVE_MODULES_COMMON_RETURN_ERROR,
              LynxModuleUtils::GenerateErrorMessage(module_name_, methodName, "return error"),
              "This error is caught by native, please ask RD of Lynx or client for help.",
              base::LynxErrorLevel::Error});
    }
    return res;
  }
  LOGV("LynxModuleDarwin::invokeObjCMethod, module: " << module_name_ << " method: " << methodName
                                                      << " did PerformMethodInvocation");
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "OnMethodInvoked");
  return res;
}

base::expected<piper::Value, std::string> LynxModuleDarwin::createPromise(
    Runtime &runtime, PromiseInvocationBlock invoke) {
  if (!invoke) {
    return piper::Value::undefined();
  }

  auto Promise = runtime.global().getPropertyAsFunction(runtime, "Promise");
  if (!Promise) {
    return base::unexpected<std::string>("Can't find Promise.");
  }

  PromiseInvocationBlock invokeCopy = [invoke copy];
  piper::Function fn = piper::Function::createFromHostFunction(
      runtime, piper::PropNameID::forAscii(runtime, "fn"), 2,
      [invokeCopy, delegate = delegate_,
       weak_module_delegate = std::weak_ptr<ModuleDelegate>(scope_module_delegates_.back())](
          piper::Runtime &rt, const piper::Value &thisVal, const piper::Value *args,
          size_t count) -> base::expected<piper::Value, piper::JSINativeException> {
        auto module_delegate_lock = weak_module_delegate.lock();
        if (!module_delegate_lock) {
          return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION("ModuleDelegate has been destroyed."));
        }
        piper::Scope scope(rt);
        if (count != 2) {
          return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
              "Promise must pass constructor function two args. Passed " + std::to_string(count) +
              " args."));
        }
        if (!invokeCopy) {
          return piper::Value::undefined();
        }
        int64_t resolveCallbackId =
            module_delegate_lock->RegisterJSCallbackFunction(args[0].getObject(rt).getFunction(rt));
        int64_t rejectCallbackId =
            module_delegate_lock->RegisterJSCallbackFunction(args[1].getObject(rt).getFunction(rt));
        if (resolveCallbackId == ModuleCallback::kInvalidCallbackId ||
            rejectCallbackId == ModuleCallback::kInvalidCallbackId) {
          LOGW("LynxModuleDarwin::create promise failed, LynxRuntime has destroyed");
          return piper::Value::undefined();
        }

        LOGV("LynxModuleDarwin::createPromise, resolve block id: "
             << resolveCallbackId << ", reject block id: " << rejectCallbackId);
        __block BOOL resolveWasCalled = NO;
        __block BOOL rejectWasCalled = NO;

        LynxPromiseResolveBlock resolveBlock = ^(id result) {
          if (rejectWasCalled) {
            LOGE("Tried to resolve a promise after it's already been rejected.");
            return;
          }
          if (resolveWasCalled) {
            LOGE("Tried to resolve a promise more than once.");
            return;
          }
          auto lock_delegate = delegate.lock();
          if (lock_delegate) {
            LOGW("Promise has been destroyed.");
            return;
          }

          auto resolveCallback = std::make_shared<piper::ModuleCallback>(resolveCallbackId);
          auto array = lock_delegate->GetValueFactory()->CreateArray();
          array->PushValueToArray(std::make_unique<pub::ValueImplDarwin>(result));
          resolveCallback->SetArgs(std::move(array));
          LOGV("LynxModule, LynxResolveBlock, put to JSThread id: " << resolveCallbackId);
          module_delegate_lock->CallJSCallback(resolveCallback, rejectCallbackId);
        };
        LynxPromiseRejectBlock rejectBlock = ^(NSString *code, NSString *message) {
          if (resolveWasCalled) {
            LOGE("Tried to reject a promise after it's already been resolved.");
            return;
          }
          if (rejectWasCalled) {
            LOGE("Tried to reject a promise more than once.");
            return;
          }
          auto lock_delegate = delegate.lock();
          if (lock_delegate) {
            LOGW("Promise has been destroyed.");
            return;
          }
          auto strongRejectWrapper = std::make_shared<piper::ModuleCallback>(rejectCallbackId);
          NSDictionary *jsError = @{@"errorCode" : code, @"message" : message};
          auto array = lock_delegate->GetValueFactory()->CreateArray();
          array->PushValueToArray(std::make_unique<pub::ValueImplDarwin>(jsError));
          strongRejectWrapper->SetArgs(std::move(array));
          rejectWasCalled = YES;
          LOGV("LynxModule, LynxRejectBlock, put to JSThread id: " << rejectCallbackId);
          module_delegate_lock->CallJSCallback(strongRejectWrapper, resolveCallbackId);
        };

        invokeCopy(rt, resolveBlock, rejectBlock);
        return piper::Value::undefined();
      });

  auto res = Promise->callAsConstructor(runtime, fn);
  if (res.has_value()) {
    return std::move(*res);
  } else {
    return base::unexpected<std::string>("Promise callAsConstructor failed.");
  }
}

#pragma mark - Functions Implementations.

base::expected<std::unique_ptr<pub::Value>, std::string> PerformMethodInvocation(NSInvocation *inv,
                                                                                 const id module) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, "Fire");
  const char *returnType = [[inv methodSignature] methodReturnType];
  void (^block)() = ^{
    [inv invokeWithTarget:module];
    if (returnType[0] == _C_VOID) {
      return;
    }
  };
  block();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);

#ifndef GET_RETURN_VAULE_WITH
#define GET_RETURN_VAULE_WITH(type)                    \
  type value_for_type;                                 \
  [inv getReturnValue:&value_for_type];                \
  return std::make_unique<lynx::pub::ValueImplDarwin>( \
      [NSNumber numberWithDouble:(double)value_for_type]);
#endif

  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "ObjCValueToJSIValue");
  switch (returnType[0]) {
    case _C_VOID:
      return std::make_unique<lynx::pub::ValueImplDarwin>(nil);
    case _C_ID: {
      void *rawResult;
      [inv getReturnValue:&rawResult];
      id result = (__bridge id)rawResult;
      return std::make_unique<lynx::pub::ValueImplDarwin>(result);
    }
    case _C_SHT: {
      GET_RETURN_VAULE_WITH(short);
    }
    case _C_CHR: {
      GET_RETURN_VAULE_WITH(char);
    }
    case _C_UCHR: {
      GET_RETURN_VAULE_WITH(unsigned char);
    }
    case _C_USHT: {
      GET_RETURN_VAULE_WITH(unsigned short);
    }
    case _C_INT: {
      GET_RETURN_VAULE_WITH(int);
    }
    case _C_UINT: {
      GET_RETURN_VAULE_WITH(unsigned int);
    }
    case _C_LNG: {
      GET_RETURN_VAULE_WITH(long);
    }
    case _C_ULNG: {
      GET_RETURN_VAULE_WITH(unsigned long);
    }
    case _C_LNG_LNG: {
      GET_RETURN_VAULE_WITH(long long);
    }
    case _C_ULNG_LNG: {
      GET_RETURN_VAULE_WITH(unsigned long long);
    }
    case _C_BOOL: {
      GET_RETURN_VAULE_WITH(bool);
    }
    case _C_FLT: {
      GET_RETURN_VAULE_WITH(float);
    }
    case _C_DBL: {
      GET_RETURN_VAULE_WITH(double);
    }
  }
  _LogE(@"LynxModule, PerformMethodInvocation, returnType[0]: %c is an unknown type, return "
        @"undefined instead",
        returnType[0]);
  return std::make_unique<lynx::pub::ValueImplDarwin>(nil);
}

LynxCallbackBlock LynxModuleDarwin::ConvertModuleCallbackToCallbackBlock(
    std::shared_ptr<LynxModuleCallback> callback, const std::string &method_name,
    const pub::Value *first_arg, uint64_t start_time) {
  __block int64_t callback_id = callback->CallbackId();
  LOGV("LynxModuleDarwin::ConvertModuleCallbackToCallbackBlock, |JS FUNCTION| id: "
       << callback_id << " " << module_name_ << "." << method_name);

  std::weak_ptr<Delegate> delegate(delegate_);

  __block BOOL wrapperWasCalled = NO;
  //  Some JSB implement will use first arg as JSB function name, so we need first
  //  arg for tracing.
  __block std::string jsb_func_name = LynxModuleInterceptor::GetJSBFuncName(instance_, first_arg);
  __block uint64_t start_time_copy = start_time;
  __block std::string module_name_copy = module_name_;
  __block std::string schema_copy = schema_;
  __block std::string method_name_copy = method_name;
  ALLOW_UNUSED_TYPE uint64_t callback_flow_id = callback->CallbackFlowId();

  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY_JSB, "CreateJSB Callback",
                      [=](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_flow_ids(callback_flow_id);
                        auto *debug = ctx.event()->add_debug_annotations();
                        debug->set_name("startTimestamp");
                        debug->set_string_value(std::to_string(start_time));
                      });
  return ^(id response) {
    if (wrapperWasCalled) {
      LOGR("LynxModule, callback id: " << callback_id << " is called more than once.");
      return;
    }
    wrapperWasCalled = YES;
    auto lock_delegate = delegate.lock();
    if (!lock_delegate) {
      LOGR("LynxModuleCallback has been destroyed. id:" << callback_id);
      return;
    }

    auto array = lock_delegate->GetValueFactory()->CreateArray();
    array->PushValueToArray(std::make_unique<pub::ValueImplDarwin>(response));
    callback->SetArgs(std::move(array));

    if (tasm::LynxEnv::GetInstance().IsPiperMonitorEnabled() &&
        [response isKindOfClass:[NSDictionary class]] && ((NSDictionary *)response).count > 0) {
      tasm::LynxEnvDarwin::onPiperResponsed(
          module_name_copy, jsb_func_name.size() == 0 ? method_name_copy : jsb_func_name,
          schema_copy, response, std::to_string(start_time_copy));
    }
    LOGV("LynxModule, LynxCallbackBlock, put function to JSThread, "
         << "callback id: " << callback_id << "piper::ModuleCallback: " << callback);
    lock_delegate->InvokeCallback(callback);
  };
}

}  // namespace piper
}  // namespace lynx
