// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundCapInsets.h"
#import "LynxUI.h"
#import "LynxUnitUtils.h"

@implementation LynxBackgroundCapInsets
- (instancetype)initWithParams:(NSString*)capInsetsString {
  if (self = [super init]) {
    NSArray* capInsetsProps = [capInsetsString componentsSeparatedByString:@" "];
    const NSInteger count = [capInsetsProps count];

    _capInsets.top = [self toCapInsetValue:count > 0 ? capInsetsProps[0] : nil];
    _capInsets.right = count > 1 ? [self toCapInsetValue:capInsetsProps[1]] : _capInsets.top;
    _capInsets.bottom = count > 2 ? [self toCapInsetValue:capInsetsProps[2]] : _capInsets.top;
    _capInsets.left = count > 3 ? [self toCapInsetValue:capInsetsProps[3]] : _capInsets.right;
  }
  return self;
}

- (CGFloat)toCapInsetValue:(NSString*)unitValue {
  const CGSize rootSize = _ui.context.rootView.frame.size;
  LynxScreenMetrics* screenMetrics = _ui.context.screenMetrics;
  return [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                    unitValue:unitValue
                                 rootFontSize:((LynxUI*)_ui.context.rootUI).fontSize
                                  curFontSize:_ui.fontSize
                                    rootWidth:rootSize.width
                                   rootHeight:rootSize.height
                                withDefaultPt:0];
}

- (void)reset {
  _capInsets = UIEdgeInsetsZero;
}
@end
