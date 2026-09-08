// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// cspell:ignore containerid containertype explorersupportsexplicitrouteownership
// cspell:ignore explorersupportssparklingcontainer sparklingavailable
// cspell:ignore explorersparklingversion sparklingnavigation spkcontainerid spkpipe

#import "LynxViewShellViewController.h"
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxView.h>
#import "DemoGenericResourceFetcher.h"
#import "DemoMediaResourceFetcher.h"
#import "DemoTemplateResourceFetcher.h"
#import "LynxExplorerInput.h"
#import "LynxExplorerSwiftInterop.h"
#import "LynxSettingManager.h"
#import "UIHelper.h"

NSString *const kParamHiddenNav = @"hidden_nav";
NSString *const kParamFullScreen = @"fullscreen";
NSString *const kParamTitle = @"title";
NSString *const kParamTitleColor = @"title_color";
NSString *const kParamBarColor = @"bar_color";
NSString *const kParamBackButtonStyle = @"back_button_style";
NSString *const kBackButtonStyleLight = @"light";
NSString *const kBackButtonStyleDark = @"dark";
NSString *const kBackButtonImageLight = @"back_light";
NSString *const kBackButtonImageDark = @"back_dark";

@interface LynxViewShellViewController () <LynxViewLifecycle> {
  LynxExtraTiming *extraTiming;
}

@property(nonatomic, assign) BOOL fullScreen;
@property(nonatomic, copy) NSString *backButtonImageName;
@property(nonatomic, copy) NSString *navTitle;
@property(nonatomic, strong) UIColor *titleColor;
@property(nonatomic, strong) UIColor *barColor;
@property(nonatomic, strong) UIView *previousViewControllerView;
@property(nonatomic, strong) LXRouteCoordinator *interactiveBackCoordinator;
@property(nonatomic, assign) BOOL interactiveBackReserved;
@property(nonatomic, copy) NSString *frontendTheme;
@property(nonatomic, strong) LynxView *lynxView;
@property(nonatomic, strong) ExplorerLegacyLoadingView *loadingView;
@property(nonatomic, strong) UIView *statusView;
@property(nonatomic, strong) UIView *barView;
@property(nonatomic, strong) UILabel *titleLabel;
@property(nonatomic, assign) CGSize currentScreenMetricsSize;
@property(nonatomic, assign) CGSize currentViewportSize;
@property(nonatomic, assign) UIEdgeInsets currentSafeAreaInsets;

@end

@implementation LynxViewShellViewController

// Reserved-capability checks use the shared +[ExplorerReservedKeys
// isReservedCapabilityKey:] (Swift) so this boundary stays in lockstep with
// GlobalPropsMerger and LegacyContainerLauncher.

static NSString *LegacyGlobalPropKey(NSString *key) {
  NSUInteger leadingUnderline = 0;
  while (leadingUnderline < key.length && [key characterAtIndex:leadingUnderline] == '_') {
    leadingUnderline++;
  }
  NSString *trimmedKey = [key substringFromIndex:leadingUnderline];
  if (trimmedKey.length == 0) {
    return key;
  }

  NSArray<NSString *> *parts = [trimmedKey componentsSeparatedByString:@"_"];
  NSMutableString *propsKey = [NSMutableString stringWithString:parts[0]];
  for (NSUInteger index = 1; index < parts.count; index++) {
    NSString *part = parts[index];
    if (part.length == 0) {
      continue;
    }
    NSString *capitalizedPart =
        [part stringByReplacingCharactersInRange:NSMakeRange(0, 1)
                                      withString:[part substringToIndex:1].uppercaseString];
    [propsKey appendString:capitalizedPart];
  }
  return propsKey;
}

- (BOOL)hasExplicitViewportSize {
  return [self.params.allKeys containsObject:@"height"] &&
         [self.params.allKeys containsObject:@"width"];
}

