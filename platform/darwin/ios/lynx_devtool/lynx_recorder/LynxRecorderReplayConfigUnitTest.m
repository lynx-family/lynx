// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderActionManager.h>
#import <LynxDevtool/LynxRecorderReplayConfig.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

@interface LynxRecorderReplayConfig (RuntimeOptionsTest)
@property(nonatomic, readonly, nullable) NSNumber *enableBTSOverride;
@end

@interface LynxRecorderActionManager (BooleanDataTest)
- (LynxTemplateData *)templateDataWithDictionary:(NSDictionary *)dictionary;
- (void)updateDataByPreParsedData:(NSDictionary *)params;
@end

@interface LynxRecorderReplayConfigUnitTest : XCTestCase
@end

@implementation LynxRecorderReplayConfigUnitTest

- (LynxRecorderReplayConfig *)configWithQuery:(NSString *)query {
  NSString *url = [@"sslocal://arkview?url=https%3A%2F%2Fexample.com%2Freplay.json"
      stringByAppendingString:query];
  return [[LynxRecorderReplayConfig alloc] initWithProductUrl:url];
}

- (void)testMissingEnableBTSDoesNotOverrideRecording {
  XCTAssertNil([self configWithQuery:@""].enableBTSOverride);
  XCTAssertFalse([self configWithQuery:@""].enableAirStrictMode);
}

- (void)testEnableBTSParsesZeroAndOne {
  XCTAssertFalse([self configWithQuery:@"&enable_bts=0"].enableBTSOverride.boolValue);
  XCTAssertTrue([self configWithQuery:@"&enable_bts=1"].enableBTSOverride.boolValue);
}

- (void)testEnableBTSTakesPrecedenceOverLegacyAirStrictMode {
  LynxRecorderReplayConfig *enableConfig =
      [self configWithQuery:@"&enableAirStrict=true&enable_bts=1"];
  XCTAssertTrue(enableConfig.enableBTSOverride.boolValue);
  XCTAssertTrue(enableConfig.enableAirStrictMode);
  LynxRecorderReplayConfig *disableConfig =
      [self configWithQuery:@"&enableAirStrict=false&enable_bts=0"];
  XCTAssertFalse(disableConfig.enableBTSOverride.boolValue);
  XCTAssertFalse(disableConfig.enableAirStrictMode);
}

- (void)testEnableBTSFallsBackToLegacyAirStrictMode {
  XCTAssertTrue([self configWithQuery:@"&enableAirStrict=true"].enableAirStrictMode);
  XCTAssertFalse([self configWithQuery:@"&enableAirStrict=false"].enableAirStrictMode);
}

- (void)testEmbeddedModeParsesInteger {
  XCTAssertEqual([self configWithQuery:@"&embedded_mode=5"].embeddedMode, 5);
}

- (void)testBooleanDataConversionIsOptIn {
  XCTAssertFalse([self configWithQuery:@""].useBoolLiterals);
  XCTAssertTrue([self configWithQuery:@"&useBoolLiterals=true"].useBoolLiterals);
  XCTAssertFalse([self configWithQuery:@"&useBoolLiterals=false"].useBoolLiterals);
}

- (void)testRecordedDataPreservesBooleanTypesWithoutChangingNumbers {
  LynxRecorderActionManager *manager = [[LynxRecorderActionManager alloc] init];
  NSDictionary *input = @{@"nested" : @{@"enabled" : @YES, @"disabled" : @NO, @"number" : @1}};
  for (NSString *query in @[ @"", @"&useBoolLiterals=true" ]) {
    LynxRecorderReplayConfig *config = [self configWithQuery:query];
    [manager setValue:config forKey:@"replayConfig"];
    NSDictionary *nested = [manager templateDataWithDictionary:input].dictionary[@"nested"];
    XCTAssertEqual(CFGetTypeID((__bridge CFTypeRef)nested[@"enabled"]) == CFBooleanGetTypeID(),
                   config.useBoolLiterals);
    XCTAssertEqual(CFGetTypeID((__bridge CFTypeRef)nested[@"disabled"]) == CFBooleanGetTypeID(),
                   config.useBoolLiterals);
    XCTAssertNotEqual(CFGetTypeID((__bridge CFTypeRef)nested[@"number"]), CFBooleanGetTypeID());
    XCTAssertEqualObjects(nested[@"number"], @1);
  }
}

- (void)testRecordedUpdatePreservesBooleans {
  LynxRecorderActionManager *manager = [[LynxRecorderActionManager alloc] init];
  [manager setValue:[self configWithQuery:@"&useBoolLiterals=true"] forKey:@"replayConfig"];
  id view = OCMClassMock([LynxView class]);
  [manager setValue:view forKey:@"lynxView"];
  __block LynxTemplateData *updatedData = nil;
  OCMStub([view updateDataWithTemplateData:[OCMArg any]]).andDo(^(NSInvocation *invocation) {
    __unsafe_unretained LynxTemplateData *data;
    [invocation getArgument:&data atIndex:2];
    updatedData = data;
  });
  [manager updateDataByPreParsedData:@{
    @"value" : @{@"enabled" : @YES, @"disabled" : @NO, @"number" : @1},
    @"preprocessorName" : @"update"
  }];
  XCTAssertNotNil(updatedData);
  if (updatedData == nil) {
    [view stopMocking];
    return;
  }
  XCTAssertEqual(CFGetTypeID((__bridge CFTypeRef)updatedData.dictionary[@"enabled"]),
                 CFBooleanGetTypeID());
  XCTAssertEqual(CFGetTypeID((__bridge CFTypeRef)updatedData.dictionary[@"disabled"]),
                 CFBooleanGetTypeID());
  XCTAssertNotEqual(CFGetTypeID((__bridge CFTypeRef)updatedData.dictionary[@"number"]),
                    CFBooleanGetTypeID());
  XCTAssertTrue(updatedData.isReadOnly);
  [view stopMocking];
}

- (void)testCallJSModuleReplaysBeforeFirstScreen {
  LynxRecorderReplayConfig *config = [self configWithQuery:@""];
  XCTAssertTrue([config.canMockFuncName containsObject:@"callJSModule"]);
  XCTAssertFalse([config.reloadFuncName containsObject:@"callJSModule"]);
}

@end
