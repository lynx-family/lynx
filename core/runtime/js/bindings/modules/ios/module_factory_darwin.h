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

#import <Lynx/LynxThreadSafeDictionary.h>
#include "core/public/jsb/native_module_factory.h"
#include "core/runtime/js/bindings/modules/ios/lynx_module_creator_darwin.h"
#include "core/runtime/js/bindings/modules/ios/lynx_module_darwin.h"
#include "core/runtime/js/bindings/modules/lynx_module_manager.h"

namespace lynx {
class DarwinEmbedder;

namespace piper {

class ModuleFactoryDarwin : public NativeModuleFactory {
 public:
  ModuleFactoryDarwin();
  virtual ~ModuleFactoryDarwin();

  // bind module creator
  void Bind(std::unique_ptr<ModuleCreatorDarwin> module_creator);
  void SetContextFinder(const std::shared_ptr<LynxContextFinderDarwin> &context_finder);
  std::shared_ptr<lynx::piper::LynxContextFinderDarwin> CurrentContextFinder();
  void DeleteLynxContextForInstance(NSString *instanceId);

  // register module class and param.
  void registerModule(Class<LynxModule> cls);
  void registerModule(Class<LynxModule> cls, id param);
  void registerMethodAuth(LynxMethodBlock block);
  void registerExtraInfo(NSDictionary *extra);
  void registerMethodSession(LynxMethodSessionBlock block);

  // register wrappers
  void addWrappers(NSMutableDictionary<NSString *, id> *wrappers);
  // Only used in LynxBackgroundRuntime Standalone to craete LynxView, we already register
  // some modules in RuntimeOptions and we don't want the modules on LynxViewBuilder overwrite it.
  void addModuleParamWrapperIfAbsent(NSMutableDictionary<NSString *, id> *wrappers);

  NSMutableDictionary<NSString *, id> *moduleWrappers();
  NSMutableDictionary<NSString *, id> *extraWrappers();
  NSMutableArray<LynxMethodBlock> *methodAuthWrappers();
  NSMutableArray<LynxMethodSessionBlock> *methodSessionWrappers();
  LynxThreadSafeDictionary<NSString *, id> *getModuleClasses() { return modulesClasses_; }

  // create module instance. used for lynx module manager
  std::shared_ptr<LynxNativeModule> CreateModule(const std::string &name) override;

  void SetModuleExtraInfo(std::shared_ptr<ModuleDelegate> delegate) {
    module_delegate_ = std::move(delegate);
  }

  std::shared_ptr<ModuleFactoryDarwin> parent;
  LynxThreadSafeDictionary<NSString *, id> *modulesClasses_;
  NSMutableDictionary<NSString *, id> *extra_;
  id lynxModuleExtraData_;

  // auth validator
  NSMutableArray<LynxMethodBlock> *methodAuthBlocks_;
  NSMutableArray<LynxMethodSessionBlock> *methodSessionBlocks_;

 private:
  friend DarwinEmbedder;
  std::shared_ptr<ModuleDelegate> module_delegate_;
  std::unique_ptr<ModuleCreatorDarwin> module_creator_;
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_MODULE_FACTORY_DARWIN_H_
