// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxResourceModule.h"

#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceImageProtocol.h>
#import <Lynx/LynxServiceResourceProtocol.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxTraceEvent.h>
#import <Lynx/LynxTraceEventWrapper.h>
#import <Lynx/LynxView+Internal.h>
#import "LynxContext+Internal.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTraceEventDef.h"

static NSString* kDataKey = @"data";
static NSString* kUriKey = @"uri";
static NSString* kTypeKey = @"type";
static NSString* kParamsKey = @"params";
static NSString* kCodeKey = @"code";
static NSString* kMsgKey = @"msg";
static NSString* kDetailKey = @"details";

static NSString* kImageType = @"image";
static NSString* kAudioType = @"audio";
static NSString* kVideoType = @"video";

static NSInteger kDefaultMediaSize = 500 * 1024;

@implementation LynxResourceModule {
  __weak LynxContext* context_;
}

+ (NSString*)name {
  return @"LynxResourceModule";
}

+ (NSDictionary<NSString*, NSString*>*)methodLookup {
  return @{
    @"requestResourcePrefetch" : NSStringFromSelector(@selector(requestResourcePrefetch:callback:)),
    @"cancelResourcePrefetch" : NSStringFromSelector(@selector(cancelResourcePrefetch:callback:)),
    @"requestResourcePrefetchImage" :
        NSStringFromSelector(@selector(requestResourcePrefetchImage:callback:)),
  };
}

- (instancetype)initWithLynxContext:(LynxContext*)context {
  self = [super init];
  if (self) {
    context_ = context;
  }
  return self;
}

- (std::pair<NSInteger, NSString*>)resourcePrefetch:(NSDictionary*)prefetchData
                                           isCancel:(BOOL)isCancel
                                         allResults:(NSMutableDictionary*)allResults {
  NSInteger globalCode = ECLynxSuccess;
  NSString* globalMsg = @"";

  id data = [prefetchData objectForKey:kDataKey];

  if (data == nil || ![data isKindOfClass:[NSArray class]]) {
    globalCode = ECLynxResourceModuleParamsError;
    globalMsg =
        @"Parameters error in Lynx resource prefetch module! Value of 'data' should be an array.";
    LynxError* error = [LynxError
        lynxErrorWithCode:globalCode
                  message:globalMsg
            fixSuggestion:@"Please check the parameters passed to Lynx resource prefetch module."
                    level:LynxErrorLevelError];
    [error addCustomInfo:(isCancel ? @"cancel" : @"request") forKey:@"actionType"];
    [context_ reportLynxError:error];
  } else {
    NSArray* array = data;
    NSMutableArray* resultArray = [[NSMutableArray alloc] init];
    for (id obj in array) {
      NSInteger code = ECLynxSuccess;
      NSString* msg = @"";
      NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
      id uri = @"";
      if (![obj isKindOfClass:[NSDictionary class]]) {
        code = ECLynxResourceModuleParamsError;
        msg = @"Parameters error in Lynx resource prefetch module! The prefetch data should be a "
              @"map.";
      } else {
        uri = [obj objectForKey:kUriKey];
        id type = [obj objectForKey:kTypeKey];
        id params = [obj objectForKey:kParamsKey];
        if (uri == nil || type == nil) {
          code = ECLynxResourceModuleParamsError;
          msg = @"Parameters error in Lynx resource prefetch module! 'resource uri' or 'type' is "
                @"null.";
        } else {
          auto res = isCancel ? [self cancelResourcePrefetchInternal:uri type:type params:params]
                              : [self requestResourcePrefetchInternal:uri type:type params:params];
          code = res.first;
          msg = res.second;
          result[kUriKey] = uri;
          result[kTypeKey] = type;
        }
      }
      if (code != ECLynxSuccess) {
        LynxError* error = [LynxError
            lynxErrorWithCode:code
                      message:msg
                fixSuggestion:@"If it is a parameter error, please check the parameters passed in. "
                              @"If the Resource service does not exist, it may be due to an error "
                              @"that occurred while creating the resource service through "
                              @"reflection. Please contact the client RD for help."
                        level:LynxErrorLevelError];
        [error addCustomInfo:(isCancel ? @"cancel" : @"request") forKey:@"actionType"];
        [error addCustomInfo:uri forKey:@"resourceUri"];
        [context_ reportLynxError:error];
      }
      result[kCodeKey] = @(code);
      result[kMsgKey] = msg;
      [resultArray addObject:result];
    }
    allResults[kDetailKey] = resultArray;
  }
  return std::make_pair(globalCode, globalMsg);
}

