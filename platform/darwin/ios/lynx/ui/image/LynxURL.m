// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxURL.h"

@implementation LynxURL

- (void)updatePreviousUrl {
  _preUrl = _url;
}

- (BOOL)isPreviousUrl {
  return [_url.absoluteString isEqualToString:_preUrl.absoluteString];
}

- (void)initResourceInformation {
  if (!_resourceInfo) {
    _resourceInfo = [NSMutableDictionary dictionary];
  }
  _resourceInfo[@"res_src"] = _url.absoluteString ?: @"";
  _resourceInfo[@"res_scene"] = @"lynx_image";
}

- (void)updateTimeStamp:(NSDate*)getImageTime startRequestTime:(NSDate*)startRequestTime {
  NSDate* completeRequestTime = [NSDate date];
  _fetchTime = [getImageTime timeIntervalSinceDate:startRequestTime];
  _completeTime = [completeRequestTime timeIntervalSinceDate:startRequestTime];
}

- (NSMutableDictionary*)reportInfo {
  if (nil == _reportInfo) {
    _reportInfo = [NSMutableDictionary dictionary];
  }
  return _reportInfo;
}
@end
