// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderActionManager.h>
#import <LynxDevtool/LynxRecorderPageManager.h>
#import <LynxDevtool/LynxRecorderUIHelper.h>
#import <LynxDevtool/LynxRecorderURLAnalyzer.h>
#import <LynxDevtool/LynxRecorderViewController.h>

@interface Counter : NSObject
@property(atomic, assign) NSInteger count;
- (NSInteger)increaseAndGet;
@end

@implementation Counter
- (instancetype)init {
  self = [super init];
  if (self) {
    _count = 0;
  }
  return self;
}

- (NSInteger)increaseAndGet {
  @synchronized(self) {
    return self.count++;
  }
}

@end

@interface LynxRecorderPageManager ()
@property NSDictionary *pages;
@property NSMutableArray *pageStack;
@property NSDictionary *routers;
@property NSMutableDictionary *groups;
@property NSMutableDictionary *viewControllers;
@property BOOL isMultiEnv;
@property Counter *count;
@property(nonatomic, strong) NSMutableArray<id<LynxRecorderActionCallback>> *actionCallbacks;

- (void)clear;
- (void)loadDescribeFile:(NSString *)url;
- (BOOL)commitRouteForLabel:(NSString *)label popLast:(BOOL *)popLast;
@end

@implementation LynxRecorderPageManager

+ (instancetype)sharedInstance {
  static LynxRecorderPageManager *sharedInstance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });
  return sharedInstance;
}

- (instancetype)init {
  if (self = [super init]) {
    [self clear];
    self.actionCallbacks = [NSMutableArray array];
  }
  return self;
}

- (void)clear {
  _pages = Nil;
  _routers = Nil;
  _groups = [[NSMutableDictionary alloc] init];
  _pageStack = [[NSMutableArray alloc] init];
  _isMultiEnv = NO;
  _viewControllers = [[NSMutableDictionary alloc] init];
}

- (NSString *)buildPageName:(NSString *)rawPageName {
  return [NSString stringWithFormat:@"%@#%ld", rawPageName, [_count increaseAndGet]];
}

- (NSString *)getRawName:(NSString *)pageName {
  return [pageName componentsSeparatedByString:@"#"][0];
}

- (void)registerLynxRecorderActionCallback:(id<LynxRecorderActionCallback>)callback {
  [self.actionCallbacks addObject:callback];
}

// called by LynxRecorderOpenUrlModule
- (void)replayPageFromOpenSchema:(NSDictionary *)params {
  if (!_isMultiEnv) {
    return;
  }

  BOOL popLast = NO;
  if (![self commitRouteForLabel:[params objectForKey:@"label"] popLast:&popLast]) {
    return;
  }
  [self replayCurrPage:popLast];
}

- (BOOL)commitRouteForLabel:(NSString *)label popLast:(BOOL *)popLast {
  NSString *currPageName = [[self pageStack] lastObject];
  if (currPageName.length == 0) {
    return NO;
  }
  NSString *currRawPageName = [self getRawName:currPageName];
  NSDictionary *pageRouters = [[self routers] objectForKey:currRawPageName];
  NSDictionary *nextPageInfo = [pageRouters objectForKey:label];
  NSString *nextRawPageName = [nextPageInfo objectForKey:@"next"];
  if (label.length == 0 || nextRawPageName.length == 0 ||
      [self getPageInfo:nextRawPageName] == nil) {
    return NO;
  }

  BOOL shouldPopLast = [[nextPageInfo objectForKey:@"popLast"] boolValue];
  if (shouldPopLast) {
    [[[self viewControllers] objectForKey:currPageName] setHasBeenPop:YES];
    [[self pageStack] removeLastObject];
  }
  [[self pageStack] addObject:[self buildPageName:nextRawPageName]];
  if (popLast != NULL) {
    *popLast = shouldPopLast;
  }
  return YES;
}

- (void)removeCurrTestBenchVC:(NSString *)pageName hasBeenPop:(BOOL)hasBeenPop {
  if (!_isMultiEnv) {
    return;
  }
  if (!hasBeenPop) {
    [[self pageStack] removeLastObject];
  }
  [[self viewControllers] removeObjectForKey:pageName];
}

- (LynxRecorderViewController *)buildVCByUrl:(NSString *)url {
  NSURL *source = [NSURL URLWithString:url];
  NSArray *query = [[source query] componentsSeparatedByString:@"&"];

  LynxRecorderViewController *tbVC = [LynxRecorderViewController new];
  for (id<LynxRecorderActionCallback> callback in self.actionCallbacks) {
    [tbVC registerLynxRecorderActionCallback:callback];
  }
  if ([query containsObject:@"fullScreen=true"]) {
    tbVC.fullScreen = YES;
  } else {
    tbVC.fullScreen = NO;
  }
  tbVC.url = url;
  return tbVC;
}

