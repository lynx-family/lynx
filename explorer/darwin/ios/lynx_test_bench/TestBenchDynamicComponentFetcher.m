// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchDynamicComponentFetcher.h"
#import <Foundation/Foundation.h>

static NSString* ASSETS_SCHEME = @"assets://";
static NSData* loadDataFromAssets(NSString* schema, NSError** error) {
  NSBundle* bundle = [NSBundle mainBundle];
  NSString* path = [bundle pathForResource:[schema stringByDeletingPathExtension]
                                    ofType:[schema pathExtension]
                               inDirectory:@"Resource"];

  NSData* data = [NSData dataWithContentsOfFile:path options:0 error:error];

  if (data == nil || data.length == 0) {
    NSLog(@"TestBenchDynamicComponentFetcher request from file failed, url:%@, error:%@", schema,
          [*error localizedDescription]);
    return nil;
  } else {
    NSLog(@"TestBenchDynamicComponentFetcher request from file succeeded, url:%@, data:%@", schema,
          data);
    return data;
  }
}

@interface TestBenchDynamicComponentFetcher ()
@property NSMutableDictionary* dynamicMap;
@end

@implementation TestBenchDynamicComponentFetcher

- (void)loadDynamicComponent:(nonnull NSString*)schema
             withLoadedBlock:(nonnull onComponentLoaded)block {
  NSDictionary* data = [[self dynamicMap] objectForKey:schema];
  if (data != nil) {
    [self loadDynamicComponentWithCache:data withLoadedBlock:block];
  } else {
    [self loadDynamicComponentWithSchema:schema withLoadedBlock:block];
  }
}

- (void)parse:(NSArray*)actionList {
  _dynamicMap = [[NSMutableDictionary alloc] init];
  for (int index = 0; index < actionList.count; ++index) {
    NSDictionary* obj = actionList[index];
    if ([obj[@"Function Name"] isEqual:@"LoadComponentWithCallback"]) {
      [_dynamicMap setObject:obj[@"Params"] forKey:obj[@"Params"][@"url"]];
    }
  }
}

- (void)request:(LynxResourceRequest*)request onComplete:(LynxResourceLoadBlock)callback {
  NSLog(@"DemoLynxResourceProvider request url:%@", request.url);

  NSString* url = request.url;

  if ([url hasPrefix:ASSETS_SCHEME]) {
    NSError* error;
    NSData* data = loadDataFromAssets([url substringFromIndex:[ASSETS_SCHEME length]], &error);
    if (data == nil) {
      callback([[LynxResourceResponse alloc] initWithError:error
                                                      code:LynxResourceResponseCodeFailed]);
    } else {
      callback([[LynxResourceResponse alloc] initWithData:data]);
    }
    return;
  }

  if (_urlRedirectMap != Nil && [_urlRedirectMap objectForKey:url] != Nil) {
    url = [_urlRedirectMap objectForKey:url];
  }

  NSURL* nsUrl = [NSURL URLWithString:url];
  NSURLRequest* nsRequest = [NSURLRequest requestWithURL:nsUrl
                                             cachePolicy:NSURLRequestReloadIgnoringCacheData
                                         timeoutInterval:5];
  NSError* error = [[NSError alloc] init];
  NSData* data = [NSURLConnection sendSynchronousRequest:nsRequest
                                       returningResponse:nil
                                                   error:&error];
  if (data == nil || data.length == 0) {
    NSLog(@"DemoLynxResourceProvider request failed, url:%@, error:%@", request.url,
          [error localizedDescription]);
    callback([[LynxResourceResponse alloc] initWithError:error
                                                    code:LynxResourceResponseCodeFailed]);
  } else {
    NSLog(@"DemoLynxResourceProvider request successfully, url:%@, data:%@", request.url, data);
    callback([[LynxResourceResponse alloc] initWithData:data]);
  }
}

- (void)cancel:(LynxResourceRequest*)request {
}

- (void)loadDynamicComponentWithCache:(NSDictionary*)cache
                      withLoadedBlock:(onComponentLoaded)block {
  NSData* res =
      [[NSData alloc] initWithBase64EncodedString:cache[@"source"]
                                          options:NSDataBase64DecodingIgnoreUnknownCharacters];
  if ([cache[@"sync_tag"] boolValue]) {
    block(res, nil);
  } else {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(queue, ^{
      block(res, nil);
    });
  }
}

- (void)loadDynamicComponentWithSchema:(nonnull NSString*)schema
                       withLoadedBlock:(nonnull onComponentLoaded)block {
  if ([schema hasPrefix:ASSETS_SCHEME]) {
    NSError* error;
    NSData* data = loadDataFromAssets([schema substringFromIndex:[ASSETS_SCHEME length]], &error);
    block(data, error);
    return;
  }

  NSURL* url = [NSURL URLWithString:schema];
  NSURLRequest* request = [NSURLRequest requestWithURL:url
                                           cachePolicy:NSURLRequestReloadIgnoringCacheData
                                       timeoutInterval:2];
  // use async request dynamic component template to to ensure the stability of test result
  [NSURLConnection
      sendAsynchronousRequest:request
                        queue:[NSOperationQueue mainQueue]
            completionHandler:^(NSURLResponse* _Nullable response, NSData* _Nullable data,
                                NSError* _Nullable connectionError) {
              // response: The response header returned by the server
              // data: The response body returned by the server
              // connectionError: The connection error
              if (!connectionError) {
                block(data, nil);
              } else {
                block(data, connectionError);
              }
            }];
}
@end