- (CGSize)screenSizeForCurrentBounds {
  if ([self hasExplicitViewportSize]) {
    NSNumber *width = [self.params objectForKey:@"width"];    // Physical pixel
    NSNumber *height = [self.params objectForKey:@"height"];  // Physical pixel
    CGFloat realScale = [[UIScreen mainScreen] scale];
    return CGSizeMake([width intValue] / realScale, [height intValue] / realScale);
  }
  return self.view.bounds.size;
}

- (CGFloat)statusBarHeight {
  CGFloat statusBarHeight = [UIApplication sharedApplication].statusBarFrame.size.height;
  if (statusBarHeight > 0) {
    return statusBarHeight;
  }
  return [self statusBarHeightForCurrentLayout];
}

- (CGFloat)statusBarHeightForCurrentLayout {
  if (@available(iOS 13.0, *)) {
    UIWindowScene *windowScene = self.view.window.windowScene;
    CGFloat statusBarHeight = windowScene.statusBarManager.statusBarFrame.size.height;
    if (statusBarHeight > 0) {
      return statusBarHeight;
    }
  }
  CGFloat statusBarHeight = [UIApplication sharedApplication].statusBarFrame.size.height;
  if (statusBarHeight > 0) {
    return statusBarHeight;
  }
  return [self isNotchScreen] ? 44 : 20;
}

- (UIEdgeInsets)safeAreaInsetsForCurrentLayout {
  if (@available(iOS 11.0, *)) {
    UIEdgeInsets safeAreaInsets = self.view.safeAreaInsets;
    if (!UIEdgeInsetsEqualToEdgeInsets(safeAreaInsets, UIEdgeInsetsZero)) {
      return safeAreaInsets;
    }
    UIWindow *window = self.view.window;
    if (window) {
      safeAreaInsets = window.safeAreaInsets;
      if (!UIEdgeInsetsEqualToEdgeInsets(safeAreaInsets, UIEdgeInsetsZero)) {
        return safeAreaInsets;
      }
    }
  }
  return UIEdgeInsetsMake([self statusBarHeightForCurrentLayout], 0, [self isNotchScreen] ? 34 : 0,
                          0);
}

- (CGFloat)navigationBarHeight {
  return self.navigationController.navigationBar.frame.size.height;
}

- (CGRect)lynxViewFrameForScreenSize:(CGSize)screenSize {
  CGFloat y = 0;
  CGFloat height = screenSize.height;
  if (!self.fullScreen) {
    CGFloat statusBarHeight = [self statusBarHeight];
    y += statusBarHeight;
    height -= statusBarHeight;
    if (!self.hiddenNav) {
      CGFloat navigationBarHeight = [self navigationBarHeight];
      y += navigationBarHeight;
      height -= navigationBarHeight;
    }
  }
  return CGRectMake(0, y, screenSize.width, MAX(height, 0));
}

- (void)layoutNavigationForScreenSize:(CGSize)screenSize {
  CGFloat statusH = [self statusBarHeight];
  CGFloat navH = [self navigationBarHeight];

  self.statusView.frame = CGRectMake(0, 0, screenSize.width, statusH);
  self.barView.frame = CGRectMake(0, statusH, screenSize.width, navH);
  self.titleLabel.frame = CGRectMake(navH, 0, MAX(screenSize.width - 2 * navH, 0), navH);
}

- (void)updateLynxViewLayoutIfNeeded {
  if (!self.lynxView) {
    return;
  }

  CGSize screenSize = [self screenSizeForCurrentBounds];
  CGRect lynxViewFrame = [self lynxViewFrameForScreenSize:screenSize];
  CGSize viewportSize = lynxViewFrame.size;
  UIEdgeInsets safeAreaInsets = [self safeAreaInsetsForCurrentLayout];

  [self layoutNavigationForScreenSize:screenSize];
  self.lynxView.frame = lynxViewFrame;
  [self.loadingView updateFrame:lynxViewFrame];

  if (CGSizeEqualToSize(self.currentScreenMetricsSize, screenSize) &&
      CGSizeEqualToSize(self.currentViewportSize, viewportSize) &&
      UIEdgeInsetsEqualToEdgeInsets(self.currentSafeAreaInsets, safeAreaInsets)) {
    return;
  }

  self.currentScreenMetricsSize = screenSize;
  self.currentViewportSize = viewportSize;
  self.currentSafeAreaInsets = safeAreaInsets;
  [self.lynxView updateScreenMetricsWithWidth:screenSize.width height:screenSize.height];
  [self.lynxView updateViewportWithPreferredLayoutWidth:viewportSize.width
                                  preferredLayoutHeight:viewportSize.height];
  [self.lynxView updateGlobalPropsWithTemplateData:[self getGlobalPropsForScreenSize:screenSize]];
}

