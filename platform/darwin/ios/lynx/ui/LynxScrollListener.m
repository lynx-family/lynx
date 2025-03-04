// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxScrollListener.h"
#import "LUIBodyView.h"

@implementation LynxScrollInfo

+ (instancetype)infoWithScrollView:(UIScrollView *)scrollView
                           tagName:(NSString *)tagName
              scrollMonitorTagName:(NSString *)scrollMonitorTagName {
  LynxScrollInfo *info = [[self alloc] init];
  info.scrollView = scrollView;
  info.tagName = tagName;
  info.scrollMonitorTagName = scrollMonitorTagName;
  return info;
}

- (void)setLynxView:(UIView<LUIBodyView> *)lynxView {
  _lynxView = lynxView;
  _lynxViewUrl = lynxView.url;
}

- (BOOL)isEqual:(id)object {
  if (![object isKindOfClass:LynxScrollInfo.class]) {
    return NO;
  }
  LynxScrollInfo *info = (LynxScrollInfo *)object;
  return self.lynxViewUrl == info.lynxViewUrl && self.tagName == info.tagName &&
         self.scrollMonitorTagName == info.scrollMonitorTagName;
}

- (NSUInteger)hash {
  NSUInteger hashValue = self.lynxViewUrl.hash * 31 + self.tagName.hash;
  return hashValue * 31 + self.scrollMonitorTagName.hash;
}

- (id)copyWithZone:(NSZone *)zone {
  LynxScrollInfo *info = [[LynxScrollInfo allocWithZone:zone] init];
  info.lynxView = self.lynxView;
  // lynxView maybe nil, so we need reset lynxViewUrl value.
  info->_lynxViewUrl = self.lynxViewUrl;
  info.tagName = self.tagName;
  info.scrollMonitorTagName = self.scrollMonitorTagName;
  info.scrollView = self.scrollView;
  info.selector = self.selector;
  info.decelerate = self.decelerate;
  return info;
}

@end
