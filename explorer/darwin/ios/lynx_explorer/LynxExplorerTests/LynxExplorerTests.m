// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateData.h>
#import <XCTest/XCTest.h>
#import "ExplorerLynxTestModule.h"
#import "LynxNodeAPIModule.h"
#import "LynxViewShellViewController.h"
#import "ScanViewController.h"

@interface LynxViewShellViewController (GlobalPropsTesting)

- (LynxTemplateData *)getGlobalPropsFromParams;
- (LynxTemplateData *)getGlobalPropsForScreenSize:(CGSize)screenSize;
- (LynxTemplateData *)initialTemplateData;
- (void)parseParameters;

@end

@interface LXScanCaptureSessionSpy : NSObject

@property(nonatomic, assign, getter=isRunning) BOOL running;
@property(nonatomic, assign) NSUInteger startRunningCount;

@end

@implementation LXScanCaptureSessionSpy

- (void)startRunning {
  self.running = YES;
  self.startRunningCount += 1;
}

@end

@interface LXScanRouteCoordinatorSpy : NSObject

@property(nonatomic, assign) NSUInteger closeCount;
@property(nonatomic, assign) BOOL lastAnimated;

@end

@implementation LXScanRouteCoordinatorSpy

- (void)closeAnimated:(BOOL)animated callback:(void (^)(id payload))callback {
  self.closeCount += 1;
  self.lastAnimated = animated;
  callback(@{
    @"success" : @NO,
    @"code" : @"cannot_close_root",
    @"message" : @"",
  });
}

@end

@interface ScanViewController (BackButtonTesting)

- (id)routeCoordinator;
- (void)backButtonTapped;

@end

@interface LXBackButtonScanViewController : ScanViewController

@property(nonatomic, strong) LXScanRouteCoordinatorSpy *routeCoordinatorSpy;

@end

@implementation LXBackButtonScanViewController

- (id)routeCoordinator {
  return self.routeCoordinatorSpy;
}

@end

@interface LynxExplorerTests : XCTestCase

@end

@interface LXContextModuleViewSpy : NSObject
@property(nonatomic, copy) NSDictionary *data;
@property(nonatomic, copy) NSDictionary *props;
@end

@implementation LXContextModuleViewSpy
- (void)updateDataWithDictionary:(NSDictionary *)data {
  NSMutableDictionary *merged = [self.data mutableCopy] ?: [NSMutableDictionary dictionary];
  [merged addEntriesFromDictionary:data];
  self.data = merged;
}
- (void)updateGlobalPropsWithDictionary:(NSDictionary *)props {
  self.props = props;
}
- (NSDictionary *)getPageDataByKey:(NSArray *)keys {
  NSMutableDictionary *result = [NSMutableDictionary dictionary];
  for (NSString *key in keys) {
    if (self.data[key]) {
      result[key] = self.data[key];
    }
  }
  return result;
}
@end

@interface LXContextModuleContextSpy : NSObject
@property(nonatomic, strong) LXContextModuleViewSpy *view;
@property(nonatomic, assign) BOOL hasLynxViewDestroyed;
@property(nonatomic, copy) NSString *event;
@property(nonatomic, copy) NSArray *params;
@end

@implementation LXContextModuleContextSpy
- (LynxView *)getLynxView {
  return (LynxView *)self.view;
}
- (void)sendGlobalEvent:(NSString *)event withParams:(NSArray *)params {
  self.event = event;
  self.params = params;
}
@end

@implementation LynxExplorerTests