- (id)init {
  if (self = [super init]) {
    self.params = @{};
    self.commonGlobalProps = @{};
    self.pageGlobalProps = @{};
    self.hiddenNav = NO;
    self.fullScreen = NO;
    self.backButtonImageName = kBackButtonImageLight;
    self.navTitle = @"";
    self.titleColor = [UIColor blackColor];
    self.barColor = [UIColor whiteColor];
    self.frontendTheme = kBackButtonStyleLight;
  }

  return self;
}

- (instancetype)initWithLegacySourceURL:(NSString *)sourceURL
                             parameters:(NSDictionary<NSString *, id> *)parameters {
  self = [self init];
  if (self) {
    LocalBundleResult localResult =
        [DemoTemplateResourceFetcher readLocalBundleFromResource:sourceURL];
    if (localResult.isLocalScheme) {
      self.url = localResult.url;
      self.data = localResult.data;
    } else {
      self.url = sourceURL;
      self.data = nil;
    }
    self.params = [parameters copy];
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(explorerThemePreferenceDidChange:)
                                               name:@"ExplorerThemePreferenceDidChange"
                                             object:nil];
  extraTiming = [[LynxExtraTiming alloc] init];
  extraTiming.openTime = [[NSDate date] timeIntervalSince1970] * 1000;
  // Do any additional setup after loading the view.
  extraTiming.containerInitStart = [[NSDate date] timeIntervalSince1970] * 1000;
  [self parseParameters];
  [self initNavigation];
  extraTiming.containerInitEnd = [[NSDate date] timeIntervalSince1970] * 1000;
  extraTiming.prepareTemplateStart = [[NSDate date] timeIntervalSince1970] * 1000;
  extraTiming.prepareTemplateEnd = [[NSDate date] timeIntervalSince1970] * 1000;
  [self loadLynxViewWithUrl:self.url templateData:self.data];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)explorerThemePreferenceDidChange:(NSNotification *)notification {
  if (self.lynxView == nil) {
    return;
  }
  CGSize screenSize = self.currentScreenMetricsSize;
  if (CGSizeEqualToSize(screenSize, CGSizeZero)) {
    screenSize = UIScreen.mainScreen.bounds.size;
  }
  [self.lynxView updateGlobalPropsWithTemplateData:[self getGlobalPropsForScreenSize:screenSize]];
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [self.navigationController setNavigationBarHidden:YES animated:NO];
}

- (LynxTemplateData *)getGlobalPropsFromParams {
  NSMutableDictionary *params = [NSMutableDictionary dictionary];
  for (NSString *key in self.commonGlobalProps) {
    if (![ExplorerReservedKeys isReservedCapabilityKey:key]) {
      [params setObject:self.commonGlobalProps[key] forKey:key];
    }
  }
  for (NSString *key in self.params) {
    if ([ExplorerReservedKeys isReservedCapabilityKey:key]) {
      continue;
    }
    id value = self.params[key];
    [params setObject:value forKey:LegacyGlobalPropKey(key)];
  }
  for (NSString *key in self.pageGlobalProps) {
    if (![ExplorerReservedKeys isReservedCapabilityKey:key]) {
      [params setObject:self.pageGlobalProps[key] forKey:key];
    }
  }

  LynxTemplateData *globalProps = [[LynxTemplateData alloc] initWithDictionary:params];
  return globalProps;
}