- (NSDictionary *)getPageInfo:(NSString *)pageLabel {
  if (![self pages]) {
    return Nil;
  }

  return [[self pages] objectForKey:pageLabel];
}

- (LynxGroup *)getLynxGroup:(NSString *)groupName {
  if (![self groups]) {
    self.groups = [[NSMutableDictionary alloc] init];
    return nil;
  }

  return [[[self groups] objectForKey:groupName] getLynxGroup];
}

- (void)replayCurrPage:(BOOL)popLast {
  NSString *currPageName = [[self pageStack] lastObject];
  NSString *currRawPageName = [self getRawName:currPageName];
  NSDictionary *pageInfo = [self getPageInfo:currRawPageName];
  if (!pageInfo) {
    return;
  }
  LynxRecorderViewController *tbVC = [self buildVCByUrl:[pageInfo objectForKey:@"url"]];
  tbVC.pageName = currPageName;
  LynxGroup *group = [self getLynxGroup:[pageInfo objectForKey:@"group"]];
  if (group) {
    [tbVC setLynxGroup:group];
  } else {
    [[self groups] setObject:tbVC forKey:[pageInfo objectForKey:@"group"]];
  }
  [[self viewControllers] setObject:tbVC forKey:currPageName];
  [self pushTestBenchVC:tbVC popLastVC:popLast];
}

- (void)pushTestBenchVC:(LynxRecorderViewController *)tbVC popLastVC:(BOOL)popLastVC {
  dispatch_async(dispatch_get_main_queue(), ^{
    UINavigationController *vc = [LynxRecorderUIHelper getTopNavigationController];
    if (popLastVC) {
      [vc popViewControllerAnimated:NO];
    }
    [vc pushViewController:tbVC animated:YES];
  });
}

- (void)replayMultiPages:(NSString *)url {
  [self clear];
  [self setIsMultiEnv:YES];
  [self loadDescribeFile:url];
}

- (void)replaySignalPage:(NSString *)url {
  /**
   1. if you replay this page by scan QR code , the sourceURl look like
   file://testbench?url=http:XXXX.json
   2. else LynxPlayground Demo, looks like file://testbench?url=http:XXX.json&pop=false
   the additional "pop=false" means don't pop old vc
   */
  [self clear];
  [self setIsMultiEnv:NO];
  LynxRecorderViewController *tbVC = [self buildVCByUrl:url];
  NSURL *source = [NSURL URLWithString:url];
  NSArray *query = [[source query] componentsSeparatedByString:@"&"];
  BOOL pop = YES;
  if ([query containsObject:@"pop=false"]) {
    pop = NO;
  }
  [self pushTestBenchVC:tbVC popLastVC:pop];
}

- (void)startReplay:(NSString *)url {
  NSURL *baseURL = [NSURL URLWithString:url];
  BOOL isDescribeFile = [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                                   forKey:@"describe_file"
                                                             defaultValue:NO];
  if (isDescribeFile) {
    [self replayMultiPages:url];
  } else {
    [self replaySignalPage:url];
  }
}

- (void)loadDescribeFile:(NSString *)url {
  NSURL *baseURL = [NSURL URLWithString:url];

  NSURLSessionConfiguration *configuration =
      [NSURLSessionConfiguration defaultSessionConfiguration];
  configuration.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
  NSURLSession *session = [NSURLSession sessionWithConfiguration:configuration];
  __weak typeof(self) _self = self;
  NSURLSessionDataTask *dataTask = [session
        dataTaskWithURL:[NSURL
                            URLWithString:[LynxRecorderURLAnalyzer getQueryStringParameter:baseURL
                                                                                    forKey:@"url"]]
      completionHandler:^(NSData *_Nullable data, NSURLResponse *_Nullable response,
                          NSError *_Nullable error) {
        __strong typeof(_self) strongSelf = _self;
        NSDictionary *jsonData = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
        strongSelf.routers = [jsonData objectForKey:@"routers"];
        strongSelf.pages = [jsonData objectForKey:@"pages"];
        [strongSelf.pageStack addObject:[self buildPageName:[jsonData objectForKey:@"root"]]];
        [strongSelf replayCurrPage:NO];
      }];
  [dataTask resume];
}

@end
