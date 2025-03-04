// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

typedef NS_OPTIONS(NSInteger, ReplayViewState) {
  DOWNLOAD_JSON_FILE = 0,
  PARSING_JSON_FILE,
  HANDLE_ACTION_LIST,
  INVALID_JSON_FILE,
  RECORD_ERROR_MISS_TEMPLATEJS
};

NS_ASSUME_NONNULL_BEGIN

@interface TestBenchStateReplayView : UIView
- (void)setReplayState:(NSInteger)stateCode;
@end

NS_ASSUME_NONNULL_END
