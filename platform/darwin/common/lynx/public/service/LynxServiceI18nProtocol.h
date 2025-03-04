// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEI18NPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEI18NPROTOCOL_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

#ifdef __cplusplus
extern "C" {
#endif
typedef struct napi_env__* napi_env;
#ifdef __cplusplus
}
#endif

@protocol LynxServiceProtocol;

@protocol LynxServiceI18nProtocol <LynxServiceProtocol>

/**
 * Currently, Intl.NumberFormat and Intl.DateTimeFormat will be mounted in NapiEnv.
 * the NapiEnv pointer of LepusNG is passed from the templateEntry decode stage, and the NapiEnv of
 * JS is passed from the PrepareJS stage of LynxRuntime.
 *
 * @param napiEnv The NapiEnv pointer passed from the Native layer
 */
- (void)registerNapiEnv:(napi_env)napiEnv;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEI18NPROTOCOL_H_
