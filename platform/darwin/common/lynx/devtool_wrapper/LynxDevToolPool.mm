// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDevToolPool.h>
#import <Lynx/LynxDevtool.h>
#import <Lynx/LynxEnv.h>

#include "core/devtool_wrapper/devtool_pool.h"

namespace lynx {
namespace devtool {

class DevToolPoolDarwin : public DevToolPool {
 public:
  DevToolPoolDarwin(LynxDevToolPool* pool) { _pool = pool; }
  ~DevToolPoolDarwin() override = default;

  void CreateDevTool() override {
    __strong typeof(_pool) strong_pool = _pool;
    [strong_pool createDevTool];
  }

  void PopDevTool() override {
    __strong typeof(_pool) strong_pool = _pool;
    [strong_pool popDevTool];
  }

  std::shared_ptr<lepus::InspectorLepusObserver> OnMTSRuntimeCreated() override {
    __strong typeof(_pool) strong_pool = _pool;
    [strong_pool onMTSRuntimeCreated];
    return DevToolPool::OnMTSRuntimeCreated();
  }

  void OnTemplateBundleCreated(intptr_t bundle) {
    auto* bundle_ptr = reinterpret_cast<tasm::LynxTemplateBundle*>(bundle);
    bundle_ptr->SetDevToolPool(shared_from_this());
  }

 private:
  __weak LynxDevToolPool* _pool;
};

}  // namespace devtool
}  // namespace lynx

@implementation LynxDevToolPool {
  NSMutableArray<LynxDevtool*>* _devtools;
  BOOL _debuggable;
  NSString* _url;
  std::shared_ptr<lynx::devtool::DevToolPoolDarwin> devtool_pool_;
}

- (instancetype)initWithURL:(NSString*)url debuggable:(BOOL)debuggable {
  self = [super init];
  if (self) {
    _devtools = [[NSMutableArray alloc] init];
    _debuggable = debuggable;
    _url = url;
    if (_url == nil || [_url isEqualToString:@""]) {
      static NSString* const kAnonymousBundleURL = @"anonymous bundle";
      _url = kAnonymousBundleURL;
    }
    devtool_pool_ = std::make_shared<lynx::devtool::DevToolPoolDarwin>(self);
  }
  return self;
}

- (void)onTemplateBundleCreated:(intptr_t)bundle {
  devtool_pool_->OnTemplateBundleCreated(bundle);
}

- (void)createDevTool {
  if (LynxEnv.sharedInstance.lynxDebugEnabled) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
    [_devtools addObject:[[LynxDevtool alloc] initWithLynxView:nil debuggable:_debuggable]];
#pragma clang diagnostic pop
  }
}

- (void)popDevTool {
  if ([_devtools count] == 0) {
    return;
  }
  [_devtools removeLastObject];
}

- (void)onMTSRuntimeCreated {
  if ([_devtools count] == 0) {
    return;
  }
  LynxDevtool* devtool = [_devtools lastObject];
  [devtool onMTSRuntimeCreated:(intptr_t)devtool_pool_.get()];
  static int next_context_id = 0;
  NSString* sessionURL = [NSString stringWithFormat:@"%@ - context%d", _url, next_context_id++];
  [devtool attachDebugBridge:sessionURL];
}

@end
