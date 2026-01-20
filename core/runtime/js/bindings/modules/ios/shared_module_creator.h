// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_SHARED_MODULE_CREATOR_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_SHARED_MODULE_CREATOR_H_

#import <Foundation/Foundation.h>

#import <Lynx/LynxThreadSafeDictionary.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/runtime/js/bindings/modules/ios/lynx_module_creator_darwin.h"

@protocol LynxModule;

namespace lynx {
namespace runtime {
namespace js {
// Create LynxContextModuleInstance or LynxModuleInstance
class SharedModuleCreator : public ModuleCreatorDarwin {
 public:
  SharedModuleCreator();
  virtual ~SharedModuleCreator();

  id<LynxModule> Create(NSString* name, LynxModuleWrapper* module_classes) override;

  void Destroy() override;

  std::shared_ptr<LynxContextFinderDarwin> CurrentContextFinder() override;

  void SetContextFinder(const std::shared_ptr<LynxContextFinderDarwin>& context_finder) override;

 private:
  // instance cache
  LynxThreadSafeDictionary<NSString*, id>* moduleInstances_;
  // context finder: only one lynxcontext
  std::shared_ptr<LynxContextFinderDarwin> context_finder_;
};

class SharedLynxContextFinderDarwin : public LynxContextFinderDarwin {
 public:
  SharedLynxContextFinderDarwin();
  virtual ~SharedLynxContextFinderDarwin();

  LynxContext* FindContext(const std::string& unique_id) override;
  std::string FindSchema(const std::string& unique_id) override;
  void RegisterContext(const std::string& unique_id, LynxContext* context,
                       const std::string& schema) override;
  bool IsShared() override { return true; };

 private:
  // The LynxContext object will be used in BTS Thread , so a weak reference is used here.
  NSMapTable<NSString*, LynxContext*>* lynxContextWeakMap_;
  std::unordered_map<std::string, std::string> schemas_;
};
}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // !CORE_RUNTIME_BINDINGS_JSI_MODULES_IOS_SHARED_MODULE_CREATOR_H_
