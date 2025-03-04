// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchStateReplayView.h"

#define SCREEN_SIZE [[UIScreen mainScreen] bounds].size

@interface TestBenchStateReplayView ()
@property NSString *info;
@property float progress;
@property UITextView *stateDesc;
@property UIProgressView *stateProgress;
@property NSMutableArray *stateArray;
@end

@implementation TestBenchStateReplayView

- (id)init {
  if (self = [super init]) {
    self.stateDesc = [[UITextView alloc] init];
    self.stateProgress = [[UIProgressView alloc] init];
    [self setSize];
    [self createStateArray];
  }
  return self;
}

- (void)setSize {
  // set view size
  CGRect rect = self.bounds;
  rect.size.height = SCREEN_SIZE.height * 0.3;
  rect.size.width = SCREEN_SIZE.width * 0.7;
  self.bounds = rect;
  self.center = CGPointMake(SCREEN_SIZE.width * 0.5, SCREEN_SIZE.height * 0.4);
  self.backgroundColor = [UIColor whiteColor];

  self.stateDesc.textColor = [UIColor blackColor];
  self.stateDesc.textAlignment = NSTextAlignmentCenter;
  self.stateDesc.frame = CGRectMake(0, 0, self.frame.size.width, self.frame.size.height * 0.4);
  [self.stateDesc setFont:[UIFont systemFontOfSize:30]];

  self.stateProgress.frame = CGRectMake(0, self.frame.size.height * 0.6, self.frame.size.width,
                                        self.frame.size.height * 0.4);
  self.stateProgress.trackTintColor = [UIColor grayColor];
  self.stateProgress.progressTintColor = [UIColor orangeColor];

  [self addSubview:self.stateDesc];
  [self addSubview:self.stateProgress];
}

- (float)getProgress:(NSInteger)stateCode {
  return (stateCode + 1) / ([_stateArray count] + 1);
}

- (void)createStateArray {
  _stateArray = [[NSMutableArray alloc] init];
  [_stateArray insertObject:@"Download Json File" atIndex:DOWNLOAD_JSON_FILE];
  [_stateArray insertObject:@"Parsing Json File" atIndex:PARSING_JSON_FILE];
  [_stateArray insertObject:@"Handle Action List" atIndex:HANDLE_ACTION_LIST];
  [_stateArray insertObject:@"Invalid JSON File" atIndex:INVALID_JSON_FILE];
  [_stateArray insertObject:@"Record Error:Miss template.js" atIndex:RECORD_ERROR_MISS_TEMPLATEJS];
}

- (void)setReplayState:(NSInteger)stateCode {
  __weak typeof(self) _self = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong typeof(_self) strongSelf = _self;
    if (stateCode >= 0 && stateCode < [strongSelf.stateArray count]) {
      strongSelf.stateDesc.text = strongSelf.stateArray[stateCode];
      [strongSelf.stateProgress setProgress:[strongSelf getProgress:stateCode] animated:YES];
    } else {
      strongSelf.stateDesc.text = @"Unknown State";
      [strongSelf.stateProgress setProgress:0];
    }
  });
}

@end
