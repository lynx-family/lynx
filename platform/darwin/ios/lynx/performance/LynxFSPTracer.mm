// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFSPTracer.h"
#import "LynxFSPTracer+Internal.h"

#import <Lynx/LynxEnv.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxUI+Internal.h>
#import "LynxTimingConstants.h"
#import "LynxUIContext+Internal.h"
#import "LynxUIOwner+Private.h"

#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "base/include/no_destructor.h"
#include "base/include/string/string_utils.h"
#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/services/fsp_tracing/fsp_trace_event_def.h"
#include "core/services/fsp_tracing/fsp_tracer.h"

@implementation LynxFSPTracer {
  LynxFSPConfig _config;
  LynxUIContext* _context;

  void (^_timingCallback)(uint64_t timestamp, NSString* key);

  dispatch_source_t _timer;

  std::unique_ptr<lynx::tasm::timing::FSPTracer> _tracer;

  void (^_completion)(LynxFSPResult);
  dispatch_block_t _hardTimeoutBlock;

  BOOL _hasLoadingUI;
  dispatch_block_t _gracefulTimeoutBlock;
  std::optional<lynx::tasm::timing::FSPResult> _lastResult;

  uint64_t _flow_id;
  uint64_t _snapshot_flow_id;
}

static BOOL g_enabled = false;
static LynxFSPConfig g_config;
static lynx::tasm::timing::FSPConfig GetCXXConfig(const LynxFSPConfig& config, uint64_t flow_id) {
  lynx::tasm::timing::FSPConfig cfg;
  if (config.tracerConfig != 0) {
    cfg = *reinterpret_cast<lynx::tasm::timing::FSPConfig*>(config.tracerConfig);
  } else if (config.mode == LynxFSPModeArea) {
    cfg = lynx::tasm::timing::FSPAreaConfig();
  } else if (config.mode == LynxFSPModeAxial) {
    cfg = lynx::tasm::timing::FSPAxialConfig();
  } else {
    cfg = lynx::tasm::timing::FSPConfig();
  }
  cfg.SetFlowID(flow_id);
  return cfg;
};

+ (void)initialize {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    BOOL enabled = [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableFirstStablePaint
                                                    defaultValue:YES];
    [self setEnabled:enabled config:&kLynxFSPConfigDefault];
  });
}

+ (BOOL)isEnabled {
  return g_enabled;
}

+ (void)setEnabled:(BOOL)enabled config:(LynxFSPConfig const*)config {
  g_enabled = enabled;
  if (config) {
    g_config = *config;
  }
}

- (instancetype)initWithUIContext:(LynxUIContext*)context {
  // Generate a unique flow ID
  auto flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY_FSP, FSP_LYNX_TRACER_CREATE,
              [&](lynx::perfetto::EventContext ctx) { ctx.event()->add_flow_ids(flow_id); });

  self = [super init];
  if (self) {
    _flow_id = flow_id;

    auto config = g_config;
    auto cxx_config = GetCXXConfig(config, flow_id);

    __weak typeof(self) weakSelf = self;
    _tracer = lynx::tasm::timing::FSPTracer::Create(
        cxx_config, [weakSelf](lynx::tasm::timing::FSPSnapshot& snapshot) {
          __strong typeof(weakSelf) strongSelf = weakSelf;
          if (!strongSelf) {
            return;
          }

          [strongSelf updateSnapshot:snapshot];
        });

    if (!_tracer) {
      return nil;
    }

    _config = config;
    _context = context;

    _timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_timer(_timer, DISPATCH_TIME_NOW, _config.snapshotInterval * NSEC_PER_SEC,
                              0);
    dispatch_source_set_event_handler(_timer, ^{
      __strong typeof(weakSelf) strongSelf = weakSelf;
      if (!strongSelf) {
        return;
      }

      [strongSelf captureSnapshot];
    });
  }
  return self;
}

- (void)dealloc {
  [self stopTracing:kLynxFSPTracerStopReasonNone];
}

- (BOOL)isTracing {
  return [_context isFSPTracing];
}

