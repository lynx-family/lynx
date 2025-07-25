// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFSPTracer+Internal.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxUIOwner+Private.h>
#import <Lynx/LynxView.h>
#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>
#import <LynxDevtool/LynxDevToolEnv.h>
#import <LynxDevtool/LynxTimingOverlayHelper+FSP.h>
#import <LynxDevtool/LynxTimingOverlayHelper+Internal.h>
#import <LynxDevtool/LynxTimingOverlayHelper.h>
#import <UIKit/UIKit.h>

#include "lynx/core/services/fsp_tracing/fsp_tracer.h"

#pragma mark - LynxTimingOverlayFSPState

typedef NS_ENUM(NSInteger, LynxTimingOverlayFSPState) {
  LynxTimingOverlayFSPStateNone = 0,
  LynxTimingOverlayFSPStateTracking,
  LynxTimingOverlayFSPStateStable,
  LynxTimingOverlayFSPStateSuccess,
  LynxTimingOverlayFSPStateFailure,
};

#pragma mark - LynxTimingFSPInfo

@interface LynxTimingFSPInfo ()

@property(nonatomic, assign) uint64_t startTimestamp;
@property(nonatomic, assign) LynxTimingOverlayFSPState state;
@property(nonatomic, assign) LynxFSPResult result;
@property(nonatomic, assign) LynxFSPTracerStopReason stopReason;
@property(nonatomic, assign) uintptr_t opaqueTracer;

@end

@implementation LynxTimingFSPInfo

- (instancetype)init {
  self = [super init];
  if (self) {
    self.state = LynxTimingOverlayFSPStateNone;
    self.startTimestamp = lynx::base::CurrentSystemTimeMicroseconds();
    self.result = (LynxFSPResult){0};
    self.stopReason = kLynxFSPTracerStopReasonNone;
    self.opaqueTracer = 0;
  }
  return self;
}

- (BOOL)isEnded {
  return self.state == LynxTimingOverlayFSPStateFailure ||
         self.state == LynxTimingOverlayFSPStateSuccess;
}

- (BOOL)isHidden {
  return self.state == LynxTimingOverlayFSPStateNone;
}

- (UIColor *)backgroundColor {
  switch (self.state) {
    case LynxTimingOverlayFSPStateNone:
      return [UIColor clearColor];
    case LynxTimingOverlayFSPStateTracking:  // yellow
      return [UIColor colorWithRed:0.5 green:0.5 blue:0.0 alpha:0.15];
    case LynxTimingOverlayFSPStateStable:  // green(15%)
      return [UIColor colorWithRed:0.0 green:0.0 blue:0.5 alpha:0.15];
    case LynxTimingOverlayFSPStateSuccess:  // green(25%)
      return [UIColor colorWithRed:0.0 green:0.5 blue:0.0 alpha:0.25];
    case LynxTimingOverlayFSPStateFailure:  // red
      return [UIColor colorWithRed:0.5 green:0.0 blue:0.0 alpha:0.25];
  }
  return [UIColor clearColor];
}

- (NSInteger)highlightElementSign {
  return self.result.lastUISign;
}

- (NSArray<NSString *> *)informationLines {
  NSMutableArray *lines = [NSMutableArray array];

  NSString *fspDescStr = @"";
  switch (self.state) {
    case LynxTimingOverlayFSPStateNone:
      return @[];
    case LynxTimingOverlayFSPStateTracking:
      fspDescStr = @"Tracking";
      break;
    case LynxTimingOverlayFSPStateStable:
      fspDescStr = [NSString
          stringWithFormat:@"Stable(%.3f ms)",
                           (self.result.lastChangeTimestamp - self.startTimestamp) / 1000.0];
      break;
    case LynxTimingOverlayFSPStateSuccess:
      fspDescStr = [NSString
          stringWithFormat:@"Reached(%.3f ms)",
                           (self.result.lastChangeTimestamp - self.startTimestamp) / 1000.0];
      break;
    case LynxTimingOverlayFSPStateFailure:
      fspDescStr = @"Failed";
      if (self.stopReason == kLynxFSPTracerStopReasonTimeout) {
        fspDescStr = [fspDescStr stringByAppendingString:@" (Timeout)"];
      } else if (self.stopReason == kLynxFSPTracerStopReasonUserInteraction) {
        fspDescStr = [fspDescStr stringByAppendingString:@" (User Action)"];
      }
      break;
  }

  [lines insertObject:[NSString stringWithFormat:@"FSP: %@", fspDescStr] atIndex:0];
  [lines addObject:[NSString stringWithFormat:@" Content: %@x%@", @(self.result.contentSize.width),
                                              @(self.result.contentSize.height)]];
  if (self.result.lastUISign != NSNotFound) {
    [lines addObject:[NSString stringWithFormat:@" Last UI: %@", @(self.result.lastUISign)]];
  }
  return [lines copy];
}

