// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LUIErrorHandling.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxView.h>

@interface LynxView ()

- (LynxTemplateRender* _Nullable)templateRender;
- (NSDictionary* _Nullable)getAllJsSource;
- (void)onLongPress;

@end
