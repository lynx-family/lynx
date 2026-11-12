// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>
#import <Lynx/LynxErrorBehavior.h>
#import <XCTest/XCTest.h>

static NSString *const kDevToolSettingsTestIgnoreErrorTypesKey = @"ignore_error_types";
static NSString *const kDevToolSettingsTestActivatedCDPDomainsKey = @"activated_cdp_domains";

@interface DevToolSettings (BacktraceInternalForTest)
+ (NSArray<NSNumber *> *)buildReportedBacktraceAddresses;
+ (NSString *)buildBacktraceAddressSummary;
@end

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
  [[DevToolSettings sharedInstance] setEnabledCDPDomains:[NSSet set]];
}

- (void)tearDown {
  [[DevToolSettings sharedInstance] setEnabledCDPDomains:[NSSet set]];
  [[NSUserDefaults standardUserDefaults]
      removeObjectForKey:kDevToolSettingsTestActivatedCDPDomainsKey];
  [super tearDown];
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
  XCTAssertFalse(settings.longPressMenuEnabled);
  XCTAssertTrue(settings.previewScreenshotEnabled);
  XCTAssertFalse([settings isCSSErrorIgnored]);
  XCTAssertTrue([[settings ignoredErrorTypes] count] == 0);
  XCTAssertTrue([[settings enabledCDPDomains] count] == 0);
  XCTAssertNotNil(settings.bootstrap);
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

- (void)testBootstrapDefaultValues {
  DevToolBootstrapSettings *bootstrap = [[DevToolBootstrapSettings alloc] init];

  XCTAssertFalse(bootstrap.lynxDebugEnabled);
  XCTAssertFalse(bootstrap.logBoxEnabled);
}

- (void)testBootstrapSettingsAreNotPersisted {
  DevToolBootstrapSettings *bootstrap = [[DevToolBootstrapSettings alloc] init];
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  NSDictionary *before = [defaults dictionaryRepresentation];

  bootstrap.lynxDebugEnabled = YES;
  bootstrap.logBoxEnabled = YES;

  XCTAssertTrue(bootstrap.lynxDebugEnabled);
  XCTAssertTrue(bootstrap.logBoxEnabled);
  XCTAssertEqualObjects(before, [defaults dictionaryRepresentation]);
}

- (void)testBootstrapDevelopmentDefaultsFillOnlyUnset {
  DevToolBootstrapSettings *bootstrap = [[DevToolBootstrapSettings alloc] init];
  bootstrap.logBoxEnabled = NO;

  [bootstrap applyDevelopmentDefaultsIfUnset];

  XCTAssertTrue(bootstrap.lynxDebugEnabled);
  XCTAssertFalse(bootstrap.logBoxEnabled);
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

- (void)testEnabledCDPDomainsAreNotPersisted {
  DevToolSettings *settings = [DevToolSettings sharedInstance];
  NSSet<NSString *> *enabledDomains = [NSSet
      setWithObjects:SP_KEY_ENABLE_CDP_DOMAIN_DOM, SP_KEY_ENABLE_CDP_DOMAIN_CSS,
                     SP_KEY_ENABLE_CDP_DOMAIN_PAGE, SP_KEY_ENABLE_CDP_DOMAIN_DEBUGGER,
                     SP_KEY_ENABLE_CDP_DOMAIN_OVERLAY, SP_KEY_ENABLE_CDP_DOMAIN_RUNTIME, nil];
  [settings setEnabledCDPDomains:enabledDomains];

  XCTAssertEqualObjects(enabledDomains, [settings enabledCDPDomains]);
  XCTAssertNil([[NSUserDefaults standardUserDefaults]
      objectForKey:kDevToolSettingsTestActivatedCDPDomainsKey]);
}

- (void)testSetCDPDomainEnabled {
  DevToolSettings *settings = [DevToolSettings sharedInstance];

  XCTAssertFalse([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_CSS]);

  [settings setCDPDomain:SP_KEY_ENABLE_CDP_DOMAIN_CSS enabled:YES];
  XCTAssertTrue([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_CSS]);
  XCTAssertNil([[NSUserDefaults standardUserDefaults]
      objectForKey:kDevToolSettingsTestActivatedCDPDomainsKey]);

  [settings setCDPDomain:SP_KEY_ENABLE_CDP_DOMAIN_CSS enabled:NO];
  XCTAssertFalse([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_CSS]);
  XCTAssertNil([[NSUserDefaults standardUserDefaults]
      objectForKey:kDevToolSettingsTestActivatedCDPDomainsKey]);
}

- (void)testCDPDomainsIgnorePersistedValues {
  // Values saved by older versions must not restore enabled domains.
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  NSArray<NSString *> *persistedDomains = @[ SP_KEY_ENABLE_CDP_DOMAIN_DOM ];
  [defaults setObject:persistedDomains forKey:kDevToolSettingsTestActivatedCDPDomainsKey];
  DevToolSettings *settings = [[DevToolSettings alloc] init];

  XCTAssertEqual([settings enabledCDPDomains].count, 0U);
  [settings setCDPDomain:SP_KEY_ENABLE_CDP_DOMAIN_CSS enabled:YES];
  XCTAssertFalse([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_DOM]);
  XCTAssertEqualObjects(persistedDomains,
                        [defaults arrayForKey:kDevToolSettingsTestActivatedCDPDomainsKey]);

  // A new instance must start empty despite earlier updates.
  DevToolSettings *freshSettings = [[DevToolSettings alloc] init];
  XCTAssertEqual([freshSettings enabledCDPDomains].count, 0U);
}

- (void)testCDPDomainsUseSnapshotsAndReplacePreviousValues {
  DevToolSettings *settings = [DevToolSettings sharedInstance];
  NSMutableSet<NSString *> *domains = [NSMutableSet setWithObject:SP_KEY_ENABLE_CDP_DOMAIN_DOM];
  [settings setEnabledCDPDomains:domains];
  // Mutating the input set must not change settings.
  [domains removeAllObjects];
  XCTAssertTrue([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_DOM]);

  // Batch updates replace the previous set rather than merging it.
  [settings setEnabledCDPDomains:[NSSet setWithObject:SP_KEY_ENABLE_CDP_DOMAIN_CSS]];
  XCTAssertFalse([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_DOM]);
  XCTAssertTrue([settings isCDPDomainEnabled:SP_KEY_ENABLE_CDP_DOMAIN_CSS]);

  [settings setEnabledCDPDomains:[NSSet set]];
  XCTAssertEqual([settings enabledCDPDomains].count, 0U);
}

- (void)testCDPDomainsRejectInvalidKeysWithoutChangingState {
  DevToolSettings *settings = [DevToolSettings sharedInstance];
  NSSet<NSString *> *domains = [NSSet setWithObject:SP_KEY_ENABLE_CDP_DOMAIN_DOM];
  [settings setEnabledCDPDomains:domains];

  // One invalid key rejects the entire batch, including CSS.
  [settings setEnabledCDPDomains:[NSSet setWithObjects:SP_KEY_ENABLE_CDP_DOMAIN_CSS,
                                                       SP_KEY_ENABLE_DEVTOOL, nil]];
  // The single-domain setter must also reject non-CDP keys.
  [settings setCDPDomain:SP_KEY_ENABLE_DEVTOOL enabled:YES];
  XCTAssertFalse([settings isCDPDomainEnabled:SP_KEY_ENABLE_DEVTOOL]);
  XCTAssertEqualObjects(domains, [settings enabledCDPDomains]);
}

- (void)testBuildReportedBacktraceAddresses {
  NSArray<NSNumber *> *addresses = [DevToolSettings buildReportedBacktraceAddresses];

  XCTAssertNotNil(addresses);
  XCTAssertLessThanOrEqual(addresses.count, 10U);
}

- (void)testBuildBacktraceAddressSummary {
  NSString *summary = [DevToolSettings buildBacktraceAddressSummary];

  XCTAssertNotNil(summary);
  XCTAssertTrue(summary.length > 0);
}

@end
