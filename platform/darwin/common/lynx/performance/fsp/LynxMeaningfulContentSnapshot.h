// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxUIMeaningfulContentProtocol.h>

NS_ASSUME_NONNULL_BEGIN

/// @class LynxMeaningfulContentInfo
/// @brief A class representing Meaningful Content (FSP) information.
@interface LynxMeaningfulContentInfo : NSObject

@property(nonatomic, assign) LynxUIMeaningfulContentStatus status;

@property(nonatomic, assign) CGRect rect;

@property(nonatomic, assign) int64_t firstPresentedTimestampMicros;

@end

/// @class LynxMeaningfulContentSnapshot
/// @brief A class representing a snapshot of Meaningful Content (FSP) information.
@interface LynxMeaningfulContentSnapshot : NSObject

@property(nonatomic, strong) NSArray<LynxMeaningfulContentInfo *> *contentsArray;

@property(nonatomic, assign) CGSize containerSize;

@end

NS_ASSUME_NONNULL_END