- (void)cancelResourcePrefetch:(NSDictionary*)prefetchData callback:(LynxCallbackBlock)callback {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, RESOURCE_MODULE_CANCEL_PREFETCH);

  NSMutableDictionary* allResults = [[NSMutableDictionary alloc] init];

  auto res = [self resourcePrefetch:prefetchData isCancel:YES allResults:allResults];
  NSInteger globalCode = res.first;
  NSString* globalMsg = res.second;

  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
  allResults[kCodeKey] = @(globalCode);
  allResults[kMsgKey] = globalMsg;
  if (callback) {
    callback(allResults);
  }
}

- (void)requestResourcePrefetch:(NSDictionary*)prefetchData callback:(LynxCallbackBlock)callback {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, RESOURCE_MODULE_REQUEST_PREFETCH);

  NSMutableDictionary* allResults = [[NSMutableDictionary alloc] init];

  auto res = [self resourcePrefetch:prefetchData isCancel:NO allResults:allResults];
  NSInteger globalCode = res.first;
  NSString* globalMsg = res.second;

  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
  allResults[kCodeKey] = @(globalCode);
  allResults[kMsgKey] = globalMsg;
  if (callback) {
    callback(allResults);
  }
}

- (std::pair<NSInteger, NSString*>)cancelResourcePrefetchInternal:(NSString*)uri
                                                             type:(NSString*)type
                                                           params:(nullable NSDictionary*)params {
  NSInteger code = ECLynxSuccess;
  NSString* msg = @"";
  if ([type isEqualToString:kImageType]) {
    // TODO(wujintian): add image prefetch implementation
    code = ECLynxResourceModuleParamsError;
    msg = @"Image prefetch dose not been supported on iOS yet.";
  } else if ([type isEqualToString:kAudioType] || [type isEqualToString:kVideoType]) {
    NSString* preloadKey = params[@"preloadKey"];
    NSString* videoID = params[@"videoID"];
    BOOL videoModel = [params[@"videoModel"] boolValue];
    id<LynxServiceResourceProtocol> service = LynxService(LynxServiceResourceProtocol);
    if (!preloadKey) {
      code = ECLynxResourceModuleParamsError;
      msg = @"missing preloadKey!";
    } else if (!service) {
      code = ECLynxResourceModuleResourceServiceNotExist;
      msg = @"Resource service do not exist!";
    } else {
      [service cancelPreloadMedia:preloadKey videoID:videoID videoModel:videoModel];
    }
  } else {
    code = ECLynxResourceModuleParamsError;
    msg = [NSString stringWithFormat:@"%@%@", @"Parameters error! Unknown type :", type];
  }
  LOGI("LynxResourceModule requestResourcePrefetch uri: " << uri << " type: " << type);
  return std::make_pair(code, msg);
}