- (LynxTemplateData *)initialTemplateData {
  NSDictionary<NSString *, id> *data = self.launchInitialData;
  if (!data) {
    data = @{@"mockData" : @"Hello Lynx Explorer"};
  }
  return [[LynxTemplateData alloc] initWithDictionary:data];
}

- (LynxTemplateData *)getGlobalPropsForScreenSize:(CGSize)screenSize {
  LynxTemplateData *globalProps = [self getGlobalPropsFromParams];
  [globalProps updateBool:[LXRouteCoordinator supportsExplicitRouteOwnership]
                   forKey:@"explorerSupportsExplicitRouteOwnership"];
  [globalProps updateBool:[LXRouteCoordinator supportsSparklingContainer]
                   forKey:@"explorerSupportsSparklingContainer"];
  NSString *preferredContainer =
      [[NSUserDefaults standardUserDefaults] stringForKey:@"preferredContainer"];
  if (preferredContainer.length == 0) {
    preferredContainer =
        [[NSUserDefaults standardUserDefaults] stringForKey:@"qrContainerPreference"];
  }
  if (![preferredContainer isEqualToString:@"sparkling"]) {
    preferredContainer = @"legacy";
  }
  [globalProps updateObject:preferredContainer forKey:@"explorerPreferredContainer"];
  NSString *sparklingVersion = [LXRouteCoordinator sparklingVersion];
  if (sparklingVersion.length > 0) {
    [globalProps updateObject:sparklingVersion forKey:@"explorerSparklingVersion"];
  }
  [globalProps updateBool:[self isNotchScreen] forKey:@"isNotchScreen"];
  [globalProps updateDouble:screenSize.height forKey:@"screenHeight"];
  [globalProps updateDouble:screenSize.width forKey:@"screenWidth"];
  UIEdgeInsets safeAreaInsets = [self safeAreaInsetsForCurrentLayout];
  [globalProps updateDouble:safeAreaInsets.top forKey:@"safeAreaTop"];
  [globalProps updateDouble:safeAreaInsets.bottom forKey:@"safeAreaBottom"];
  [globalProps updateDouble:safeAreaInsets.left forKey:@"safeAreaLeft"];
  [globalProps updateDouble:safeAreaInsets.right forKey:@"safeAreaRight"];

  NSString *theme = @"Light";
  if ([UIScreen mainScreen].traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark) {
    theme = @"Dark";
  }
  [globalProps updateObject:theme forKey:@"theme"];
  [globalProps updateObject:self.frontendTheme forKey:@"frontendTheme"];

  NSString *preferredTheme = [self getStorageItem:@"preferredTheme"];
  if (preferredTheme) {
    [globalProps updateObject:preferredTheme forKey:@"preferredTheme"];
  }
  return globalProps;
}

- (void)loadLynxViewWithUrl:(NSString *)url templateData:(NSData *)data {
  CGSize screenSize = [self screenSizeForCurrentBounds];
  CGRect lynxViewFrame = [self lynxViewFrameForScreenSize:screenSize];
  LynxThreadStrategyForRender threadStrategy =
      [LynxSettingManager sharedDataHandler].threadStrategy;

  LynxView *lynxView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.config =
        [[LynxConfig alloc] initWithProvider:[LynxEnv sharedInstance].config.templateProvider];
    builder.screenSize = screenSize;
    builder.fontScale = 1.0;
    builder.fetcher = nil;
    // for homepage only
    [builder.config registerUI:LynxExplorerInput.class withName:@"explorer-input"];
    // Add fetchers
    builder.enableGenericResourceFetcher = true;
    builder.genericResourceFetcher = [[DemoGenericResourceFetcher alloc] init];
    builder.templateResourceFetcher = [[DemoTemplateResourceFetcher alloc] init];
    builder.mediaResourceFetcher = [[DemoMediaResourceFetcher alloc] init];
    [builder setThreadStrategyForRender:threadStrategy];
  }];
  lynxView.preferredLayoutWidth = lynxViewFrame.size.width;
  [lynxView setExtraTiming:extraTiming];

  lynxView.preferredLayoutHeight = lynxViewFrame.size.height;
  lynxView.layoutWidthMode = LynxViewSizeModeExact;
  lynxView.layoutHeightMode = LynxViewSizeModeExact;
  lynxView.enableAutoLayout = YES;
  [self.view addSubview:lynxView];
  self.lynxView = lynxView;
  [lynxView addLifecycleClient:self];

  ExplorerLegacyLoadingView *loadingView =
      [[ExplorerLegacyLoadingView alloc] initWithParentView:self.view frame:lynxViewFrame];
  self.loadingView = loadingView;
  [loadingView showDownloading];

  [lynxView updateGlobalPropsWithTemplateData:[self getGlobalPropsForScreenSize:screenSize]];

  LynxTemplateData *initData = [self initialTemplateData];
  if (self.data) {
    [lynxView loadTemplate:data withURL:url initData:initData];
  } else {
    [lynxView loadTemplateFromURL:url initData:initData];
  }
  [lynxView triggerLayout];
  [self updateLynxViewLayoutIfNeeded];
}

