// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#pragma mark - Props of timing events - SSR
FOUNDATION_EXTERN NSString* const kTimingEventPropSSRMetrics;
FOUNDATION_EXTERN NSString* const kTimingEventPropSSRRenderPage;

#pragma mark - Timestamp of timing
FOUNDATION_EXTERN NSString* const kTimingCreateLynxStart;
FOUNDATION_EXTERN NSString* const kTimingCreateLynxEnd;
FOUNDATION_EXTERN NSString* const kTimingPrepareTemplateStart;
FOUNDATION_EXTERN NSString* const kTimingPrepareTemplateEnd;
FOUNDATION_EXTERN NSString* const kTimingContainerInitStart;
FOUNDATION_EXTERN NSString* const kTimingContainerInitEnd;
FOUNDATION_EXTERN NSString* const kTimingOpenTime;
