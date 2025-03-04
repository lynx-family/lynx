// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchViewClient.h"
#import <Foundation/Foundation.h>
#import <objc/runtime.h>

static char sTestBenchDownloadTaskKey;

@interface TestBenchDownloadDelegate : NSObject <NSURLSessionDataDelegate>
@end

@implementation TestBenchDownloadDelegate

+ (id<LynxResourceLoadDelegate>)delegateFromTask:(NSURLSessionDataTask*)dataTask {
  return objc_getAssociatedObject(dataTask, &sTestBenchDownloadTaskKey);
}

+ (void)setDelegate:(id<LynxResourceLoadDelegate>)delegate forTask:(NSURLSessionDataTask*)dataTask {
  objc_setAssociatedObject(dataTask, &sTestBenchDownloadTaskKey, delegate,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (void)URLSession:(NSURLSession*)session
              dataTask:(NSURLSessionDataTask*)dataTask
    didReceiveResponse:(NSURLResponse*)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
  id<LynxResourceLoadDelegate> delegate = [[self class] delegateFromTask:dataTask];
  NSHTTPURLResponse* res = (NSHTTPURLResponse*)response;
  if (res.statusCode != 200) {
    [delegate onError:@"response not 200"];
    completionHandler(NSURLSessionResponseCancel);
  } else {
    [delegate onStart:res.expectedContentLength];
    completionHandler(NSURLSessionResponseAllow);
  }
}

- (void)URLSession:(NSURLSession*)session
          dataTask:(NSURLSessionDataTask*)dataTask
    didReceiveData:(NSData*)data {
  id<LynxResourceLoadDelegate> delegate = [[self class] delegateFromTask:dataTask];
  [delegate onData:data];
}

- (void)URLSession:(NSURLSession*)session
             dataTask:(NSURLSessionDataTask*)dataTask
    willCacheResponse:(NSCachedURLResponse*)proposedResponse
    completionHandler:(void (^)(NSCachedURLResponse* cachedResponse))completionHandler {
  completionHandler(proposedResponse);
}

- (void)URLSession:(NSURLSession*)session
                    task:(NSURLSessionDataTask*)dataTask
    didCompleteWithError:(NSError*)error {
  id<LynxResourceLoadDelegate> delegate = [[self class] delegateFromTask:dataTask];
  if (error != nil) {
    [delegate onError:[error localizedDescription]];
  } else {
    [delegate onEnd];
  }
  [[self class] setDelegate:nil forTask:dataTask];
}
@end

@interface TestBenchViewClient ()
@property(nonatomic) NSString* baseUrl;
@property(nonatomic, weak) LynxView* lynxView;
@property(nonatomic, readwrite) NSDictionary* darkColor;
@property(nonatomic, readwrite) NSDictionary* lightColor;
@property(nonatomic) NSURLSession* urlSession;
@property(nonatomic, assign) NSUInteger lynxViewLifecycleIndex;
@end

@implementation TestBenchViewClient

- (void)lynxViewDidFirstScreen:(LynxView*)view {
  NSLog(@"lynx_client, %@", @"lynxViewDidFirstScreen");
  [_manager reloadAction];
}

- (instancetype)init {
  if (self = [super init]) {
    self.darkColor = [NSDictionary
        dictionaryWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"Resource/ttColorDark"
                                                                     ofType:@"plist"]];
    self.lightColor = [NSDictionary
        dictionaryWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"Resource/ttColorLight"
                                                                     ofType:@"plist"]];
    self.lynxViewLifecycleIndex = 0;
  }
  return self;
}

- (NSData*)loadDataFromAssets:(NSString*)urlPath {
  NSRange pointRange = [urlPath rangeOfString:@"." options:NSBackwardsSearch];
  NSString* type = [urlPath substringFromIndex:pointRange.location + 1];
  NSString* name = [urlPath substringToIndex:pointRange.location];
  NSString* path = [[NSBundle mainBundle] pathForResource:[@"Resource" stringByAppendingString:name]
                                                   ofType:type];
  return [NSData dataWithContentsOfFile:path];
}

- (dispatch_block_t)loadResourceWithURL:(NSURL*)url
                                   type:(LynxFetchResType)type
                             completion:(LynxResourceLoadCompletionBlock)completionBlock {
  if (url == nil) {
    return nil;
  }

  if ([url.scheme isEqualToString:@"assets"]) {
    NSData* data = [self loadDataFromAssets:url.path];
    completionBlock(YES, data, nil, url);
    return nil;
  }

  NSURLSessionDataTask* task = [NSURLSession.sharedSession
        dataTaskWithURL:url
      completionHandler:^(NSData* received, NSURLResponse* response, NSError* error) {
        if (completionBlock) {
          NSURL* responseUrl = [response URL];
          completionBlock(NO, received, error, responseUrl);
        }
      }];
  [task resume];
  return ^() {
    [task cancel];
  };
}

- (NSString*)translatedResourceWithId:(NSString*)resId
                                theme:(LynxTheme*)theme
                             themeKey:(NSString*)key
                                 view:(__weak LynxView*)view {
  if (theme != nil && [[theme valueForKey:@"brightness"] isEqualToString:@"night"]) {
    return self.darkColor[resId];
  } else {
    return self.lightColor[resId];
  }
}

- (dispatch_block_t)loadResourceWithURL:(NSURL*)url
                               delegate:(nonnull id<LynxResourceLoadDelegate>)delegate {
  if (!_urlSession) {
    _urlSession = [NSURLSession
        sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]
                        delegate:[[TestBenchDownloadDelegate alloc] init]
                   delegateQueue:nil];
  }

  NSURLSessionDataTask* task = [_urlSession dataTaskWithRequest:[NSURLRequest requestWithURL:url]];
  [TestBenchDownloadDelegate setDelegate:delegate forTask:task];
  [task resume];

  return ^() {
    [TestBenchDownloadDelegate setDelegate:nil forTask:task];
    [task cancel];
  };
}

@end
