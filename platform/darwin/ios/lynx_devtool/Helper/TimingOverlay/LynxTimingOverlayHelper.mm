// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <BaseDevtool/DevToolToast.h>
#import <Lynx/LynxFSPTracer+Internal.h>
#import <Lynx/LynxMetricActualFmpEntry.h>
#import <Lynx/LynxMetricFcpEntry.h>
#import <Lynx/LynxMetricFspEntry.h>
#import <Lynx/LynxUIOwner+Private.h>
#import <Lynx/LynxView.h>
#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>
#import <LynxDevtool/LynxDevToolEnv.h>
#import <LynxDevtool/LynxTimingOverlayHelper+FSP.h>
#import <LynxDevtool/LynxTimingOverlayHelper+Internal.h>
#import <LynxDevtool/LynxTimingOverlayHelper.h>
#import <LynxDevtool/LynxTimingOverlayView.h>
#import <UIKit/UIKit.h>

#include "lynx/core/runtime/common/lynx_console_helper.h"

#pragma mark - LynxTimingOverlayHelper

@interface LynxTimingOverlayHelper () <LynxViewLifecycleV2, LynxTimingOverlayViewDataSource>

@property(nonatomic, strong) NSMutableDictionary<NSNumber*, LynxPerformanceMetric*>* metricInfos;

@end

@implementation LynxTimingOverlayHelper {
  __weak DevToolPlatformDarwinDelegate* _delegate;
  __weak LynxView* _view;
  __weak LynxUIOwner* _uiOwner;

  LynxTimingOverlayView* _overlay;
  BOOL _needsUpdate;
}

static NSPointerArray* g_allTimingOverlays;

+ (void)initialize {
  g_allTimingOverlays = [NSPointerArray pointerArrayWithOptions:NSPointerFunctionsWeakMemory];
}

+ (void)setEnabled:(BOOL)enabled {
  [LynxDevtoolEnv sharedInstance].timingOverlayEnabled = enabled;

  for (LynxTimingOverlayHelper* helper in g_allTimingOverlays) {
    [helper updateEnabled:enabled];
  }
}

- (instancetype)initWithDelegate:(DevToolPlatformDarwinDelegate*)delegate {
  self = [super init];
  if (self) {
    _delegate = delegate;
    _metricInfos = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)dealloc {
  [self updateEnabled:NO];
}

- (void)attachLynxView:(LynxView*)view uiOwner:(LynxUIOwner*)uiOwner {
  if (![LynxDevtoolEnv sharedInstance].timingOverlayEnabled) {
    return;
  }

  if (!view || !uiOwner) {
    return;
  }

  [g_allTimingOverlays addPointer:(__bridge void*)self];

  _needsUpdate = NO;

  _view = view;
  _uiOwner = uiOwner;

  // Setup FSP with the UIOwner
  [self setupFSPWithUIOwner:uiOwner view:view];

  // Create the overlay view
  _overlay = [[LynxTimingOverlayView alloc] initWithFrame:view.bounds dataSource:self];

  __weak typeof(self) weakSelf = self;
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        __strong typeof(self) strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }

        LynxView* view = strongSelf->_view;
        if (!view.window) {
          [strongSelf log:LynxLogLevelError
                  message:@"Failed to attach overlay to LynxView: %@", view];
          return;
        }

        // Start observing performance entries.
        [view addLifecycleClient:self];

        // Add overlay to window.
        [view.window addSubview:strongSelf->_overlay];
        [NSLayoutConstraint activateConstraints:@[
          [strongSelf->_overlay.topAnchor constraintEqualToAnchor:view.topAnchor],
          [strongSelf->_overlay.leadingAnchor constraintEqualToAnchor:view.leadingAnchor],
          [strongSelf->_overlay.trailingAnchor constraintEqualToAnchor:view.trailingAnchor],
          [strongSelf->_overlay.bottomAnchor constraintEqualToAnchor:view.bottomAnchor]
        ]];
      });
}

- (void)updateEnabled:(BOOL)enabled {
  if (!enabled) {
    [_overlay removeFromSuperview];
    _overlay = nil;

    [_view removeLifecycleClient:self];

    for (NSUInteger i = 0; i < g_allTimingOverlays.count; ++i) {
      if ((__bridge id _Nullable)[g_allTimingOverlays pointerAtIndex:i] == self) {
        [g_allTimingOverlays removePointerAtIndex:i];
        break;
      }
    }
  } else if (!_overlay) {
    [self attachLynxView:_view uiOwner:_uiOwner];
  }
}

