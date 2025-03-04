// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxAnimationInfo.h"
#import "LynxFeatureCounter.h"
#import "LynxHeroTransition.h"
#import "LynxLog.h"
#import "LynxPropsProcessor.h"
#import "LynxUI+LynxHeroTransition.h"
#import "LynxUIContext+Internal.h"
#import "UIView+LynxHeroTransition.h"

@implementation LynxUI (LynxHeroTransition)

LYNX_PROP_SETTER("shared-element", setSharedElement, NSString*) {
  if (requestReset) {
    value = nil;
  }
  if (self.view.lynxHeroConfig.sharedElementName) {
    LLogWarn(@"shared-element can not be modified more than once. ");
    return;
  }
  self.view.lynxHeroConfig.sharedElementName = value;
  self.view.lynxHeroConfig.lynxUI = self;
  [[LynxHeroTransition sharedInstance] registerSharedElementsUI:self shareTag:value];
}

LYNX_PROP_SETTER("enter-transition-name", setEnterTransitionName, LynxAnimationInfo*) {
  if (requestReset) {
    value = nil;
  }
  if (self.view.lynxHeroConfig.enterTransitionName) {
    LLogWarn(@"enter-transition-name can not be modified more than once. ");
    return;
  }
  [LynxFeatureCounter count:LynxFeatureObjcEnterTransitionName
                 instanceId:[self.context instanceId]];
  self.view.lynxHeroConfig.enterTransitionName = value;
  self.view.lynxHeroConfig.lynxUI = self;
  [[LynxHeroTransition sharedInstance] registerEnterTransition:self anim:value];
}

LYNX_PROP_SETTER("exit-transition-name", setExitTransitionNatme, LynxAnimationInfo*) {
  if (requestReset) {
    value = nil;
  }
  if (self.view.lynxHeroConfig.exitTransitionName) {
    LLogWarn(@"exit-transition-name should not be modified more than once. ");
    return;
  }
  [LynxFeatureCounter count:LynxFeatureObjcExitTransitionName instanceId:[self.context instanceId]];
  self.view.lynxHeroConfig.exitTransitionName = value;
  self.view.lynxHeroConfig.lynxUI = self;
  [[LynxHeroTransition sharedInstance] registerExitTransition:self anim:value];
}

LYNX_PROP_SETTER("pause-transition-name", setPauseTransitionName, LynxAnimationInfo*) {
  if (requestReset) {
    value = nil;
  }
  if (self.view.lynxHeroConfig.pauseTransiitonName) {
    LLogWarn(@"pause-transition-name should not be modified more than once. ");
    return;
  }
  [LynxFeatureCounter count:LynxFeatureObjcPauseTransitionName
                 instanceId:[self.context instanceId]];
  self.view.lynxHeroConfig.pauseTransiitonName = value;
  self.view.lynxHeroConfig.lynxUI = self;
  [[LynxHeroTransition sharedInstance] registerPauseTransition:self anim:value];
}

LYNX_PROP_SETTER("resume-transition-name", setResumeTransitionName, LynxAnimationInfo*) {
  if (requestReset) {
    value = nil;
  }
  if (self.view.lynxHeroConfig.resumeTransitionName) {
    LLogWarn(@"resume-transition-name should not be modified more than once. ");
    return;
  }
  [LynxFeatureCounter count:LynxFeatureObjcResumeTransitionName
                 instanceId:[self.context instanceId]];
  self.view.lynxHeroConfig.resumeTransitionName = value;
  self.view.lynxHeroConfig.lynxUI = self;
  [[LynxHeroTransition sharedInstance] registerResumeTransition:self anim:value];
}

LYNX_PROP_SETTER("cross-page", setCrossPage, BOOL) {
  if (requestReset) {
    value = YES;
  }
  self.view.lynxHeroConfig.crossPage = value;
}

@end
