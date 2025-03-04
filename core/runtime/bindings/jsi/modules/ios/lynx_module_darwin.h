// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_LYNX_MODULE_DARWIN_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_LYNX_MODULE_DARWIN_H_

#import <Foundation/Foundation.h>
#import "LynxModule.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/expected.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/jsi/jsi.h"
#include "core/value_wrapper/darwin/value_impl_darwin.h"

namespace lynx {
namespace piper {
base::expected<std::unique_ptr<pub::Value>, std::string> PerformMethodInvocation(NSInvocation *inv,
                                                                                 const id module);

class LynxModuleDarwin : public LynxNativeModule {
 public:
  LynxModuleDarwin(id<LynxModule> module);
  void Destroy() override;

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string &method_name, std::unique_ptr<pub::Value> args, size_t count,
      const CallbackMap &callbacks) override;
  // TODO(liyanbo.monster): after remove native promise, delete this.
#if OS_IOS || OS_TVOS || OS_OSX
  void EnterInvokeScope(Runtime *rt, std::shared_ptr<ModuleDelegate> module_delegate) override;
  void ExitInvokeScope() override;
  std::optional<piper::Value> TryGetPromiseRet() override;
#endif

  void SetSchema(const std::string schema) { schema_ = schema; }
  void SetMethodAuth(NSMutableArray<LynxMethodBlock> *methodAuthBlocks) {
    methodAuthBlocks_ = methodAuthBlocks;
  };
  void SetMethodSession(NSMutableArray<LynxMethodSessionBlock> *methodSessionBlocks) {
    methodSessionBlocks_ = methodSessionBlocks;
  };
  void SetMethodScope(NSString *namescope) { namescope_ = namescope; }

  id instance_;

  NSDictionary<NSString *, NSString *> *methodLookup;

 private:
  NSString *namescope_;
  NSMutableArray<LynxMethodBlock> *methodAuthBlocks_;
  NSMutableArray<LynxMethodSessionBlock> *methodSessionBlocks_;
  NSDictionary *attributeLookup;
  std::string module_name_;
  std::string schema_;

  ALLOW_UNUSED_TYPE int64_t record_id_ = 0;

  // TODO(liyanbo.monster): after nativepromise delete, delete those.
  std::vector<Runtime *> scope_rts_;
  std::vector<std::shared_ptr<ModuleDelegate>> scope_module_delegates_;
  std::vector<std::optional<piper::Value>> scope_native_promise_rets_;

  using PromiseInvocationBlock = void (^)(Runtime &rt, LynxPromiseResolveBlock resolveWrapper,
                                          LynxPromiseRejectBlock rejectWrapper);

  base::expected<piper::Value, std::string> createPromise(Runtime &runtime,
                                                          PromiseInvocationBlock invoke);

  NSInvocation *getMethodInvocation(const id module, const std::string &methodName, SEL selector,
                                    const pub::Value *args, size_t count,
                                    NSMutableArray *retainedObjectsForInvocation,
                                    int32_t &callErrorCode, uint64_t start_time,
                                    NSDictionary *extra, const CallbackMap &callbacks);

  LynxCallbackBlock ConvertModuleCallbackToCallbackBlock(
      std::shared_ptr<LynxModuleCallback> callback, const std::string &method_name,
      const pub::Value *first_arg, uint64_t start_time);

  void buildLookupMap(NSDictionary<NSString *, NSString *> *);
  base::expected<std::unique_ptr<pub::Value>, std::string> invokeObjCMethod(
      const std::string &methodName, uint64_t invoke_session, SEL selector, const pub::Value *args,
      size_t count, int32_t &callErrorCode, const CallbackMap &callbacks);
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_LYNX_MODULE_DARWIN_H_
