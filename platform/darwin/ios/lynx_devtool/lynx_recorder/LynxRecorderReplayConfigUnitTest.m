// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderReplayConfig.h>
#import <XCTest/XCTest.h>

@interface LynxRecorderReplayConfig (RuntimeOptionsTest)
@property(nonatomic, readonly, nullable) NSNumber *enableBTSOverride;
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

@end