- (void)beginTracing:(void (^)(LynxFSPResult))completion {
  if (!_tracer) {
    return;
  }

  _completion = completion;

  if (NO == [_context setFSPTracing:YES]) {  // FSP tracing was not active.
    TRACE_EVENT(
        LYNX_TRACE_CATEGORY_FSP, FSP_LYNX_TRACER_BEGIN_TRACING,
        [&](lynx::perfetto::EventContext ctx) { ctx.event()->add_flow_ids(self->_flow_id); });

    dispatch_resume(_timer);

    [self beginHardTimeout];

    if (_observer) {
      [_observer fspTracerDidStart:self];
    }
  }
}

- (void)stopTracing:(LynxFSPTracerStopReason)reason {
  [self stopTracing:reason result:{}];
}

- (void)stopTracing:(LynxFSPTracerStopReason)reason result:(lynx::tasm::timing::FSPResult)result {
  if (!_tracer) {
    return;
  }

  // Convert C++ result to ObjC result.
  LynxFSPResult ocResult = [self convertResult:result];

  TRACE_EVENT(
      LYNX_TRACE_CATEGORY_FSP, FSP_LYNX_TRACER_STOP_TRACING, [&](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_terminating_flow_ids(self->_flow_id);
        ctx.event()->add_debug_annotations(FSP_DEBUG_STABLE, (ocResult.success ? "yes" : "no"));
        if (reason == kLynxFSPTracerStopReasonTimeout) {
          ctx.event()->add_debug_annotations(FSP_DEBUG_REASON, "timeout");
        } else if (reason == kLynxFSPTracerStopReasonUserInteraction) {
          ctx.event()->add_debug_annotations(FSP_DEBUG_REASON, "user_interaction");
        }
        ctx.event()->add_debug_annotations(FSP_DEBUG_TIMESTAMP,
                                           std::to_string(ocResult.lastChangeTimestamp));
      });

  if (_hardTimeoutBlock) {
    dispatch_block_cancel(_hardTimeoutBlock);
    _hardTimeoutBlock = nil;
  }

  _hasLoadingUI = NO;
  if (_gracefulTimeoutBlock) {
    dispatch_block_cancel(_gracefulTimeoutBlock);
    _gracefulTimeoutBlock = nil;
  }

  if (YES == [_context setFSPTracing:NO]) {
    if (_observer) {
      [_observer fspTracer:self willStopForReason:reason withResult:ocResult];
    }

    // FSP tracing was active.
    dispatch_source_cancel(_timer);
    _timer = nil;

    if (ocResult.success && _timingCallback) {
      _timingCallback(ocResult.lastChangeTimestamp, kTimingFSPEnd);
    }
  } else if (_timer) {
    // dispatch_timer must be resumed before releasing to avoid the following
    // error:
    // BUG IN CLIENT OF LIBDISPATCH: Release of an inactive object
    dispatch_resume(_timer);
    dispatch_source_cancel(_timer);
    _timer = nil;
  }

  _tracer = nullptr;
  _lastResult = std::nullopt;

  auto comp = _completion;
  if (comp) {
    _completion = nil;
    comp(ocResult);
  }
}

