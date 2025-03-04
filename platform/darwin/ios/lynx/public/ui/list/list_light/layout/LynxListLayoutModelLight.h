// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

typedef NS_ENUM(NSInteger, LynxLayoutModelType) {
  LynxLayoutModelInvalid,
  LynxLayoutModelNormal,
  LynxLayoutModelFullSpan,
  LyngLayoutModelNative,  // A outer layoutModel from Native cells
};

@interface LynxListLayoutModelLight : NSObject
@property(nonatomic) CGRect frame;
@property(nonatomic) LynxLayoutModelType type;
@property(nonatomic) NSInteger zIndex;       // zPosition
@property(nonatomic) NSInteger columnIndex;  // In which column this model locates.

- (instancetype)initWithFrame:(CGRect)frame;
@end