- (void)testLynxTestModuleKeepsDataAndEventsOnItsOwnPage {
  XCTestExpectation *checked = [self expectationWithDescription:@"page-scoped operations"];
  dispatch_async(dispatch_get_main_queue(), ^{
    LXContextModuleContextSpy *first = [LXContextModuleContextSpy new];
    first.view = [LXContextModuleViewSpy new];
    LXContextModuleContextSpy *second = [LXContextModuleContextSpy new];
    second.view = [LXContextModuleViewSpy new];
    ExplorerLynxTestModule *firstModule =
        [[ExplorerLynxTestModule alloc] initWithLynxContext:(LynxContext *)first];
    ExplorerLynxTestModule *secondModule =
        [[ExplorerLynxTestModule alloc] initWithLynxContext:(LynxContext *)second];
    [firstModule updateData:@{@"first" : @1}];
    [firstModule updateData:@{@"next" : @2}];
    [secondModule updateData:@{@"second" : @3}];
    [firstModule updateGlobalProps:@{@"theme" : @"dark"}];
    XCTAssertEqualObjects(first.view.data, (@{@"first" : @1, @"next" : @2}));
    XCTAssertEqualObjects(second.view.data, (@{@"second" : @3}));
    XCTAssertNil(second.view.props);
    __block NSUInteger callbacks = 0;
    [firstModule getPageDataByKey:@[ @"next" ]
                         callback:^(id result) {
                           callbacks++;
                           XCTAssertEqualObjects(result, (@{@"next" : @2}));
                         }];
    XCTAssertEqual(callbacks, 1U);
    [firstModule eventTest:@"value"];
    XCTAssertEqualObjects(first.event, @"test");
    XCTAssertEqualObjects(first.params, (@[ @10, @"value" ]));
    XCTAssertNil(second.event);
    [firstModule valueTest:@"{\"answer\":42}"];
    XCTAssertEqualObjects(first.params, (@[ @{@"answer" : @42} ]));
    [firstModule valueTest:@"not JSON"];
    XCTAssertEqualObjects(first.params, (@[ @"not JSON" ]));
    [firstModule destroy];
    [firstModule updateData:@{@"afterDestroy" : @YES}];
    [firstModule eventTest:@"afterDestroy"];
    XCTAssertNil(first.view.data[@"afterDestroy"]);
    XCTAssertEqualObjects(first.params, (@[ @"not JSON" ]));
    second.hasLynxViewDestroyed = YES;
    [secondModule updateData:@{@"afterDestroy" : @YES}];
    XCTAssertNil(second.view.data[@"afterDestroy"]);
    [checked fulfill];
  });
  [self waitForExpectations:@[ checked ] timeout:2];
}

- (void)testLynxTestModuleExposesContextMethodsUnderNewName {
  XCTAssertEqualObjects(ExplorerLynxTestModule.name, @"LynxTestModule");
  XCTAssertTrue([ExplorerLynxTestModule conformsToProtocol:@protocol(LynxContextModule)]);
  NSDictionary<NSString *, NSString *> *methods = ExplorerLynxTestModule.methodLookup;
  XCTAssertEqualObjects(
      [NSSet setWithArray:methods.allKeys], ([NSSet setWithArray:@[
        @"eventTest", @"valueTest", @"back", @"reload", @"call", @"invoke", @"callSync",
        @"updateData", @"resetData", @"updateGlobalProps", @"reloadTemplate", @"getPageDataByKey",
        @"updateScreenMatrix", @"addButton", @"setDefaultValueForSetting"
      ]]));
  for (NSString *selector in methods.allValues) {
    XCTAssertTrue(
        [ExplorerLynxTestModule instancesRespondToSelector:NSSelectorFromString(selector)]);
  }
  LXContextModuleContextSpy *context = [LXContextModuleContextSpy new];
  ExplorerLynxTestModule *module =
      [[ExplorerLynxTestModule alloc] initWithLynxContext:(LynxContext *)context];
  XCTAssertEqualObjects([module call:@"test" params:@{}], (@{@"cb_data" : @"5555"}));
}

- (void)testShellViewControllerAllowsAutorotation {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];
  XCTAssertTrue([shellVC shouldAutorotate]);
}

- (void)testShellViewControllerSupportsLandscapeByDefault {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];
  shellVC.params = [NSMutableDictionary dictionary];
  XCTAssertEqual([shellVC supportedInterfaceOrientations], UIInterfaceOrientationMaskAll);
}

