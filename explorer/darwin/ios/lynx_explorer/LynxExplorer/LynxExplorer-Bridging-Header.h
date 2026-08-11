// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderEnv.h>
#import <LynxDevtool/LynxRecorderViewController.h>
#import "DemoGenericResourceFetcher.h"
#import "DemoMediaResourceFetcher.h"
#import "DemoTemplateResourceFetcher.h"
#import "ExplorerModule.h"
#import "LynxExplorerInput.h"
#import "LynxNodeAPILifecycleListener.h"
#import "LynxNodeAPIModule.h"
#import "LynxRecorderDefaultActionCallback.h"
#import "LynxSettingManager.h"
#import "LynxViewShellViewController.h"
#import "ScanViewController.h"
#import "TasmDispatcher.h"

NS_INLINE NSString *_Nullable LXExplorerRecorderURLPrefix(void) {
  return [LynxRecorderEnv sharedInstance].lynxRecorderUrlPrefix;
}
