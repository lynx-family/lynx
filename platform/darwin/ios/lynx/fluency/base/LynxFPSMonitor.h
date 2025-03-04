// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//  This file is copied from AWEPerfSDK, Created by Fengwei Liu.
//  Some unused logic remove, structuct changes are made, so it meets Lynx's code format.

//  Original License:
//
//  AWEPerfFPSMonitor.h
//  AWEPerfSDK
//
//  Created by Fengwei Liu on 8/12/19.
//

#import "LynxFPSRecord.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxFPSRecord;

@interface LynxFPSMonitor : NSObject

/// Create a shared fps monitor with all default optiions.
+ (instancetype)sharedInstance;

/// Set to enable/disable supports for ProMotion displays introduced on iPad Pro and iPhone 13
/// Pro/Max.
///
/// Defaults to YES on >=iOS 15, otherwise false.
@property(nonatomic, assign) BOOL supportsDynamicFrameRate;

/// This property returns NO when either:
/// 1. There are no more active fps records.
/// 2. If the app has entered background/resigned active.
@property(nonatomic, readonly, getter=isActive) BOOL active;

- (LynxFPSRecord *)beginWithKey:(id<NSCopying>)key;

- (nullable LynxFPSRecord *)pauseWithKey:(id<NSCopying>)key;

- (nullable LynxFPSRecord *)endWithKey:(id<NSCopying>)key NS_WARN_UNUSED_RESULT;

- (nullable LynxFPSRecord *)recordWithKey:(id<NSCopying>)key NS_WARN_UNUSED_RESULT;

@end

NS_ASSUME_NONNULL_END
