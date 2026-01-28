// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_IOS_COMMON_MODULE_CREATOR_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_IOS_COMMON_MODULE_CREATOR_H_

#import <Foundation/Foundation.h>

#import <Lynx/LynxThreadSafeDictionary.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/runtime/js/bindings/modules/ios/lynx_module_creator_darwin.h"

@protocol LynxModule;

namespace lynx {
namespace piper {

// Create LynxContextModuleInstance or LynxModuleInstance
class CommonModuleCreator : public ModuleCreatorDarwin {
 public:
  CommonModuleCreator();
  virtual ~CommonModuleCreator();

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

// Only one LynxContext cache
class CommonLynxContextFinderDarwin : public LynxContextFinderDarwin {
 public:
  CommonLynxContextFinderDarwin();
  virtual ~CommonLynxContextFinderDarwin();

  LynxContext* FindContext(const std::string& unique_id) override;
  std::string FindSchema(const std::string& unique_id) override;
  void RegisterContext(const std::string& unique_id, LynxContext* context,
                       const std::string& schema) override;
  void Destroy() override;
  bool IsShared() override { return false; };

 private:
  LynxContext* lynxContext_;
  std::string schema_;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_IOS_COMMON_MODULE_CREATOR_H_