- (void)lynxViewDidStartLoading:(LynxView *)view {
  // This callback fires before a remote template has finished downloading.
  // Keep the transfer stage visible until Lynx confirms the template load.
}

- (void)lynxView:(LynxView *)view didLoadFinishedWithUrl:(NSString *)url {
  [self.loadingView showRendering];
}

- (void)lynxViewDidFirstScreen:(LynxView *)view {
  [self.loadingView finish];
  self.loadingView = nil;
}

- (void)lynxView:(LynxView *)view didLoadFailedWithUrl:(NSString *)url error:(NSError *)error {
  [self.loadingView showError:error.localizedDescription];
}

- (void)parseParameters {
  NSArray *paramKeys = [self.params allKeys];
  if ([paramKeys containsObject:kParamHiddenNav]) {
    self.hiddenNav = [[self.params objectForKey:kParamHiddenNav] boolValue];
  }
  if ([paramKeys containsObject:kParamFullScreen]) {
    self.fullScreen = [[self.params objectForKey:kParamFullScreen] boolValue];
  }
  if ([paramKeys containsObject:kParamTitle]) {
    id title = [self.params objectForKey:kParamTitle];
    if ([title isKindOfClass:[NSString class]]) {
      // LaunchDescriptorParser owns the single percent-decoding boundary.
      self.navTitle = title;
    }
  }
  if ([paramKeys containsObject:kParamTitleColor]) {
    id titleColor = [self.params objectForKey:kParamTitleColor];
    if ([titleColor isKindOfClass:[NSString class]]) {
      self.titleColor = [UIHelper colorWithHexString:titleColor];
    }
  }
  if ([paramKeys containsObject:kParamBarColor]) {
    id barColor = [self.params objectForKey:kParamBarColor];
    if ([barColor isKindOfClass:[NSString class]]) {
      self.barColor = [UIHelper colorWithHexString:barColor];
    }
  }
  if ([paramKeys containsObject:kParamBackButtonStyle]) {
    id style = [self.params objectForKey:kParamBackButtonStyle];
    if ([style isKindOfClass:[NSString class]] && [style isEqualToString:kBackButtonStyleDark]) {
      self.backButtonImageName = kBackButtonImageDark;
      self.frontendTheme = kBackButtonStyleDark;
    }
  }

  if (self.fullScreen) {
    // fullScreen forces hiddenNav
    self.hiddenNav = YES;
  }
}