- (void)captureSnapshot {
  if (![_context isFSPTracing]) {
    return;
  }

  if (!_tracer) {
    return;
  }

  lynx::tasm::timing::FSPResult result;

  BOOL hasChange = [_context checkPendingMeaningfulContentChanges];
  if (!hasChange) {
    if (_lastResult.has_value()) {
      // No change since last snapshot, copy last result and change it to stable
      // without the need of another round of capturing and diffing.
      result = (*_lastResult);
      result.is_stable = YES;

      _snapshot_flow_id = TRACE_FLOW_ID();
      TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_FSP, FSP_LYNX_TRACER_CAPTURE_SNAPSHOT,
                        [&](lynx::perfetto::EventContext ctx) {
                          ctx.event()->add_flow_ids(self->_flow_id);
                          ctx.event()->add_flow_ids(self->_snapshot_flow_id);
                          ctx.event()->add_debug_annotations(FSP_DEBUG_STABLE, "unchanged");
                        });
    } else {
      // Skip this snapshot, stablility detection requires at least one
      // meaningful change.
      return;
    }
  } else {
    // Captures changes inside the UI tree.
    CGSize size = [_context.rootUI getBoundingClientRect].size;
    auto int_width = static_cast<int>(size.width);
    auto int_height = static_cast<int>(size.height);
    auto int_size = lynx::base::geometry::IntSize(int_width, int_height);

    _snapshot_flow_id = TRACE_FLOW_ID();
    TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_FSP, FSP_LYNX_TRACER_CAPTURE_SNAPSHOT,
                      [&](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_flow_ids(self->_flow_id);
                        ctx.event()->add_flow_ids(self->_snapshot_flow_id);
                        ctx.event()->add_debug_annotations(
                            FSP_DEBUG_SIZE,
                            lynx::base::FormatString("%dx%d", int_width, int_height));
                        ctx.event()->add_debug_annotations(
                            FSP_DEBUG_UI_SIGN, std::to_string(self->_context.rootUI.sign));
                        if (self->_lastResult.has_value() && (*self->_lastResult).is_stable) {
                          ctx.event()->add_debug_annotations(FSP_DEBUG_STABLE, "pending");
                        }
                      });

    result = _tracer->CaptureSnapshot(int_size, _snapshot_flow_id);
  }

  BOOL wasStable = _lastResult.has_value() && (*_lastResult).is_stable;

  // FSP was stable, reverting back to unstable and canceling graceful timeout block.
  if (!result.is_stable) {
    _lastResult = result;

    if (wasStable) {
      if (_gracefulTimeoutBlock) {
        dispatch_block_cancel(_gracefulTimeoutBlock);
      }

      if (_observer) {
        [_observer fspTracer:self didBecomeStable:NO withResult:[self convertResult:result]];
      }
    }

    TRACE_EVENT_END(LYNX_TRACE_CATEGORY_FSP, [&](lynx::perfetto::EventContext ctx) {
      ctx.event()->add_debug_annotations(FSP_DEBUG_STABLE, (wasStable ? "reverted" : "no"));
    });
    return;
  }

  // FSP turned stable, notify observer and begin timeout if needed.
  if (!wasStable) {
    if (_observer) {
      [_observer fspTracer:self didBecomeStable:YES withResult:[self convertResult:result]];
    }

    double timeout = _config.gracefulTimeout;
    // If extended graceful timeout is set, use it.
    if (_config.extendedGracefulTimeout > 0.0) {
      BOOL isLoading = [_context checkPendingMeaningfulContentLoading] || _hasLoadingUI;
      if (isLoading) {
        // If there are still loading UIs, use the extended graceful timeout.
        timeout = _config.extendedGracefulTimeout;
      }
    }

    if (timeout > 0.0) {
      // Wait for graceful timeout to complete with the pending result.
      _lastResult = result;
      [self beginGracefulTimeout:timeout];

      TRACE_EVENT_END(LYNX_TRACE_CATEGORY_FSP, [&](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations(FSP_DEBUG_STABLE, "pending");
      });
    } else {
      // No graceful timeout, complete immediately.
      [self stopTracing:kLynxFSPTracerStopReasonSuccess result:result];

      TRACE_EVENT_END(LYNX_TRACE_CATEGORY_FSP, [&](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations(FSP_DEBUG_STABLE, "yes");
      });
    }
    return;
  }

  // Repeated stability, wait for previous timeout.
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_FSP);
}

- (LynxFSPResult)convertResult:(lynx::tasm::timing::FSPResult)result {
  LynxFSPResult r = (LynxFSPResult){
      .success = result.is_stable,
      .lastUISign = result.last_ui_sign,
  };

  // Convert timestamp from CACurrentMediaTime to lynx::base::CurrentSystemTimeMicroseconds().
  auto timingNow = lynx::base::CurrentSystemTimeMicroseconds();
  auto timingDiff =
      static_cast<uint64_t>((result.last_change_timestamp - CACurrentMediaTime()) * USEC_PER_SEC);
  r.lastChangeTimestamp = timingNow + timingDiff;
  return r;
}

- (void)beginGracefulTimeout:(double)timeout {
  if (_observer) {
    [_observer fspTracer:self willStartGracefulTimeout:timeout];
  }

  // Cancel any existing graceful timeout block.
  if (_gracefulTimeoutBlock) {
    dispatch_block_cancel(_gracefulTimeoutBlock);
  }

  // Create a new cancellable block for graceful timeout.
  __weak typeof(self) weakSelf = self;
  _gracefulTimeoutBlock = dispatch_block_create(DISPATCH_BLOCK_INHERIT_QOS_CLASS, ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }

    [strongSelf onGracefulTimeout:timeout];
  });

  // Schedule the graceful timeout block to run after the timeout.
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(timeout * NSEC_PER_SEC)),
                 dispatch_get_main_queue(), _gracefulTimeoutBlock);
}