- (NSArray<NSString *> *)snapshotDetailLines {
  if (self.state != LynxTimingOverlayFSPStateStable ||
      self.state != LynxTimingOverlayFSPStateSuccess) {
    return @[];
  }

  auto *tracer = reinterpret_cast<lynx::tasm::timing::FSPTracer *>(self.opaqueTracer);
  if (tracer != nullptr) {
    std::string snapshotDetail = tracer->InspectCurrentSnapshot();
    if (!snapshotDetail.empty()) {
      return @[
        @"FSP Snapshot:", [NSString stringWithCString:snapshotDetail.c_str()
                                             encoding:NSUTF8StringEncoding]
      ];
    }
  }

  return @[];
}

- (void)updateStartTimestamp:(uint64_t)timestamp {
  self.startTimestamp = timestamp;
}

@end

#pragma mark - LynxTimingOverlayHelper (FSP)

@implementation LynxTimingOverlayHelper (FSP)

- (void)setupFSPWithUIOwner:(LynxUIOwner *)uiOwner view:(LynxView *)view {
  if (!uiOwner) {
    return;
  }

  _fspInfo = [[LynxTimingFSPInfo alloc] init];
  _fspInfo.state = LynxTimingOverlayFSPStateNone;

  [uiOwner.fspTracer setObserver:self];

  // Log FSP setup
  [self log:LynxLogLevelInfo message:@"FSP tracking setup for view: %@", view];
}

- (LynxTimingFSPInfo *)fspInfo {
  return _fspInfo;
}

#pragma mark - LynxFSPTracerObserver

- (void)fspTracerDidStart:(LynxFSPTracer *)tracer {
  _fspInfo.state = LynxTimingOverlayFSPStateTracking;
  [self setNeedsUpdate];

  // Log FSP tracking start
  [self log:LynxLogLevelInfo message:@"FSP tracking started"];
}

- (void)fspTracer:(LynxFSPTracer *)tracer
    didBecomeStable:(BOOL)stable
         withResult:(LynxFSPResult)result {
  if (stable) {
    _fspInfo.state = LynxTimingOverlayFSPStateStable;
    _fspInfo.opaqueTracer = [tracer opaqueTracer];

    // Log FSP stable state
    [self log:LynxLogLevelInfo
        message:@"FSP become stable, timestamp: %u ms", result.lastChangeTimestamp];
  } else {
    _fspInfo.state = LynxTimingOverlayFSPStateTracking;
  }

  if (result.lastUISign != NSNotFound) {
    [self log:LynxLogLevelInfo message:@"Last ui element before stable: %ld", result.lastUISign];
  }

  [self log:LynxLogLevelVerbose
      message:@"FSP snapshot:\n  %@",
              [[_fspInfo snapshotDetailLines] componentsJoinedByString:@"\n  "]];

  _fspInfo.result = result;
  [self setNeedsUpdate];
}

- (void)fspTracer:(LynxFSPTracer *)tracer
    willStopForReason:(LynxFSPTracerStopReason)reason
           withResult:(LynxFSPResult)result {
  if (result.success) {
    _fspInfo.state = LynxTimingOverlayFSPStateSuccess;

    // Log FSP success
    [self log:LynxLogLevelInfo
        message:@"FSP reached, timestamp: %uul ms", result.lastChangeTimestamp];

    if (result.lastUISign != NSNotFound) {
      [self log:LynxLogLevelInfo message:@"Last ui element before stop: %ld", result.lastUISign];
    }
  } else {
    _fspInfo.state = LynxTimingOverlayFSPStateFailure;

    // Log FSP failure
    if (reason == kLynxFSPTracerStopReasonTimeout) {
      [self log:LynxLogLevelError message:@"FSP timeout"];
    } else if (reason == kLynxFSPTracerStopReasonUserInteraction) {
      [self log:LynxLogLevelError message:@"FSP failed due to user action"];
    }
  }

  _fspInfo.stopReason = reason;
  _fspInfo.opaqueTracer = [tracer opaqueTracer];
  _fspInfo.result = result;

  [self setNeedsUpdate];
}

- (void)fspTracer:(LynxFSPTracer *)tracer willStartGracefulTimeout:(double)timeout {
  // Log FSP graceful timeout
  [self log:LynxLogLevelInfo
      message:@"FSP will start graceful timeout: %.2f ms", timeout * MSEC_PER_SEC];
}

@end
