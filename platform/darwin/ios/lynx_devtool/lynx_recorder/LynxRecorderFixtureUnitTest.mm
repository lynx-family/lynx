// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderActionManager.h>
#import <LynxDevtool/LynxRecorderFixture.h>
#import <LynxDevtool/LynxRecorderReplayDataModule.h>
#import <XCTest/XCTest.h>

#include "third_party/zlib/contrib/minizip/zip.h"

@interface LynxRecorderReplayDataModule (FixtureTest)
- (NSString *)getFixtureDirectory;
- (void)releaseFixture;
@end

@interface LynxRecorderActionManager (FixtureTest)
- (void)handleActionList:(NSArray *)actions;
- (void)runAction:(id)action onTestBenchComplete:(void (^)(int64_t))completion;
@end

@interface FixtureTestProvider : NSObject <LynxRecorderReplayDataProvider>
@property LynxRecorderFixture *fixture;
@end
@implementation FixtureTestProvider
- (NSArray *)getFunctionCall {
  return @[];
}
- (NSDictionary *)getCallbackData {
  return @{};
}
- (NSArray *)getJSbIgnoredInfo {
  return @[];
}
- (NSDictionary *)getJsbSettings {
  return @{};
}
- (NSDictionary *)getSharedData {
  return @{};
}
- (LynxRecorderFixture *)getFixture {
  return self.fixture;
}
@end

@interface FixtureTestActionManager : LynxRecorderActionManager
@property NSMutableArray *immediateActions;
@property(copy) void (^eventCallback)(void);
@end
@implementation FixtureTestActionManager
- (void)dispatchAction:(NSString *)name params:(NSDictionary *)params interval:(NSInteger)interval {
  [self.immediateActions addObject:name];
}
- (void)sendGlobalEvent:(NSDictionary *)params {
  if (self.eventCallback) self.eventCallback();
}
@end

@interface LynxRecorderFixtureUnitTest : XCTestCase
@end
@implementation LynxRecorderFixtureUnitTest