- (void)onGracefulTimeout:(double)timeout {
  double extension = _config.extendedGracefulTimeout - timeout;
  if (extension > 0 && [_context checkPendingMeaningfulContentLoading]) {
    // Check again for loading UIs. If there are still loading UIs since last
    // snapshot, extend the timeout.
    [self beginGracefulTimeout:extension];
    return;
  }

  if (_lastResult.has_value() && (*_lastResult).is_stable) {
    [self stopTracing:kLynxFSPTracerStopReasonSuccess result:(*_lastResult)];
  }
}

- (void)beginHardTimeout {
  if (_hardTimeoutBlock) {
    dispatch_block_cancel(_hardTimeoutBlock);
    _hardTimeoutBlock = nil;
  }

  if (_config.hardTimeout > 0.0) {
    __weak typeof(self) weakSelf = self;
    _hardTimeoutBlock = dispatch_block_create(DISPATCH_BLOCK_INHERIT_QOS_CLASS, ^{
      typeof(self) strongSelf = weakSelf;
      [strongSelf stopTracing:kLynxFSPTracerStopReasonTimeout];
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(_config.hardTimeout * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), _hardTimeoutBlock);
  }
}

- (void)updateSnapshot:(lynx::tasm::timing::FSPSnapshot&)snapshot {
  if (!_tracer) {
    return;
  }

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_FSP, FSP_LYNX_TRACER_UPDATE_SNAPSHOT,
                    [&](lynx::perfetto::EventContext ctx) {
                      ctx.event()->add_flow_ids(self->_flow_id);
                      ctx.event()->add_flow_ids(self->_snapshot_flow_id);
                    });

  __unused __block int totalCount = 0;
  __unused __block int loadedCount = 0;
  __unused __block int loadingCount = 0;
  [_context.uiOwner enumerateUIsUsingBlock:^(LynxUI* _Nonnull ui, BOOL* _Nonnull stop) {
    totalCount++;
    if (ui.meaningfulContentStatus == kLynxUIMeaningfulContentLoaded) {
      loadedCount++;
      CGRect rect;
      if (!CGRectIsEmpty(ui.meaningfulContentRect)) {
        rect = ui.meaningfulContentRect;
      } else {
        rect = [ui getBoundingClientRect];
      }

      auto int_rect = lynx::base::geometry::IntRect();
      int_rect.SetX(static_cast<int>(CGRectGetMinX(rect)));
      int_rect.SetY(static_cast<int>(CGRectGetMinY(rect)));
      int_rect.SetWidth(static_cast<int>(CGRectGetWidth(rect)));
      int_rect.SetHeight(static_cast<int>(CGRectGetHeight(rect)));

      NSTimeInterval content_ts = ui.meaningfulContentChangeTimestamp;
      self->_tracer->FillSnapshot(snapshot,
                                  {
                                      .rect = int_rect,
                                      .ui_sign = ui.sign,
                                      .change_timestamp = content_ts,
                                  },
                                  self->_snapshot_flow_id);
    } else if (ui.meaningfulContentStatus == kLynxUIMeaningfulContentLoading) {
      loadingCount++;
    }
  }];

  _hasLoadingUI = loadingCount > 0;

  TRACE_EVENT_END(
      LYNX_TRACE_CATEGORY_FSP, [total_cnt = totalCount, loaded_cnt = loadedCount,
                                loading_cnt = loadingCount](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations(FSP_DEBUG_TOTAL_COUNT, std::to_string(total_cnt));
        ctx.event()->add_debug_annotations(FSP_DEBUG_LOADED_COUNT, std::to_string(loaded_cnt));
        ctx.event()->add_debug_annotations(FSP_DEBUG_LOADING_COUNT, std::to_string(loading_cnt));
      });
}

- (void)setObserver:(id<LynxFSPTracerObserver>)observer {
  _observer = observer;
}

- (void)setTimingCallback:(void (^)(uint64_t, NSString* _Nonnull))timingCallback {
  _timingCallback = [timingCallback copy];
}

- (uintptr_t)opaqueTracer {
  return reinterpret_cast<uintptr_t>(_tracer.get());
}

@end
