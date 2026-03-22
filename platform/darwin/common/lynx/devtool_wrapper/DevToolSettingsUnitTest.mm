// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>
#import <XCTest/XCTest.h>

@interface DevToolSettingsUnitTest : XCTestCase
@end

@implementation DevToolSettingsUnitTest

- (void)setUp {
  [super setUp];
  // Clear NSUserDefaults before each test
  NSString *appDomain = [[NSBundle mainBundle] bundleIdentifier];
  if (appDomain) {
    [[NSUserDefaults standardUserDefaults] removePersistentDomainForName:appDomain];
  } else {
    // Fallback for test environments where bundleIdentifier might be nil
    NSDictionary *defaults = [[NSUserDefaults standardUserDefaults] dictionaryRepresentation];
    for (NSString *key in defaults) {
      [[NSUserDefaults standardUserDefaults] removeObjectForKey:key];
    }
  }
}

- (void)testSingleton {
  DevToolSettings *settings = [DevToolSettings sharedInstance];
  XCTAssertNotNil(settings);
  XCTAssertEqual(settings, [DevToolSettings sharedInstance]);
}

- (void)testDefaultValues {
  DevToolSettings *settings = [DevToolSettings sharedInstance];

  // Verify expected default values
  XCTAssertFalse(settings.devToolEnabled);
  XCTAssertTrue(settings.logBoxEnabled);
  XCTAssertFalse(settings.launchRecordEnabled);
  XCTAssertTrue(settings.quickjsDebugEnabled);
  XCTAssertTrue(settings.domTreeEnabled);
  XCTAssertFalse(settings.perfMetricsEnabled);
  XCTAssertFalse(settings.fspScreenshotEnabled);
  XCTAssertFalse(settings.highlightTouchEnabled);
  XCTAssertTrue(settings.longPressMenuEnabled);
  XCTAssertTrue(settings.previewScreenshotEnabled);
}

- (void)testPersistence {
  DevToolSettings *settings = [DevToolSettings sharedInstance];

  // Change values
  settings.devToolEnabled = YES;
  settings.logBoxEnabled = NO;

  // Verify values are updated in memory
  XCTAssertTrue(settings.devToolEnabled);
  XCTAssertFalse(settings.logBoxEnabled);

  // Verify persistence in NSUserDefaults
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  XCTAssertTrue([defaults boolForKey:SP_KEY_ENABLE_DEVTOOL]);
  XCTAssertFalse([defaults boolForKey:SP_KEY_ENABLE_LOGBOX]);
}

@end
