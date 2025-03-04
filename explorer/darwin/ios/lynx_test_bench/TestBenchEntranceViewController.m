// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchEntranceViewController.h"
#import <LynxDevtool/TestBenchTraceProfileHelper.h>
#import "TestBenchActionManager.h"
#import "TestBenchEnv.h"
#import "TestBenchUIHelper.h"
#import "TestBenchViewController.h"

static NSInteger kSTOPTRACESECOND = 20;
NSString *const TESTBENCH_ASSETS_SCHEME = @"asset://";

@interface TestBenchEntranceViewController ()
@property(nonatomic, strong) UITextView *textView;
@property(nonatomic, strong) TestBenchTraceProfileHelper *traceHelper;
@property(nonatomic, assign) BOOL isStartTrace;
@property(nonatomic, assign) NSInteger delaySeconds;
@property(nonatomic, strong) id<TestBenchActionCallback> actionCallback;

@end

@implementation TestBenchEntranceViewController

+ (BOOL)entranceTestBenchView:(id<TestBenchActionCallback>)actionCallback {
  NSArray<NSString *> *argvs = [[NSProcessInfo processInfo] arguments];

  for (NSString *arg_str in argvs) {
    if (arg_str.length > 0 &&
        [arg_str hasPrefix:[[TestBenchEnv sharedInstance] testBenchUrlPrefix]]) {
      TestBenchEntranceViewController *testbenchVC = [TestBenchEntranceViewController new];
      testbenchVC.url = arg_str;
      testbenchVC.actionCallback = actionCallback;
      UINavigationController *navigationVC = [TestBenchUIHelper getTopNavigationController];
      [navigationVC popViewControllerAnimated:NO];
      [navigationVC pushViewController:testbenchVC animated:YES];
      NSInteger index = [argvs indexOfObject:arg_str];
      if (index < (argvs.count - 1)) {
        testbenchVC.delaySeconds = [argvs[index + 1] integerValue];
      }

      return YES;
    }
  }
  return NO;
}

+ (BOOL)enableLoadLocalRecordFile:(NSString *)url {
  if (url.length > 0) {
    return [url hasPrefix:TESTBENCH_ASSETS_SCHEME];
  }
  return NO;
}

+ (NSData *)localRecordFileData:(NSString *)url {
  NSString *path = [url substringFromIndex:[TESTBENCH_ASSETS_SCHEME length]];
  NSString *filePath =
      [[NSBundle mainBundle] pathForResource:[@"Resource" stringByAppendingString:path] ofType:@""];
  NSData *data = [NSData dataWithContentsOfFile:filePath];
  return data;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    self.delaySeconds = 3;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.title = @"TestBench Entrance";

  self.view.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.2];
  self.textView = [[UITextView alloc] initWithFrame:self.view.bounds];
  self.textView.backgroundColor = [[UIColor grayColor] colorWithAlphaComponent:0.1];
  self.textView.textColor = [UIColor grayColor];
  self.textView.text = self.url;
  self.textView.textContainerInset = UIEdgeInsetsMake(0, 10, 0, 10);
  [self.view addSubview:self.textView];
  [self.textView sizeToFit];

  self.traceHelper = [TestBenchTraceProfileHelper new];

  NSString *ark_template_str = self.url;
  ark_template_str = [ark_template_str stringByRemovingPercentEncoding];
  __weak typeof(self) weakSelf = self;
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [self startTrace];
    [self openTargetUrl:ark_template_str];
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, (kSTOPTRACESECOND + weakSelf.delaySeconds) * NSEC_PER_SEC),
        dispatch_get_main_queue(), ^{
          [self stopTrace];
        });
  });
}

- (void)openTargetUrl:(NSString *)sourceUrl {
  NSURL *source = [NSURL URLWithString:sourceUrl];
  if ([source.scheme isEqualToString:[[TestBenchEnv sharedInstance] testBenchUrlSchema]]) {
    if ([[source host] isEqualToString:[[TestBenchEnv sharedInstance] testBenchUrlHost]]) {
      NSArray *query = [[source query] componentsSeparatedByString:@"&"];

      TestBenchViewController *tbVC = [TestBenchViewController new];
      [tbVC registerTestBenchActionCallback:self.actionCallback];
      if ([query containsObject:@"fullScreen=true"]) {
        tbVC.fullScreen = YES;
      } else {
        tbVC.fullScreen = NO;
      }
      tbVC.url = sourceUrl;
      __weak typeof(self) weakSelf = self;
      tbVC.endTestBenchBlock = ^{
        dispatch_after(
            dispatch_time(DISPATCH_TIME_NOW, (int64_t)(weakSelf.delaySeconds * NSEC_PER_SEC)),
            dispatch_get_main_queue(), ^{
              __strong __typeof(weakSelf) strongSelf = weakSelf;
              [strongSelf stopTrace];
            });
      };
      UINavigationController *vc = [TestBenchUIHelper getTopNavigationController];
      if (![query containsObject:@"pop=false"]) {
        [vc popViewControllerAnimated:NO];
      }
      [vc pushViewController:tbVC animated:YES];
      return;
    }
  }
}

- (void)startTrace {
  self.isStartTrace = YES;
  [self.traceHelper startTrace];
}

- (void)stopTrace {
  if (!self.isStartTrace) {
    return;
  }
  self.isStartTrace = NO;
  [self.traceHelper stopTrace];
  [self renameLocalTraceFile];
}

- (void)renameLocalTraceFile {
  static NSString *LYNX_TESTBENCH_TRACE = @"lynx-benchmark-trace";
  NSString *documentsPath =
      [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
  NSArray<NSString *> *file_list =
      [[NSFileManager defaultManager] contentsOfDirectoryAtPath:documentsPath error:NULL];
  BOOL isExit = [[NSFileManager defaultManager]
      fileExistsAtPath:[documentsPath stringByAppendingPathComponent:LYNX_TESTBENCH_TRACE]];
  if (isExit) {
    return;
  }
  for (NSString *file in file_list) {
    if ([file hasPrefix:@"lynx-profile-trace"]) {
      NSString *filePath = [documentsPath stringByAppendingPathComponent:file];
      NSString *moveToPath = [documentsPath stringByAppendingPathComponent:LYNX_TESTBENCH_TRACE];
      BOOL isSuccess = [[NSFileManager defaultManager] moveItemAtPath:filePath
                                                               toPath:moveToPath
                                                                error:nil];
      if (isSuccess) {
        NSLog(@"rename success");
      } else {
        NSLog(@"rename fail");
      }
      break;
    }
  }
}
@end
