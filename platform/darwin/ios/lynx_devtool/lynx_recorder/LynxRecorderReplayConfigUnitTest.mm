// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateData+Converter.h>
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
    LynxTemplateData *data = [manager templateDataWithDictionary:input];
    // The dictionary getter converts Lepus booleans back to numeric NSNumbers.
    const auto nested = LynxGetLepusValueFromTemplateData(data)->GetProperty("nested");
    XCTAssertEqual(nested.GetProperty("enabled").IsBool(), config.useBoolLiterals);
    XCTAssertEqual(nested.GetProperty("disabled").IsBool(), config.useBoolLiterals);
    XCTAssertTrue(nested.GetProperty("number").IsNumber());
    XCTAssertEqual(nested.GetProperty("number").Number(), 1);
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
  // The manager accesses its view during deallocation.
  [manager setValue:nil forKey:@"lynxView"];
  [view stopMocking];
  XCTAssertNotNil(updatedData);
  if (updatedData == nil) {
    return;
  }
  const auto *data = LynxGetLepusValueFromTemplateData(updatedData);
  XCTAssertTrue(data->GetProperty("enabled").IsBool());
  XCTAssertTrue(data->GetProperty("disabled").IsBool());
  XCTAssertTrue(data->GetProperty("number").IsNumber());
  XCTAssertTrue(updatedData.isReadOnly);
}

- (void)testCallJSModuleReplaysBeforeFirstScreen {
  LynxRecorderReplayConfig *config = [self configWithQuery:@""];
  XCTAssertTrue([config.canMockFuncName containsObject:@"callJSModule"]);
  XCTAssertFalse([config.reloadFuncName containsObject:@"callJSModule"]);
}

@end
