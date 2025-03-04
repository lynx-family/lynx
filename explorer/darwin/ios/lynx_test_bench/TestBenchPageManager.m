// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchPageManager.h"
#import "TestBenchActionManager.h"
#import "TestBenchUIHelper.h"
#import "TestBenchURLAnalyzer.h"
#import "TestBenchViewController.h"

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

@interface TestBenchPageManager ()
@property NSDictionary *pages;
@property NSMutableArray *pageStack;
@property NSDictionary *routers;
@property NSMutableDictionary *groups;
@property NSMutableDictionary *viewControllers;
@property BOOL isMultiEnv;
@property Counter *count;
@property(nonatomic, strong) NSMutableArray<id<TestBenchActionCallback>> *actionCallbacks;

- (void)clear;
- (void)loadDescribeFile:(NSString *)url;
@end

@implementation TestBenchPageManager

+ (instancetype)sharedInstance {
  static TestBenchPageManager *sharedInstance = nil;
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

- (void)registerTestBenchActionCallback:(id<TestBenchActionCallback>)callback {
  [self.actionCallbacks addObject:callback];
}

// called by TestBenchOpenUrlModule
- (void)replayPageFromOpenSchema:(NSDictionary *)params {
  if (!_isMultiEnv) {
    return;
  }

  NSString *currPageName = [[self pageStack] lastObject];
  NSString *currRawPageName = [self getRawName:currPageName];
  NSString *label = [params objectForKey:@"label"];

  NSDictionary *nextPageInfo = [[[self routers] objectForKey:currRawPageName] objectForKey:label];

  BOOL popLast = [[nextPageInfo objectForKey:@"popLast"] boolValue];
  NSString *nextRawPageName = [nextPageInfo objectForKey:@"next"];
  if (popLast) {
    [[[self viewControllers] objectForKey:currPageName] setHasBeenPop:YES];
    [[self pageStack] removeLastObject];
  }
  [[self pageStack] addObject:[self buildPageName:nextRawPageName]];

  [self replayCurrPage:popLast];
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

- (TestBenchViewController *)buildVCByUrl:(NSString *)url {
  NSURL *source = [NSURL URLWithString:url];
  NSArray *query = [[source query] componentsSeparatedByString:@"&"];

  TestBenchViewController *tbVC = [TestBenchViewController new];
  for (id<TestBenchActionCallback> callback in self.actionCallbacks) {
    [tbVC registerTestBenchActionCallback:callback];
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
  TestBenchViewController *tbVC = [self buildVCByUrl:[pageInfo objectForKey:@"url"]];
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

- (void)pushTestBenchVC:(TestBenchViewController *)tbVC popLastVC:(BOOL)popLastVC {
  dispatch_async(dispatch_get_main_queue(), ^{
    UINavigationController *vc = [TestBenchUIHelper getTopNavigationController];
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
  TestBenchViewController *tbVC = [self buildVCByUrl:url];
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
  BOOL isDescribeFile = [TestBenchURLAnalyzer getQueryBooleanParameter:baseURL
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
        dataTaskWithURL:[NSURL URLWithString:[TestBenchURLAnalyzer getQueryStringParameter:baseURL
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