- (void)testShellViewControllerKeepsExplicitOrientationRestrictions {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];

  shellVC.params = [@{@"orientation" : @"portrait"} mutableCopy];
  XCTAssertEqual([shellVC supportedInterfaceOrientations], UIInterfaceOrientationMaskPortrait);

  shellVC.params = [@{@"orientation" : @"landscape"} mutableCopy];
  XCTAssertEqual([shellVC supportedInterfaceOrientations], UIInterfaceOrientationMaskLandscape);
}

- (void)testShellViewControllerUsesHostOwnedExplicitRouteCapability {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];
  shellVC.params = @{
    @"explorerSupportsExplicitRouteOwnership" : @"caller-value",
    @"explorer_supports_explicit_route_ownership" : @"caller-value",
    @"explorerSupportsSparklingContainer" : @"caller-value",
    @"explorer_supports_sparkling_container" : @"caller-value",
  };

  LynxTemplateData *globalProps = [shellVC getGlobalPropsForScreenSize:CGSizeMake(390, 844)];

  XCTAssertEqualObjects(globalProps.dictionary[@"explorerSupportsExplicitRouteOwnership"], @YES);
  id sparklingCapability = globalProps.dictionary[@"explorerSupportsSparklingContainer"];
  XCTAssertTrue([sparklingCapability isKindOfClass:NSNumber.class]);
}

- (void)testShellSemanticGlobalPropsUseDocumentedLayerPrecedence {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];
  shellVC.commonGlobalProps = @{
    @"theme" : @"common",
    @"commonOnly" : @"common-value",
  };
  shellVC.params = @{
    @"theme" : @"parameter",
    @"parameter_only" : @"parameter-value",
  };
  shellVC.pageGlobalProps = @{
    @"theme" : @"page",
    @"pageOnly" : @"page-value",
  };

  NSDictionary *globalProps = [shellVC getGlobalPropsFromParams].dictionary;

  XCTAssertEqualObjects(globalProps[@"theme"], @"page");
  XCTAssertEqualObjects(globalProps[@"commonOnly"], @"common-value");
  XCTAssertEqualObjects(globalProps[@"parameterOnly"], @"parameter-value");
  XCTAssertEqualObjects(globalProps[@"pageOnly"], @"page-value");
}

- (void)testShellFiltersReservedCapabilityAliasesFromEveryUntrustedLayer {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];
  shellVC.commonGlobalProps = @{
    @"container_type" : @"sparkling",
    @"spk-pipe" : @"fake",
  };
  shellVC.params = @{
    @"sparkling_navigation" : @YES,
    @"spk_container_id" : @"fake",
  };
  shellVC.pageGlobalProps = @{
    @"sparklingAvailable" : @YES,
    @"explorer_supports_explicit_route_ownership" : @NO,
    @"explorer_supports_sparkling_container" : @NO,
  };

  NSDictionary *globalProps = [shellVC getGlobalPropsFromParams].dictionary;

  XCTAssertNil(globalProps[@"containerType"]);
  XCTAssertNil(globalProps[@"spkPipe"]);
  XCTAssertNil(globalProps[@"spkContainerId"]);
  XCTAssertNil(globalProps[@"sparklingNavigation"]);
  XCTAssertNil(globalProps[@"sparklingAvailable"]);
  XCTAssertNil(globalProps[@"explorerSupportsExplicitRouteOwnership"]);
  XCTAssertNil(globalProps[@"explorerSupportsSparklingContainer"]);
}

- (void)testShellUsesSemanticInitialDataAndPreservesRawLegacyMockFallback {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];

  XCTAssertEqualObjects([shellVC initialTemplateData].dictionary[@"mockData"],
                        @"Hello Lynx Explorer");

  shellVC.launchInitialData = @{@"answer" : @42};
  NSDictionary *semanticInitialData = [shellVC initialTemplateData].dictionary;
  XCTAssertEqualObjects(semanticInitialData[@"answer"], @42);
  XCTAssertNil(semanticInitialData[@"mockData"]);
}