- (void)initNavigation {
  [self.navigationController setNavigationBarHidden:YES animated:NO];
  UIScreenEdgePanGestureRecognizer *edgePanGesture =
      [[UIScreenEdgePanGestureRecognizer alloc] initWithTarget:self
                                                        action:@selector(handleEdgePanGesture:)];
  edgePanGesture.edges = UIRectEdgeLeft;
  self.view.backgroundColor = self.barColor;
  [self.view addGestureRecognizer:edgePanGesture];
  if (self.fullScreen) {
    return;
  }

  CGSize screenSize = [UIScreen mainScreen].bounds.size;
  CGFloat statusH = [UIApplication sharedApplication].statusBarFrame.size.height;
  CGFloat navH = self.navigationController.navigationBar.frame.size.height;
  // create status view
  UIView *statusView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, screenSize.width, statusH)];
  statusView.backgroundColor = self.barColor;
  [self.view addSubview:statusView];
  self.statusView = statusView;

  if (self.hiddenNav) {
    return;
  }
  // create custom navigation bar
  UIView *barView = [[UIView alloc] initWithFrame:CGRectMake(0, statusH, screenSize.width, navH)];
  barView.backgroundColor = self.barColor;
  self.barView = barView;

  UIButton *goBackButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, navH, navH)];
  UIImage *backImage = [self scaleImage:[UIImage imageNamed:self.backButtonImageName]
                                   size:CGSizeMake(24, 24)];
  [goBackButton setImage:backImage forState:UIControlStateNormal];
  [goBackButton addTarget:self
                   action:@selector(backButtonTapped)
         forControlEvents:UIControlEventTouchUpInside];
  UILabel *titleLabel =
      [[UILabel alloc] initWithFrame:CGRectMake(navH, 0, screenSize.width - 2 * navH, navH)];
  titleLabel.text = self.navTitle;
  titleLabel.textColor = self.titleColor;
  titleLabel.textAlignment = NSTextAlignmentCenter;
  self.titleLabel = titleLabel;

  [barView addSubview:goBackButton];
  [barView addSubview:titleLabel];
  [self.view addSubview:barView];
}

- (BOOL)shouldAutorotate {
  return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
  if ([self.params.allKeys containsObject:@"orientation"]) {
    NSString *orientation = self.params[@"orientation"];
    if ([orientation isEqualToString:@"portrait"]) {
      return UIInterfaceOrientationMaskPortrait;
    } else if ([orientation isEqualToString:@"landscape"]) {
      return UIInterfaceOrientationMaskLandscape;
    }
  }
  return UIInterfaceOrientationMaskAll;
}

- (void)viewWillLayoutSubviews {
  [super viewWillLayoutSubviews];
  [self setNavigationStatus];
}

- (void)viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  [self updateLynxViewLayoutIfNeeded];
}

- (void)setNavigationStatus {
  [self.navigationController setNavigationBarHidden:YES animated:NO];
}

