// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxResourceModule.h"

#import "LynxContext+Internal.h"
#import "LynxService.h"
#import "LynxServiceImageProtocol.h"
#import "LynxServiceResourceProtocol.h"
#import "LynxSubErrorCode.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"
#import "LynxView+Internal.h"

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
    [context_.lynxView.templateRender onErrorOccurred:error];
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
        [context_.lynxView.templateRender onErrorOccurred:error];
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
  [LynxTraceEvent beginSection:LYNX_TRACE_CATEGORY_WRAPPER withName:@"cancelResourcePrefetch"];

  NSMutableDictionary* allResults = [[NSMutableDictionary alloc] init];

  auto res = [self resourcePrefetch:prefetchData isCancel:YES allResults:allResults];
  NSInteger globalCode = res.first;
  NSString* globalMsg = res.second;

  [LynxTraceEvent endSection:LYNX_TRACE_CATEGORY_WRAPPER withName:@"cancelResourcePrefetch"];
  allResults[kCodeKey] = @(globalCode);
  allResults[kMsgKey] = globalMsg;
  if (callback) {
    callback(allResults);
  }
}

- (void)requestResourcePrefetch:(NSDictionary*)prefetchData callback:(LynxCallbackBlock)callback {
  [LynxTraceEvent beginSection:LYNX_TRACE_CATEGORY_WRAPPER withName:@"requestResourcePrefetch"];

  NSMutableDictionary* allResults = [[NSMutableDictionary alloc] init];

  auto res = [self resourcePrefetch:prefetchData isCancel:NO allResults:allResults];
  NSInteger globalCode = res.first;
  NSString* globalMsg = res.second;

  [LynxTraceEvent endSection:LYNX_TRACE_CATEGORY_WRAPPER withName:@"requestResourcePrefetch"];
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
    id<LynxServiceResourceProtocol> service =
        [LynxServices getInstanceWithProtocol:@protocol(LynxServiceResourceProtocol)
                                        bizID:DEFAULT_LYNX_SERVICE];
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
    id<LynxServiceImageProtocol> service =
        [LynxServices getInstanceWithProtocol:@protocol(LynxServiceImageProtocol)
                                        bizID:DEFAULT_LYNX_SERVICE];
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

    id<LynxServiceResourceProtocol> service =
        [LynxServices getInstanceWithProtocol:@protocol(LynxServiceResourceProtocol)
                                        bizID:DEFAULT_LYNX_SERVICE];

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

@end