- (void)setNeedsUpdate {
  if (_overlay) {
    [_overlay setNeedsUpdate];
  }

  __weak typeof(self) weakSelf = self;
  if (self.fspInfo.isEnded) {
    [_overlay
        installTapGestureRecognizer:^{
          [DevToolToast showToast:@"Double tap to dismiss the overlay."];
        }
        twice:^{
          __strong typeof(self) strongSelf = weakSelf;
          if (!strongSelf) {
            return;
          }

          UIView* overlayView = strongSelf->_overlay;
          [UIView animateWithDuration:0.25
              animations:^{
                overlayView.alpha = 0.0;
              }
              completion:^(BOOL finished) {
                [strongSelf updateEnabled:NO];
              }];
        }];
  }
}

- (void)log:(LynxLogLevel)level message:(NSString*)format, ... {
  va_list args;
  va_start(args, format);
  NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
  va_end(args);

  _LynxLog(level, @"<LynxTimingOverlay: %p>: %@", self, message);

  int consoleLevel = lynx::piper::CONSOLE_LOG_INFO;
  switch (level) {
    case LynxLogLevelWarning: {
      consoleLevel = lynx::piper::CONSOLE_LOG_WARNING;
      break;
    }
    case LynxLogLevelError: {
      consoleLevel = lynx::piper::CONSOLE_LOG_ERROR;
      break;
    }
    default: {
      break;
    }
  }

  [_delegate sendConsoleEvent:message
                    withLevel:(int32_t)consoleLevel
                withTimeStamp:[[NSDate date] timeIntervalSince1970] * 1000];
}

#pragma mark - LynxViewLifecycleV2

- (void)onPerformanceEvent:(LynxPerformanceEntry*)entry {
  [self log:LynxLogLevelInfo
      message:@"onPerformanceEvent: %@\n%@", entry.name, [entry toDictionary]];

  if ([entry isKindOfClass:[LynxMetricFcpEntry class]]) {
    _metricInfos[@(0)] = [(LynxMetricFcpEntry*)entry fcp];
    _metricInfos[@(1)] = [(LynxMetricFcpEntry*)entry lynxFcp];
    _metricInfos[@(2)] = [(LynxMetricFcpEntry*)entry totalFcp];

    NSNumber* startTimestamp = [(LynxMetricFcpEntry*)entry totalFcp].startTimestamp;
    [self.fspInfo updateStartTimestamp:startTimestamp.doubleValue * 1000.0];
  } else if ([entry isKindOfClass:[LynxMetricActualFmpEntry class]]) {
    _metricInfos[@(3)] = [(LynxMetricActualFmpEntry*)entry actualFmp];
    _metricInfos[@(4)] = [(LynxMetricActualFmpEntry*)entry lynxActualFmp];
    _metricInfos[@(5)] = [(LynxMetricActualFmpEntry*)entry totalActualFmp];
  } else if ([entry isKindOfClass:[LynxMetricFspEntry class]]) {
    _metricInfos[@(6)] = [(LynxMetricFspEntry*)entry fsp];
    _metricInfos[@(7)] = [(LynxMetricFspEntry*)entry lynxFsp];
    _metricInfos[@(8)] = [(LynxMetricFspEntry*)entry totalFsp];
  }

  [self setNeedsUpdate];
}

#pragma mark - LynxTimingOverlayViewDataSource

- (BOOL)overlayViewIsHidden {
  return self.fspInfo.isHidden;
}

- (UIColor*)overlayViewBackgroundColor {
  return self.fspInfo.backgroundColor;
}

- (CGRect)overlayViewHighlightRect {
  auto sign = [self.fspInfo highlightElementSign];
  if (sign != NSNotFound) {
    LynxUI* ui = [_uiOwner findUIBySign:sign];
    if (!ui) {
      return CGRectNull;
    }

    return [ui getBoundingClientRect];
  }

  return CGRectNull;
}

- (NSArray<NSString*>*)overlayViewDescriptionLines {
  // Get FSP information lines
  NSMutableArray* lines = [[[self fspInfo] informationLines] mutableCopy];
  if (!lines) {
    lines = [NSMutableArray array];
  }

  // Add metric information lines
  auto* metricInfos = self.metricInfos;
  if (metricInfos.count) {
    [lines addObject:@"------"];
    [lines addObject:@"Perf Timing:"];
    auto* ranks = [metricInfos.allKeys sortedArrayUsingSelector:@selector(compare:)];
    for (NSNumber* rank in ranks) {
      auto* metric = metricInfos[rank];
      if (metric) {
        [lines addObject:[NSString stringWithFormat:@" %@ = %.3f ms", metric.name,
                                                    metric.duration.doubleValue]];
      }
    }
  }

  return [lines copy];
}

@end
