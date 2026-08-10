// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEngine.h"
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTemplateRender.h>
#import "LynxEnginePool.h"
#include "core/shell/lynx_engine_wrapper.h"

@implementation LynxEngine {
  std::unique_ptr<lynx::shell::LynxEngineWrapper> engine_wrapper_;
}

- (instancetype)initWithTemplateRender:(LynxTemplateRender *)render {
  if (self = [super init]) {
    _engineState = LynxEngineStateUnloaded;
    _lynxTemplateRender = render;
    _templateBundle = render.templateBundle;
    _lynxUIRenderer = render.lynxUIRenderer;
    _shadowNodeOwner = render.shadowNodeOwner;
    engine_wrapper_ = std::make_unique<lynx::shell::LynxEngineWrapper>();
  }
  return self;
}

- (BOOL)hasLoaded {
  return self.engineState != LynxEngineStateUnloaded;
}

- (BOOL)canBeReused {
  return self.engineState == LynxEngineStateReadyToBeReused;
}

- (lynx::shell::LynxEngineWrapper *)getEngineNative {
  return engine_wrapper_.get();
}

- (void)registerToReuse {
  self.engineState = LynxEngineStateReadyToBeReused;
  [[LynxEnginePool sharedInstance] registerReuseEngine:self];
}

- (BOOL)isRunOnCurrentTemplateRender:(LynxTemplateRender *)render {
  return render != nil && _lynxTemplateRender == render;
}

- (void)attachTemplateRender:(LynxTemplateRender *)render {
  _lynxTemplateRender = render;
}

- (void)detachEngine {
  if (self.lynxTemplateRender) {
    [self.lynxTemplateRender detachLynxEngine];
    self.lynxTemplateRender = nil;
  }
}

- (void)destroy {
  if (self.engineQueueRef) {
    [self.engineQueueRef removeObject:self];
  }
  self.engineState = LynxEngineStateDestroyed;
  engine_wrapper_->DestroyEngine();
}

@end
