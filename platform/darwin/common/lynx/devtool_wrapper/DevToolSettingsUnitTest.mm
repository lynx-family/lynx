// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>
#import <Lynx/LynxErrorBehavior.h>
#import <XCTest/XCTest.h>

static NSString *const kDevToolSettingsTestIgnoreErrorTypesKey = @"ignore_error_types";
static NSString *const kDevToolSettingsTestActivatedCDPDomainsKey = @"activated_cdp_domains";

@interface DevToolSettingsUnitTest : XCTestCase
@end

@implementation DevToolSettingsUnitTest

- (void)setUp {
  [super setUp];
  // Clear NSUserDefaults before each test
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  NSString *appDomain = [[NSBundle mainBundle] bundleIdentifier];
  if (appDomain) {
    [defaults removePersistentDomainForName:appDomain];
  } else {
    // Fallback for test environments where bundleIdentifier might be nil
    NSDictionary *defaultValues = [defaults dictionaryRepresentation];
    for (NSString *key in defaultValues) {
      [defaults removeObjectForKey:key];
    }
  }
  [defaults removeObjectForKey:kDevToolSettingsTestIgnoreErrorTypesKey];
  [defaults removeObjectForKey:kDevToolSettingsTestActivatedCDPDomainsKey];
  [defaults synchronize];
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
  XCTAssertFalse([settings isCSSErrorIgnored]);
  XCTAssertTrue([[settings ignoredErrorTypes] count] == 0);
  XCTAssertTrue([[settings enabledCDPDomains] count] == 0);
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

- (void)testIgnoreErrorCSSPersistence {
  DevToolSettings *settings = [DevToolSettings sharedInstance];
  [settings setCSSErrorIgnored:YES];

  XCTAssertTrue([settings isCSSErrorIgnored]);
  XCTAssertTrue([settings isErrorTypeIgnored:EBLynxCSS]);
  NSDictionary *ignoredErrorTypes = [[NSUserDefaults standardUserDefaults]
      dictionaryForKey:kDevToolSettingsTestIgnoreErrorTypesKey];
  XCTAssertNotNil(ignoredErrorTypes[@(EBLynxCSS).stringValue]);
}

- (void)testIgnoredErrorTypesPersistence {
  DevToolSettings *settings = [DevToolSettings sharedInstance];
  NSSet<NSString *> *ignoredErrorTypes = [NSSet setWithObject:@(EBLynxCSS).stringValue];
  [settings setIgnoredErrorTypes:ignoredErrorTypes];

  XCTAssertEqualObjects(ignoredErrorTypes, [settings ignoredErrorTypes]);

  [settings setErrorType:EBLynxCSS ignored:NO];
  XCTAssertFalse([settings isErrorTypeIgnored:EBLynxCSS]);
}

- (void)testEnabledCDPDomainsPersistence {
  DevToolSettings *settings = [DevToolSettings sharedInstance];
  NSSet<NSString *> *enabledDomains = [NSSet
      setWithObjects:SP_KEY_ENABLE_CDP_DOMAIN_DOM, SP_KEY_ENABLE_CDP_DOMAIN_CSS,
                     SP_KEY_ENABLE_CDP_DOMAIN_PAGE, SP_KEY_ENABLE_CDP_DOMAIN_DEBUGGER,
                     SP_KEY_ENABLE_CDP_DOMAIN_OVERLAY, SP_KEY_ENABLE_CDP_DOMAIN_RUNTIME, nil];
  [settings setEnabledCDPDomains:enabledDomains];

  XCTAssertEqualObjects(enabledDomains, [settings enabledCDPDomains]);
  NSArray *persistedDomains = [[NSUserDefaults standardUserDefaults]
      arrayForKey:kDevToolSettingsTestActivatedCDPDomainsKey];
  XCTAssertEqualObjects(enabledDomains, [NSSet setWithArray:persistedDomains]);
}

- (void)testSetCDPDomainEnabled {
  DevToolSettings *settings = [DevToolSettings sharedInstance];

  XCTAssertFalse([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_CSS]);

  [settings setCDPDomain:SP_KEY_ENABLE_CDP_DOMAIN_CSS enabled:YES];
  XCTAssertTrue([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_CSS]);

  [settings setCDPDomain:SP_KEY_ENABLE_CDP_DOMAIN_CSS enabled:NO];
  XCTAssertFalse([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_CSS]);
}

@end