- (std::pair<NSInteger, NSString*>)requestResourcePrefetchInternal:(NSString*)uri
                                                              type:(NSString*)type
                                                            params:(nullable NSDictionary*)params {
  NSInteger code = ECLynxSuccess;
  NSString* msg = @"";
  if ([type isEqualToString:kImageType]) {
    LynxURL* lynxUri = [[LynxURL alloc] init];
    lynxUri.url = [[NSURL alloc] initWithString:uri];
    id<LynxServiceImageProtocol> service = LynxService(LynxServiceImageProtocol);
    if (service) {
      [service prefetchImage:lynxUri params:params];
    } else {
      code = ECLynxResourceModuleImgPrefetchHelperNotExist;
      msg = @"Image prefetch service do not exist!";
    }
  } else if ([type isEqualToString:kAudioType] || [type isEqualToString:kVideoType]) {
    NSString* preloadKey = params[@"preloadKey"];
    NSString* videoID = params[@"videoID"];
    NSString* videoModelStr = params[@"videoModel"];
    NSUInteger resolution = [params[@"resolution"] unsignedIntegerValue];
    NSUInteger encodeType = [params[@"encodeType"] unsignedIntegerValue];
    NSString* apiString = params[@"apiString"];
    NSDictionary* videoModel = nil;
    if (videoModelStr) {
      NSData* data = [videoModelStr dataUsingEncoding:NSUTF8StringEncoding];
      if (data.length) {
        videoModel = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
      }
    }

    NSInteger size = kDefaultMediaSize;
    if (params[@"size"]) {
      size = [params[@"size"] integerValue];
    }

    id<LynxServiceResourceProtocol> service = LynxService(LynxServiceResourceProtocol);

    if (!preloadKey) {
      code = ECLynxResourceModuleParamsError;
      msg = @"missing preloadKey!";
    } else if (!service) {
      code = ECLynxResourceModuleResourceServiceNotExist;
      msg = @"Resource service do not exist!";
    } else {
      [service preloadMedia:uri
                   cacheKey:preloadKey
                    videoID:videoID
                 videoModel:videoModel
                 resolution:resolution
                 encodeType:encodeType
                  apiString:apiString
                       size:size];
    }
  } else {
    code = ECLynxResourceModuleParamsError;
    msg = [NSString stringWithFormat:@"%@%@", @"Parameters error! Unknown type :", type];
  }
  LOGI("LynxResourceModule requestResourcePrefetch uri: " << uri << " type: " << type);
  return std::make_pair(code, msg);
}

- (void)requestResourcePrefetchImage:(NSDictionary*)prefetchData
                            callback:(LynxCallbackBlock)callback {
  id uri = [prefetchData objectForKey:kUriKey];
  id params = [prefetchData objectForKey:kParamsKey];
  if (uri == nil) {
    NSInteger code = ECLynxResourceModuleParamsError;
    NSString* msg = @"Parameters error in Lynx resource prefetch module! 'uri' is null.";
    LynxError* error = [LynxError
        lynxErrorWithCode:code
                  message:msg
            fixSuggestion:@"Please check the parameters passed to Lynx resource prefetch module."
                    level:LynxErrorLevelError];
    [context_ reportLynxError:error];
    if (callback) {
      NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
      result[kCodeKey] = @(code);
      result[kMsgKey] = msg;
      callback(result);
    }
    return;
  }
  id<LynxServiceImageProtocol> service = LynxService(LynxServiceImageProtocol);
  if (service == nil) {
    NSInteger code = ECLynxResourceModuleImgPrefetchHelperNotExist;
    NSString* msg = @"Image prefetch service do not exist!";
    LynxError* error =
        [LynxError lynxErrorWithCode:code
                             message:msg
                       fixSuggestion:@"If the Resource service does not exist, it may be due to an "
                                     @"error that occurred while creating the resource service "
                                     @"through reflection. Please contact the client RD for help."
                               level:LynxErrorLevelError];
    [context_ reportLynxError:error];
    if (callback) {
      NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
      result[kCodeKey] = @(code);
      result[kMsgKey] = msg;
      callback(result);
    }
    return;
  }
  LynxURL* lynxUri = [[LynxURL alloc] init];
  lynxUri.url = [[NSURL alloc] initWithString:uri];
  LynxImageLoadCompletionBlock requestBlock =
      ^(UIImage* _Nullable image, NSError* _Nullable error, NSURL* _Nullable imageURL) {
        if (callback) {
          NSString* errorDetail = [NSString stringWithFormat:@"%@", [error description]];
          NSNumber* errorCode = error ? [NSNumber numberWithInteger:error.code] : @(0);
          NSInteger code = [errorCode integerValue];
          NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
          result[kCodeKey] = @(code);
          result[kMsgKey] = errorDetail ?: @"";
          callback(result);
        }
      };

  [service prefetchImage:lynxUri params:params completed:requestBlock];
}

@end
