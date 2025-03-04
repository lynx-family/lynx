// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEEVENTREPORTERPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEEVENTREPORTERPROTOCOL_H_

#import <Foundation/Foundation.h>
#import "LynxEventReporter.h"
#import "LynxServiceProtocol.h"

@class LynxView;

NS_ASSUME_NONNULL_BEGIN
@protocol LynxServiceEventReporterProtocol <LynxServiceProtocol, LynxEventReportObserverProtocol>

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEEVENTREPORTERPROTOCOL_H_
