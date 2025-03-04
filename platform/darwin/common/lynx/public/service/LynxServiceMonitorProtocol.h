// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEMONITORPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEMONITORPROTOCOL_H_

#import <Foundation/Foundation.h>
#import "LynxView.h"

@protocol LynxServiceProtocol;
NS_ASSUME_NONNULL_BEGIN

typedef enum : NSUInteger {
  LynxContextTagLastLynxURL,
  LynxContextTagLastLynxAsyncComponentURL,
} LynxContextTagType;

@protocol LynxServiceMonitorProtocol <LynxServiceProtocol>

/**
 * report data about loading LynxView
 * @param event event name
 * @param data event data
 */
- (void)reportTrailEvent:(NSString *)event data:(NSDictionary *)data;

/**
 * report data about loading image
 * @param event event name
 * @param data event data
 */
- (void)reportImageStatus:(NSString *)event data:(NSDictionary *)data;

/**
 * report tag in global context when app crash
 * @param type tag type
 * @param data tag data
 */
- (void)reportErrorGlobalContextTag:(LynxContextTagType)type data:(NSString *)data;

/**
 * report data about loading resource
 * @param LynxView LynxView
 * @param data event data
 * @param extra extra data
 */
- (void)reportResourceStatus:(LynxView *__nonnull)LynxView
                        data:(NSDictionary *__nonnull)data
                       extra:(NSDictionary *__nullable)extra;

NS_ASSUME_NONNULL_END
@end

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEMONITORPROTOCOL_H_
