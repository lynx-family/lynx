// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>
#import <LynxDevtool/LynxDebugBridge.h>
#import <XCTest/XCTest.h>

@interface LynxDebugBridgeUnitTest : XCTestCase
@end

@implementation LynxDebugBridgeUnitTest

- (void)setUp {
  [super setUp];
  // Clear NSUserDefaults before each test
  NSString *appDomain = [[NSBundle mainBundle] bundleIdentifier];
  if (appDomain) {
    [[NSUserDefaults standardUserDefaults] removePersistentDomainForName:appDomain];
  } else {
    NSDictionary *defaults = [[NSUserDefaults standardUserDefaults] dictionaryRepresentation];
    for (NSString *key in defaults) {
      [[NSUserDefaults standardUserDefaults] removeObjectForKey:key];
    }
  }
}

- (void)testSupportedGlobalSwitches {
  LynxDebugBridge *bridge = [LynxDebugBridge singleton];
  DevToolSettings *settings = [DevToolSettings sharedInstance];

  // Test DEVTOOL
  [bridge handleSetGlobalSwitch:SP_KEY_ENABLE_DEVTOOL value:YES];
  XCTAssertTrue(settings.devToolEnabled);
  XCTAssertTrue([bridge handleGetGlobalSwitch:SP_KEY_ENABLE_DEVTOOL]);

  // Test LOGBOX
  [bridge handleSetGlobalSwitch:SP_KEY_ENABLE_LOGBOX value:NO];
  XCTAssertFalse(settings.logBoxEnabled);
  XCTAssertFalse([bridge handleGetGlobalSwitch:SP_KEY_ENABLE_LOGBOX]);

  // Test QUICKJS_DEBUG
  [bridge handleSetGlobalSwitch:SP_KEY_ENABLE_QUICKJS_DEBUG value:YES];
  XCTAssertTrue(settings.quickjsDebugEnabled);
  XCTAssertTrue([bridge handleGetGlobalSwitch:SP_KEY_ENABLE_QUICKJS_DEBUG]);

  // Test DOM_TREE
  [bridge handleSetGlobalSwitch:SP_KEY_ENABLE_DOM_TREE value:NO];
  XCTAssertFalse(settings.domTreeEnabled);
  XCTAssertFalse([bridge handleGetGlobalSwitch:SP_KEY_ENABLE_DOM_TREE]);

  // Test LONG_PRESS_MENU
  [bridge handleSetGlobalSwitch:SP_KEY_ENABLE_LONG_PRESS_MENU value:NO];
  XCTAssertFalse(settings.longPressMenuEnabled);
  XCTAssertFalse([bridge handleGetGlobalSwitch:SP_KEY_ENABLE_LONG_PRESS_MENU]);

  // Test PERF_METRICS
  [bridge handleSetGlobalSwitch:SP_KEY_ENABLE_PERF_METRICS value:YES];
  XCTAssertTrue(settings.perfMetricsEnabled);
  XCTAssertTrue([bridge handleGetGlobalSwitch:SP_KEY_ENABLE_PERF_METRICS]);
}

- (void)testUnsupportedGlobalSwitches {
  LynxDebugBridge *bridge = [LynxDebugBridge singleton];
  DevToolSettings *settings = [DevToolSettings sharedInstance];

  // Test unsupported key: LAUNCH_RECORD (removed from supported list)
  settings.launchRecordEnabled = NO;
  [bridge handleSetGlobalSwitch:SP_KEY_ENABLE_LAUNCH_RECORD value:YES];
  XCTAssertFalse(settings.launchRecordEnabled);  // Should remain NO

  // Test unsupported key: HIGHLIGHT_TOUCH (removed from supported list)
  settings.highlightTouchEnabled = NO;
  [bridge handleSetGlobalSwitch:SP_KEY_ENABLE_HIGHLIGHT_TOUCH value:YES];
  XCTAssertFalse(settings.highlightTouchEnabled);  // Should remain NO

  // Test unsupported key: CDP domains (not added to supported list)
  [settings setCDPDomain:SP_KEY_ENABLE_CDP_DOMAIN_DOM enabled:NO];
  [bridge handleSetGlobalSwitch:SP_KEY_ENABLE_CDP_DOMAIN_DOM value:YES];
  XCTAssertFalse([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_DOM]);

  // Test random key
  [bridge handleSetGlobalSwitch:@"random_key" value:YES];
  XCTAssertFalse([bridge handleGetGlobalSwitch:@"random_key"]);
}

@end