- (NSData *)zip:(NSArray<NSArray *> *)entries {
  NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
  zipFile archive = zipOpen64(path.fileSystemRepresentation, APPEND_STATUS_CREATE);
  XCTAssertNotEqual(archive, nullptr);
  for (NSArray *entry in entries) {
    NSString *name = entry[0];
    NSData *bytes = [entry[1] dataUsingEncoding:NSUTF8StringEncoding];
    XCTAssertEqual(zipOpenNewFileInZip(archive, name.UTF8String, nullptr, nullptr, 0, nullptr, 0,
                                       nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION),
                   ZIP_OK);
    XCTAssertEqual(zipWriteInFileInZip(archive, bytes.bytes, (unsigned int)bytes.length), ZIP_OK);
    XCTAssertEqual(zipCloseFileInZip(archive), ZIP_OK);
  }
  XCTAssertEqual(zipClose(archive, nullptr), ZIP_OK);
  NSData *data = [NSData dataWithContentsOfFile:path];
  [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
  return data;
}

#if ENABLE_TESTBENCH_REPLAY
- (void)testEvaluationAndAssetLifetime {
  NSString *directory;
  LynxRecorderReplayDataModule *module;
  @autoreleasepool {
    NSData *data = [self zip:@[
      @[
        @"fixture.js", (@"export default function(ctx) {"
                         "ctx.setGlobalProps({label:'hello 👋'});"
                         "ctx.loadTemplate('test://page','template/page.bin',{title:'fixture'});"
                         "ctx.sharedData('answer',42);"
                         "ctx.after(25,()=>ctx.sendGlobalEvent({arguments:['ready',[]]}));}")
      ],
      @[ @"assets/template/page.bin", @"template bytes" ],
      @[ @"config.json", @"{\"enablePreDecode\":true}" ], @[ @"component_list.json", @"[]" ]
    ]];
    NSError *error = nil;
    LynxRecorderFixture *fixture = [LynxRecorderFixture openData:data error:&error];
    XCTAssertNotNil(fixture);
    XCTAssertNil(error);
    directory = fixture.directory;
    NSArray *actions = fixture.record[@"Action List"];
    XCTAssertEqualObjects(actions[0][@"Params"][@"global_props"][@"label"], @"hello 👋");
    NSData *source = [[NSData alloc] initWithBase64EncodedString:actions[1][@"Params"][@"source"]
                                                         options:0];
    XCTAssertEqualObjects([[NSString alloc] initWithData:source encoding:NSUTF8StringEncoding],
                          @"template bytes");
    XCTAssertEqualObjects(actions[1][@"Params"][@"templateData"][@"title"], @"fixture");
    XCTAssertEqualObjects(actions[2][@"RecordMillisecond"], @25);
    XCTAssertEqualObjects(fixture.record[@"SharedData"][@"answer"], @42);
    XCTAssertEqualObjects(fixture.record[@"Config"][@"enablePreDecode"], @YES);

    FixtureTestProvider *provider = [FixtureTestProvider new];
    provider.fixture = fixture;
    module = [[LynxRecorderReplayDataModule alloc] initWithParam:provider];
    XCTAssertEqualObjects([module getFixtureDirectory], directory);
    provider.fixture = nil;
    fixture = nil;
    [module destroy];
    XCTAssertEqualObjects([module getFixtureDirectory], @"");
  }
  XCTAssertTrue([[NSFileManager defaultManager] fileExistsAtPath:directory]);
  [module releaseFixture];
  [module releaseFixture];
  XCTAssertFalse([[NSFileManager defaultManager] fileExistsAtPath:directory]);
  LynxRecorderReplayDataModule *jsonModule =
      [[LynxRecorderReplayDataModule alloc] initWithParam:[NSObject new]];
  XCTAssertEqualObjects([jsonModule getFixtureDirectory], @"");
}

- (void)testInvalidPackagesCleanUpAssets {
  NSArray *invalid = @[
    @[ @[ @"../escaped", @"bad" ], @[ @"fixture.js", @"export default function(ctx) {}" ] ],
    @[ @[ @"fixture.js", @"throw new Error('invalid');" ] ], @[ @[
      @"fixture.js", @"export default function(ctx) {ctx.loadTemplate('test://page','../bad',{});}"
    ] ],
    @[ @[ @"config.json", @"{}" ] ],
    @[ @[ @"bad/", @"directory payload" ], @[ @"fixture.js", @"export default function(ctx) {}" ] ],
    @[ @[ @"fixture.js", @"export default function(ctx) {}" ], @[ @"fixture.js", @"duplicate" ] ]
  ];
  NSFileManager *files = NSFileManager.defaultManager;
  NSPredicate *packages = [NSPredicate predicateWithBlock:^BOOL(id name, NSDictionary *bindings) {
    return [name hasPrefix:@"lynx-fixture-"];
  }];
  NSArray *before = [[files contentsOfDirectoryAtPath:NSTemporaryDirectory()
                                                error:nil] filteredArrayUsingPredicate:packages];
  for (NSArray *entries in invalid) {
    @autoreleasepool {
      NSError *error = nil;
      XCTAssertNil([LynxRecorderFixture openData:[self zip:entries] error:&error]);
      XCTAssertNotNil(error);
    }
  }
  NSArray *after = [[files contentsOfDirectoryAtPath:NSTemporaryDirectory()
                                               error:nil] filteredArrayUsingPredicate:packages];
  XCTAssertEqualObjects([NSSet setWithArray:before], [NSSet setWithArray:after]);
}

- (void)testBothFormatsDeferInteractionsAndReplaceReloadActions {
  LynxRecorderFixture *fixture = [LynxRecorderFixture
      openData:[self zip:@[ @[ @"fixture.js", @"export default function(ctx) {}" ] ]]
         error:nil];
  for (NSNumber *useFixture in @[ @NO, @YES ]) {
    FixtureTestActionManager *manager = [FixtureTestActionManager new];
    manager.immediateActions = [NSMutableArray array];
    [manager setValue:[NSMutableArray array] forKey:@"reloadFuncList"];
    [manager setValue:[[LynxRecorderReplayConfig alloc]
                          initWithProductUrl:@"sslocal://arkview?url=https://example.com/replay"]
               forKey:@"replayConfig"];
    FixtureTestProvider *provider = [FixtureTestProvider new];
    provider.fixture = useFixture.boolValue ? fixture : nil;
    [manager setValue:provider forKey:@"dataProvider"];
    NSInteger origin = useFixture.boolValue ? 0 : 1000000;
    NSArray *actions = @[
      @{
        @"Function Name" : @"setGlobalProps",
        @"Record Time" : @0,
        @"RecordMillisecond" : @(origin),
        @"Params" : @{}
      },
      @{
        @"Function Name" : @"sendGlobalEvent",
        @"Record Time" : @0,
        @"RecordMillisecond" : @(origin + 50),
        @"Params" : @{}
      }
    ];
    [manager handleActionList:actions];
    [manager handleActionList:actions];
    XCTAssertEqualObjects(manager.immediateActions, (@[ @"setGlobalProps", @"setGlobalProps" ]));
    XCTAssertEqual([[manager valueForKey:@"reloadFuncList"] count], 1u);
    __block BOOL delivered = NO;
    XCTestExpectation *event = [self expectationWithDescription:@"interaction after first screen"];
    manager.eventCallback = ^{
      delivered = YES;
      [event fulfill];
    };
    XCTAssertFalse(delivered);
    [manager reloadAction];
    XCTAssertFalse(delivered);
    [self waitForExpectationsWithTimeout:1 handler:nil];
  }
}

- (void)testPreviousReplayInteractionsAreInvalidated {
  FixtureTestActionManager *manager = [FixtureTestActionManager new];
  [manager setValue:[NSMutableArray array] forKey:@"reloadFuncList"];
  [manager setValue:[[LynxRecorderReplayConfig alloc]
                        initWithProductUrl:@"sslocal://arkview?url=https://example.com/replay"]
             forKey:@"replayConfig"];
  [manager runAction:@{
    @"Function Name" : @"sendGlobalEvent",
    @"RecordMillisecond" : @1000,
    @"Params" : @{}
  }
      onTestBenchComplete:^(int64_t interval){
      }];
  XCTestExpectation *oldEvent = [self expectationWithDescription:@"previous replay event"];
  oldEvent.inverted = YES;
  manager.eventCallback = ^{
    [oldEvent fulfill];
  };
  [manager reloadAction];
  [manager setValue:@1 forKey:@"replayGeneration"];
  [self waitForExpectationsWithTimeout:0.1 handler:nil];
}

#else
- (void)testDisabledBuildRejectsFixture {
  NSError *error = nil;
  XCTAssertNil([LynxRecorderFixture openData:[NSData data] error:&error]);
  XCTAssertNotNil(error);
}
#endif

@end
