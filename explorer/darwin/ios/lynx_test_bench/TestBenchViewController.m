// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchViewController.h"
#import <Lynx/LynxUI.h>
#import <Lynx/LynxView.h>
#import "TestBenchActionManager.h"
#import "TestBenchPageManager.h"
#import "TestBenchReplayConfig.h"
#import "TestBenchStateReplayView.h"
#import "TestBenchURLAnalyzer.h"
#import "TestBenchView.h"

@interface TestBenchViewController ()
@property LynxView *lynxView;
@property LynxGroup *group;
@property TestBenchActionManager *manager;
@property TestBenchStateReplayView *stateView;
@property NSArray *pages;
@property NSMutableArray *pageInstances;
@property UIScrollView *scrollContainer;
@property NSMutableArray<id<TestBenchActionCallback>> *actionCallbacks;
@end

@implementation TestBenchViewController

- (id)init {
  return [self initWithGroup:nil];
}

- (id)initWithGroup:(LynxGroup *)group {
  if (self = [super init]) {
    _manager = [[TestBenchActionManager alloc] init];
    _manager.endTestBenchBlock = self.endTestBenchBlock;
    _hasBeenPop = NO;
    _actionCallbacks = [NSMutableArray array];
    [self.manager setLynxGroup:group];
  }
  return self;
}

- (BOOL)isMultiPageFile {
  NSURL *baseURL = [NSURL URLWithString:_url];
  BOOL isMultiPageFile = [TestBenchURLAnalyzer getQueryBooleanParameter:baseURL
                                                                 forKey:@"multi-page"
                                                           defaultValue:NO];
  return isMultiPageFile;
}

- (UIScrollView *)buildScrollContainer {
  UIScrollView *scrollContainer = [[UIScrollView alloc] init];
  scrollContainer.frame = self.view.frame;
  return scrollContainer;
}

- (void)replayMultiPageFile {
  if (_pages == nil) {
    return;
  }
  for (NSDictionary *object in self.pages) {
    NSString *url = [object objectForKey:@"url"];
    NSInteger x = [[[object objectForKey:@"frame"] objectForKey:@"x"] intValue];
    NSInteger y = [[[object objectForKey:@"frame"] objectForKey:@"y"] intValue];
    dispatch_async(dispatch_get_main_queue(), ^{
      TestBenchView *view = [[TestBenchView alloc] init];
      [self->_scrollContainer addSubview:view];
      [self->_pageInstances addObject:view];
      for (id<TestBenchActionCallback> callback in self.actionCallbacks) {
        [view registerTestBenchActionCallback:callback];
      }
      [view loadPageWithPoint:url point:CGPointMake(x, y) scrollContainer:self->_scrollContainer];
    });
  }
}

- (void)loadMultiPageFile {
  NSURL *baseURL = [NSURL URLWithString:_url];
  _pageInstances = [[NSMutableArray alloc] init];

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
        strongSelf.pages = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
        [strongSelf replayMultiPageFile];
      }];
  [dataTask resume];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self initNavigation];
  if ([self isMultiPageFile]) {
    _scrollContainer = [self buildScrollContainer];
    [self.view addSubview:_scrollContainer];
    [self loadMultiPageFile];
  } else {
    [self createArkActionManager];
  }
}

- (void)initNavigation {
  if (_fullScreen) {
    [self.navigationController setNavigationBarHidden:YES animated:NO];
    self.navigationController.interactivePopGestureRecognizer.enabled = YES;
    self.navigationController.interactivePopGestureRecognizer.delegate = self;
  } else {
    [self.navigationController setNavigationBarHidden:NO animated:NO];
    self.navigationController.navigationBar.translucent = NO;
    self.view.backgroundColor = [UIColor whiteColor];
    UILabel *titleLabel = [[UILabel alloc] initWithFrame:self.navigationItem.accessibilityFrame];
    titleLabel.text = @"TestBench";
    titleLabel.textColor = [UIColor blackColor];
    self.navigationItem.titleView = titleLabel;
    self.navigationController.navigationBar.tintColor = [UIColor blackColor];

    UIBarButtonItem *rightItem =
        [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"RefreshIcon"]
                                         style:UIBarButtonItemStylePlain
                                        target:self
                                        action:@selector(reloadTemplate)];
    rightItem.accessibilityHint = @"Reload";
    rightItem.accessibilityValue = @"Reload";
    rightItem.tintColor = [UIColor blackColor];
    self.navigationItem.rightBarButtonItem = rightItem;
  }
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer {
  return YES;
}

- (void)reloadTemplate {
  if ([self isMultiPageFile]) {
    for (TestBenchView *view in self.pageInstances) {
      [view reload];
    }
  } else if (_manager != nil) {
    [_manager reload];
  }
}

- (void)createArkActionManager {
  _stateView = [[TestBenchStateReplayView alloc] init];
  [self.view addSubview:_stateView];
  [_manager startWithUrl:_url
                  inView:self.view
              withOrigin:CGPointMake(0, 0)
               stateView:_stateView
            replayConfig:[[TestBenchReplayConfig alloc] initWithProductUrl:_url]];
}

- (void)viewWillDisappear:(BOOL)animated {
  [super viewWillDisappear:animated];
}

- (void)registerTestBenchActionCallback:(id<TestBenchActionCallback>)callback {
  [self.actionCallbacks addObject:callback];
  [self.manager registerTestBenchActionCallback:callback];
}

- (LynxGroup *)getLynxGroup {
  return self.manager.lynxGroup;
}

- (void)setLynxGroup:(LynxGroup *)group {
  self.manager.lynxGroup = group;
}

- (void)didMoveToParentViewController:(UIViewController *)parent {
  [super didMoveToParentViewController:parent];
  if (!parent) {
    [[TestBenchPageManager sharedInstance] removeCurrTestBenchVC:_pageName hasBeenPop:_hasBeenPop];
  }
}

@end
