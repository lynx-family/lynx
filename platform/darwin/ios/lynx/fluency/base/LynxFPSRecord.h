// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//  This file is copied from AWEPerfSDK, Created by Fengwei Liu.
//  Some unused logic remove, structuct changes are made, so it meets Lynx's code format.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(UInt8, LynxFPSRecordState) {
  LynxFPSRecordStateNone = 0,
  LynxFPSRecordStatePaused,
  LynxFPSRecordStateActive,
  LynxFPSRecordStateEnded,
};

typedef struct {
  /// Number of frames.
  UInt32 frames;
  /// Monitoring duration in seconds.
  NSTimeInterval duration;

  /// Number of drop1 frames recorded.
  /// A frame is considered as a drop1 frame when it is late to display for at least 16.67ms.
  UInt32 drop1Count;
  /// The sum of frame duration in seconds for drop1 frames.
  NSTimeInterval drop1Duration;

  /// Number of drop3 frames recorded.
  /// A frame is considered as a drop3 frame when it is late to display for at least 3 * 16.67ms.
  UInt32 drop3Count;
  /// The sum of frame duration  in seconds  for drop3 frames.
  NSTimeInterval drop3Duration;

  /// Same as drop3, except the threshold becomes 7 * 16.67ms.
  UInt32 drop7Count;
  /// The sum of frame duration  in seconds for drop7 frames.
  NSTimeInterval drop7Duration;

  /// Same as drop3, except the threshold becomes 25 * 16.67ms.
  UInt32 drop25Count;
  /// The sum of frame duration  in seconds for drop25 frames.
  NSTimeInterval drop25Duration;

  NSTimeInterval hitchDuration;

} LynxFPSRawMetrics;

typedef struct {
  double fps;              // count/s
  double drop1PerSecond;   // count/s
  double drop3PerSecond;   // count/s
  double drop7PerSecond;   // count/s
  double drop25PerSecond;  // count/s
  double hitchRatio;       // s/s
  double drop1Ratio;       // ms/s
  double drop3Ratio;       // ms/s
  double drop7Ratio;       // ms/s
  double drop25Ratio;      // ms/s
} LynxFPSDerivedMetrics;

@interface LynxFPSRecord : NSObject <NSCopying> {
 @package
  id<NSCopying> _key;
  LynxFPSRecordState _state;
  LynxFPSRawMetrics _totalMetrics;
}

- (instancetype)initWithKey:(id<NSCopying>)key;

/// The key of the record entry.
@property(nonatomic, nullable, readonly) id<NSCopying> key;

/// The name of the record, available when key is a string.
@property(nonatomic, nullable, readonly) NSString *name;

#pragma mark Common Metrics

/// The number of complete frame intervals recorded during the monitoring duration.
@property(nonatomic, readonly) NSUInteger frames;

/// The duration of all complete frame intervals recorded during the monitoring duration.
@property(nonatomic, readonly) NSTimeInterval duration;

/// The average FPS calculated during the monitoring duration.
@property(nonatomic, readonly) double framesPerSecond;

@property(nonatomic, readwrite) NSInteger maximumFramesPerSecond;

#pragma mark Raw Metrics

/// The raw metrics collected in the whole duration of this record.
///
/// See LynxFPSRawMetrics for details.
@property(nonatomic, readonly) LynxFPSRawMetrics metrics;

#pragma mark Derived Metrics

/// The derived metrics collected in the whole duration of this record.
///
/// See LynxFPSDerivedMetrics for details.
@property(nonatomic, readonly) LynxFPSDerivedMetrics derivedMetrics;

/// Set timeout interval & completion handler.
/// @param timeout The timeout interval in seconds.
/// @param completion A block to be invoked when the `duration` exceeds the timeout interval for the
/// first time.
- (void)setTimeout:(NSTimeInterval)timeout
        completion:(void (^_Nullable)(LynxFPSRecord *))completion;

/// Clear the record's metrics.
- (void)reset;

@end

@interface LynxFPSRecord () {
 @package
  NSTimeInterval timeoutInterval;
  void (^timeoutCompletion)(LynxFPSRecord *);
}

@property(nonatomic, assign, readonly) LynxFPSRecordState state;

@end

NS_ASSUME_NONNULL_END