- (void)testShellDoesNotPercentDecodeSemanticTitleTwice {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];
  // LaunchDescriptorParser has already decoded title=%252F once at this edge.
  shellVC.params = @{@"title" : @"%2F"};

  [shellVC parseParameters];

  XCTAssertEqualObjects([shellVC valueForKey:@"navTitle"], @"%2F");
}

- (void)testNodeAPIModuleUsesRegisteredModuleParameterAsToken {
  NSObject *token = [[NSObject alloc] init];

  LynxNodeAPIModule *module = [[LynxNodeAPIModule alloc] initWithParam:token];

  XCTAssertEqual([module valueForKey:@"token"], token);
}

- (void)testDismissedContainerChoiceResetsPendingScanAndResumesCapture {
  ScanViewController *scanViewController = [[ScanViewController alloc] init];
  [scanViewController loadViewIfNeeded];
  LXScanCaptureSessionSpy *captureSession = [[LXScanCaptureSessionSpy alloc] init];
  CALayer *captureLayer = [CALayer layer];
  [scanViewController setValue:captureSession forKey:@"captureSession"];
  [scanViewController setValue:captureLayer forKey:@"captureLayer"];
  [scanViewController setValue:@YES forKey:@"presentingContainerChoice"];
  [scanViewController setValue:@"https://example.com/page.lynx.bundle" forKey:@"pendingScanResult"];

  SEL dismissalSelector = @selector(presentationControllerDidDismiss:);
  XCTAssertTrue([scanViewController respondsToSelector:dismissalSelector]);
  XCTAssertTrue([scanViewController
      respondsToSelector:NSSelectorFromString(@"popoverPresentationControllerDidDismissPopover:")]);
  if ([scanViewController respondsToSelector:dismissalSelector]) {
    UIViewController *presentedViewController = [[UIViewController alloc] init];
    UIPresentationController *presentationController =
        [[UIPresentationController alloc] initWithPresentedViewController:presentedViewController
                                                 presentingViewController:scanViewController];
    [(id<UIAdaptivePresentationControllerDelegate>)scanViewController
        presentationControllerDidDismiss:presentationController];
  }

  XCTAssertFalse([[scanViewController valueForKey:@"presentingContainerChoice"] boolValue]);
  XCTAssertNil([scanViewController valueForKey:@"pendingScanResult"]);
  XCTAssertEqual(captureSession.startRunningCount, 1U);
  XCTAssertEqual(captureLayer.superlayer, scanViewController.view.layer);
}

- (void)testScannerBackButtonDelegatesToCoordinatorAndDoesNotPopOnFailure {
  UIViewController *rootViewController = [[UIViewController alloc] init];
  LXBackButtonScanViewController *scanViewController =
      [[LXBackButtonScanViewController alloc] init];
  LXScanRouteCoordinatorSpy *coordinator = [[LXScanRouteCoordinatorSpy alloc] init];
  scanViewController.routeCoordinatorSpy = coordinator;
  UINavigationController *navigationController =
      [[UINavigationController alloc] initWithRootViewController:rootViewController];
  [navigationController pushViewController:scanViewController animated:NO];

  [scanViewController loadViewIfNeeded];
  UIBarButtonItem *backButton = scanViewController.navigationItem.leftBarButtonItem;

  XCTAssertNotNil(backButton);
  XCTAssertEqual(backButton.target, scanViewController);
  XCTAssertEqual(backButton.action, @selector(backButtonTapped));

  [scanViewController backButtonTapped];

  XCTAssertEqual(coordinator.closeCount, 1U);
  XCTAssertTrue(coordinator.lastAnimated);
  XCTAssertEqual(navigationController.viewControllers.count, 2U);
  XCTAssertEqual(navigationController.topViewController, scanViewController);
}

@end
