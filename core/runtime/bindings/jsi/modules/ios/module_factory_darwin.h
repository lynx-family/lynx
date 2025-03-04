// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_MODULE_FACTORY_DARWIN_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_MODULE_FACTORY_DARWIN_H_

#import <Foundation/Foundation.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#import "LynxThreadSafeDictionary.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/runtime/bindings/jsi/modules/ios/lynx_module_darwin.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"

@class LynxContext;
@protocol LynxModule;

@interface LynxModuleWrapper : NSObject

@property(nonatomic, readwrite, strong) Class<LynxModule> moduleClass;
@property(nonatomic, readwrite, strong) id param;
@property(nonatomic, readwrite, weak) NSString *namescope;

@end

namespace lynx {
class DarwinEmbedder;
namespace runtime {
class LynxRuntime;
}  // namespace runtime

namespace piper {
class ModuleFactoryDarwin : public NativeModuleFactory {
 public:
  ModuleFactoryDarwin();
  virtual ~ModuleFactoryDarwin();
  void registerModule(Class<LynxModule> cls);
  void registerModule(Class<LynxModule> cls, id param);
  void registerMethodAuth(LynxMethodBlock block);
  void registerExtraInfo(NSDictionary *extra);
  void registerMethodSession(LynxMethodSessionBlock block);
  NSMutableDictionary<NSString *, id> *moduleWrappers();
  NSMutableDictionary<NSString *, id> *extraWrappers();
  NSMutableArray<LynxMethodBlock> *methodAuthWrappers();
  NSMutableArray<LynxMethodSessionBlock> *methodSessionWrappers();
  void addWrappers(NSMutableDictionary<NSString *, id> *wrappers);
  // Only used in LynxBackgroundRuntime Standalone to craete LynxView, we already register
  // some modules in RuntimeOptions and we don't want the modules on LynxViewBuilder overwrite it.
  void addModuleParamWrapperIfAbsent(NSMutableDictionary<NSString *, id> *wrappers);

  std::shared_ptr<LynxNativeModule> CreateModule(const std::string &name) override;
  void SetModuleExtraInfo(std::shared_ptr<ModuleDelegate> delegate) override {
    module_delegate_ = std::move(delegate);
  }

  LynxThreadSafeDictionary<NSString *, id> *getModuleClasses() { return modulesClasses_; }

  std::shared_ptr<ModuleFactoryDarwin> parent;
  LynxContext *context;
  LynxThreadSafeDictionary<NSString *, id> *modulesClasses_;
  NSMutableDictionary<NSString *, id> *extra_;
  id lynxModuleExtraData_;
  NSMutableArray<LynxMethodBlock> *methodAuthBlocks_;
  NSMutableArray<LynxMethodSessionBlock> *methodSessionBlocks_;

 private:
  friend DarwinEmbedder;
  std::shared_ptr<ModuleDelegate> module_delegate_;
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_MODULE_FACTORY_DARWIN_H_