- (void)handleEdgePanGesture:(UIScreenEdgePanGestureRecognizer *)gesture {
  UIGestureRecognizerState state = gesture.state;
  CGPoint translation = [gesture translationInView:self.view];
  CGFloat progress = translation.x / self.view.bounds.size.width;
  progress = fminf(fmaxf(progress, 0.0), 1.0);

  switch (state) {
    case UIGestureRecognizerStateBegan: {
      if (self.navigationController.viewControllers.count > 1) {
        LXRouteCoordinator *coordinator = [LXRouteCoordinator currentBridge];
        if (coordinator == nil || ![coordinator beginInteractiveCloseFromHost]) {
          break;
        }
        self.interactiveBackCoordinator = coordinator;
        self.interactiveBackReserved = YES;
        UIViewController *previousVC = [self.navigationController.viewControllers
            objectAtIndex:self.navigationController.viewControllers.count - 2];
        self.previousViewControllerView = previousVC.view;
        [self.view.superview insertSubview:self.previousViewControllerView belowSubview:self.view];
        CGRect previousFrame = self.view.bounds;
        previousFrame.origin.x = -self.view.bounds.size.width * 0.3;
        self.previousViewControllerView.frame = previousFrame;
      }
      break;
    }
    case UIGestureRecognizerStateChanged: {
      if (self.interactiveBackReserved && self.previousViewControllerView && translation.x >= 0) {
        CGRect previousFrame = self.previousViewControllerView.frame;
        previousFrame.origin.x = -self.view.bounds.size.width * 0.3 * (1.0 - progress);
        self.previousViewControllerView.frame = previousFrame;

        CGRect currentFrame = self.view.frame;
        currentFrame.origin.x = translation.x;
        self.view.frame = currentFrame;
      }
      break;
    }
    case UIGestureRecognizerStateEnded: {
      if (self.interactiveBackReserved && self.previousViewControllerView) {
        // get velocity of gesture
        CGPoint velocity = [gesture velocityInView:self.view];
        CGFloat flingVelocity = 1000;

        if (velocity.x >= flingVelocity || progress > 0.5) {
          [UIView animateWithDuration:0.15
              animations:^{
                CGRect previousFrame = self.previousViewControllerView.frame;
                previousFrame.origin.x = 0;
                self.previousViewControllerView.frame = previousFrame;

                CGRect currentFrame = self.view.frame;
                currentFrame.origin.x = self.view.bounds.size.width;
                self.view.frame = currentFrame;
              }
              completion:^(BOOL finished) {
                if ([self.interactiveBackCoordinator commitInteractiveCloseFromHost]) {
                  self.interactiveBackReserved = NO;
                  self.interactiveBackCoordinator = nil;
                  self.previousViewControllerView = nil;
                } else {
                  [self restoreInteractiveBackStateAnimated:YES];
                }
              }];
        } else {
          [self restoreInteractiveBackStateAnimated:YES];
        }
      } else if (self.interactiveBackReserved) {
        [self restoreInteractiveBackStateAnimated:YES];
      }
      break;
    }
    case UIGestureRecognizerStateCancelled:
    case UIGestureRecognizerStateFailed:
      [self restoreInteractiveBackStateAnimated:YES];
      break;
    default:
      break;
  }
}

- (void)backButtonTapped {
  LXRouteCoordinator *coordinator = [LXRouteCoordinator currentBridge];
  [coordinator closeAnimated:YES
                    callback:^(__unused id payload){
                    }];
}

- (void)restoreInteractiveBackStateAnimated:(BOOL)animated {
  if (self.interactiveBackReserved) {
    [self.interactiveBackCoordinator endInteractiveCloseFromHostCompleted:NO];
    self.interactiveBackReserved = NO;
    self.interactiveBackCoordinator = nil;
  }
  if (self.previousViewControllerView == nil) {
    return;
  }
  void (^animations)(void) = ^{
    CGRect previousFrame = self.previousViewControllerView.frame;
    previousFrame.origin.x = -self.view.bounds.size.width * 0.3;
    self.previousViewControllerView.frame = previousFrame;

    CGRect currentFrame = self.view.frame;
    currentFrame.origin.x = 0;
    self.view.frame = currentFrame;
  };
  void (^completion)(BOOL) = ^(__unused BOOL finished) {
    [self.previousViewControllerView removeFromSuperview];
    self.previousViewControllerView = nil;
  };
  if (animated) {
    [UIView animateWithDuration:0.15 animations:animations completion:completion];
  } else {
    animations();
    completion(YES);
  }
}

- (UIImage *)scaleImage:(UIImage *)image size:(CGSize)size {
  UIGraphicsBeginImageContextWithOptions(size, NO, 0.0);
  [image drawInRect:CGRectMake(0, 0, size.width, size.height)];
  UIImage *newImage = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  return newImage;
}

- (BOOL)isNotchScreen {
  if (@available(iOS 11.0, *)) {
    UIEdgeInsets safeAreaInsets = self.view.window.safeAreaInsets;
    if (UIEdgeInsetsEqualToEdgeInsets(safeAreaInsets, UIEdgeInsetsZero)) {
      safeAreaInsets = self.view.safeAreaInsets;
    }
    return safeAreaInsets.top > 20 || safeAreaInsets.bottom > 0 || safeAreaInsets.left > 0 ||
           safeAreaInsets.right > 0;
  }

  return NO;
}

- (NSString *)getStorageItem:(NSString *)key {
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  return [defaults objectForKey:key];
}

@end
