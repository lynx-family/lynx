// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxViewShellViewController.h"
#if __has_include("Sparkling-umbrella.h")
#import "Sparkling-umbrella.h"
#define HAS_SPARKLING 1
#endif
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxView+Identify.h>
#import <Lynx/LynxView.h>
#import "DemoGenericResourceFetcher.h"
#import "DemoMediaResourceFetcher.h"
#import "DemoTemplateResourceFetcher.h"
#import "LynxExplorer-Swift.h"
#import "LynxExplorerInput.h"
#import "LynxSettingManager.h"
#import "UIHelper.h"

const NSString *const kParamHiddenNav = @"hidden_nav";
const NSString *const kParamFullScreen = @"fullscreen";
const NSString *const kParamTitle = @"title";
const NSString *const kParamTitleColor = @"title_color";
const NSString *const kParamBarColor = @"bar_color";
const NSString *const kParamBackButtonStyle = @"back_button_style";
NSString *const kBackButtonStyleLight = @"light";
NSString *const kBackButtonStyleDark = @"dark";
NSString *const kBackButtonImageLight = @"back_light";
NSString *const kBackButtonImageDark = @"back_dark";

@interface LynxViewShellViewController () {
  LynxExtraTiming *extraTiming;
}

@property(nonatomic, assign) BOOL fullScreen;
@property(nonatomic, copy) NSString *backButtonImageName;
@property(nonatomic, copy) NSString *navTitle;
@property(nonatomic, strong) UIColor *titleColor;
@property(nonatomic, strong) UIColor *barColor;
@property(nonatomic, strong) UIView *previousViewControllerView;
@property(nonatomic, copy) NSString *frontendTheme;

@end

@implementation LynxViewShellViewController

- (id)init {
  if (self = [super init]) {
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

- (void)viewDidLoad {
  [super viewDidLoad];
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

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [self.navigationController setNavigationBarHidden:YES animated:NO];
}

- (LynxTemplateData *)getGlobalPropsFromParams {
  NSMutableDictionary *params = [NSMutableDictionary dictionary];
  for (NSString *key in self.params) {
    id value = self.params[key];

    // 1. remove redundant '_'
    NSUInteger leadingUnderline = 0;
    while (leadingUnderline < key.length && [key characterAtIndex:leadingUnderline] == '_') {
      leadingUnderline++;
    }
    NSString *trimmedKey = [key substringFromIndex:leadingUnderline];
    if (trimmedKey.length == 0) {
      [params setObject:value forKey:key];
      continue;
    }

    // 2. split by underscores and convert to camel case
    NSArray<NSString *> *parts = [trimmedKey componentsSeparatedByString:@"_"];
    NSMutableString *propsKey = [NSMutableString stringWithString:parts[0]];

    for (NSUInteger i = 1; i < parts.count; i++) {
      NSString *part = parts[i];
      if (part.length == 0) {
        continue;
      }
      NSString *capitalizedPart =
          [part stringByReplacingCharactersInRange:NSMakeRange(0, 1)
                                        withString:[part substringToIndex:1].uppercaseString];
      [propsKey appendString:capitalizedPart];
    }
    [params setObject:value forKey:propsKey];
  }

  LynxTemplateData *globalProps = [[LynxTemplateData alloc] initWithDictionary:params];
  return globalProps;
}

- (void)loadLynxViewWithUrl:(NSString *)url templateData:(NSData *)data {
  CGRect screenFrame = self.view.frame;
  CGRect statusRect = [[UIApplication sharedApplication] statusBarFrame];
  CGRect navRect = self.navigationController.navigationBar.frame;

  // Specify LynxView width and height according to the query parameters.
  CGSize screenSize = CGSizeZero;
  if ([[_params allKeys] containsObject:@"height"] && [[_params allKeys] containsObject:@"width"]) {
    NSNumber *width = [_params objectForKey:@"width"];    // Physical pixel
    NSNumber *height = [_params objectForKey:@"height"];  // Physical pixel
    CGFloat realScale = [[UIScreen mainScreen] scale];
    screenSize = CGSizeMake([width intValue] / realScale, [height intValue] / realScale);
  } else {
    screenSize = screenFrame.size;
  }
  LynxThreadStrategyForRender threadStrategy =
      [LynxSettingManager sharedDataHandler].threadStrategy;

#if HAS_SPARKLING
  NSString *containerID = [[NSUUID UUID] UUIDString];
#endif
  LynxView *lynxView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.config =
        [[LynxConfig alloc] initWithProvider:[LynxEnv sharedInstance].config.templateProvider];
    builder.screenSize = screenSize;
    builder.fontScale = 1.0;
    builder.fetcher = nil;
    // for homepage only
    [builder.config registerUI:LynxExplorerInput.class withName:@"explorer-input"];
#if HAS_SPARKLING
    // Register Sparkling spkPipe module with pre-generated containerID
    [SPKServiceRegistrar setupLynxPipeWithConfig:builder.config containerID:containerID];
#endif
    // Add fetchers
    builder.enableGenericResourceFetcher = true;
    builder.genericResourceFetcher = [[DemoGenericResourceFetcher alloc] init];
    builder.templateResourceFetcher = [[DemoTemplateResourceFetcher alloc] init];
    builder.mediaResourceFetcher = [[DemoMediaResourceFetcher alloc] init];
    [builder setThreadStrategyForRender:threadStrategy];
  }];
#if HAS_SPARKLING
  lynxView.containerID = containerID;
#endif
  lynxView.preferredLayoutWidth = screenSize.width;
  [lynxView setExtraTiming:extraTiming];
#if HAS_SPARKLING
  // Connect Sparkling MethodPipe execution engine to this LynxView
  [SPKServiceRegistrar connectPipeTo:lynxView];
#endif

  if (self.fullScreen) {
    lynxView.preferredLayoutHeight = screenSize.height;
  } else if (self.hiddenNav) {
    lynxView.preferredLayoutHeight = screenSize.height - statusRect.size.height;
  } else {
    lynxView.preferredLayoutHeight =
        screenSize.height - statusRect.size.height - navRect.size.height;
  }
  lynxView.layoutWidthMode = LynxViewSizeModeExact;
  lynxView.layoutHeightMode = LynxViewSizeModeExact;
  [self.view addSubview:lynxView];

  CGRect screenRect = [[UIScreen mainScreen] bounds];
  CGFloat screenWidth = screenRect.size.width;
  CGFloat screenHeight = screenRect.size.height;
  LynxTemplateData *globalProps = [self getGlobalPropsFromParams];
  [globalProps updateBool:[self isNotchScreen] forKey:@"isNotchScreen"];
  [globalProps updateDouble:screenHeight forKey:@"screenHeight"];
  [globalProps updateDouble:screenWidth forKey:@"screenWidth"];
  if (@available(iOS 11.0, *)) {
    UIWindow *window = UIApplication.sharedApplication.keyWindow;
    UIEdgeInsets safeAreaInsets = window.safeAreaInsets;
    [globalProps updateDouble:safeAreaInsets.top forKey:@"safeAreaTop"];
    [globalProps updateDouble:safeAreaInsets.bottom forKey:@"safeAreaBottom"];
  } else {
    [globalProps updateDouble:0 forKey:@"safeAreaTop"];
    [globalProps updateDouble:0 forKey:@"safeAreaBottom"];
  }
  NSString *theme = @"Light";
  if ([UIScreen mainScreen].traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark) {
    theme = @"Dark";
  }
  [globalProps updateObject:theme forKey:@"theme"];
  [globalProps updateObject:self.frontendTheme forKey:@"frontendTheme"];

  // Add the preferred theme from user defaults
  NSString *preferredTheme = [self getStorageItem:@"preferredTheme"];
  if (preferredTheme) {
    [globalProps updateObject:preferredTheme forKey:@"preferredTheme"];
  }

  [lynxView updateGlobalPropsWithTemplateData:globalProps];

  LynxTemplateData *initData =
      [[LynxTemplateData alloc] initWithDictionary:@{@"mockData" : @"Hello Lynx Explorer"}];
  if (self.data) {
    [lynxView loadTemplate:data withURL:url initData:initData];
  } else {
    [lynxView loadTemplateFromURL:url initData:initData];
  }
  [lynxView triggerLayout];

  CGRect lynxViewFrame;
  if (self.fullScreen) {
    lynxViewFrame =
        CGRectMake(0, 0, lynxView.intrinsicContentSize.width, lynxView.intrinsicContentSize.height);
  } else if (self.hiddenNav) {
    lynxViewFrame = CGRectMake(0, statusRect.size.height, lynxView.intrinsicContentSize.width,
                               lynxView.intrinsicContentSize.height);
  } else {
    lynxViewFrame =
        CGRectMake(0, statusRect.size.height + navRect.size.height,
                   lynxView.intrinsicContentSize.width, lynxView.intrinsicContentSize.height);
  }
  lynxView.frame = lynxViewFrame;
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
      self.navTitle = [title stringByRemovingPercentEncoding];
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

  if (self.hiddenNav) {
    return;
  }
  // create custom navigation bar
  UIView *barView = [[UIView alloc] initWithFrame:CGRectMake(0, statusH, screenSize.width, navH)];
  barView.backgroundColor = self.barColor;

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

  [barView addSubview:goBackButton];
  [barView addSubview:titleLabel];
  [self.view addSubview:barView];
}

- (BOOL)shouldAutorotate {
  return NO;
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
  return UIInterfaceOrientationMaskAllButUpsideDown;
}

- (void)viewWillLayoutSubviews {
  [super viewWillLayoutSubviews];
  [self setNavigationStatus];
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
    case UIGestureRecognizerStateBegan:
      if (self.navigationController.viewControllers.count > 1) {
        UIViewController *previousVC = [self.navigationController.viewControllers
            objectAtIndex:self.navigationController.viewControllers.count - 2];
        self.previousViewControllerView = previousVC.view;
        [self.view.superview insertSubview:self.previousViewControllerView belowSubview:self.view];
        self.previousViewControllerView.frame = CGRectMake(0, 0, 0, self.view.bounds.size.height);
      }
      break;
    case UIGestureRecognizerStateChanged: {
      if (self.previousViewControllerView && translation.x >= 0) {
        CGRect previousFrame = self.previousViewControllerView.frame;
        previousFrame.size.width = self.view.bounds.size.width * progress;
        self.previousViewControllerView.frame = previousFrame;

        CGRect currentFrame = self.view.frame;
        currentFrame.origin.x = translation.x;
        self.view.frame = currentFrame;
      }
      break;
    }
    case UIGestureRecognizerStateEnded: {
      if (self.previousViewControllerView) {
        // get velocity of gesture
        CGPoint velocity = [gesture velocityInView:self.view];
        CGFloat flingVelocity = 1000;

        if (velocity.x >= flingVelocity || progress > 0.5) {
          [UIView animateWithDuration:0.15
              animations:^{
                CGRect previousFrame = self.previousViewControllerView.frame;
                previousFrame.size.width = self.view.bounds.size.width;
                self.previousViewControllerView.frame = previousFrame;

                CGRect currentFrame = self.view.frame;
                currentFrame.origin.x = self.view.bounds.size.width;
                self.view.frame = currentFrame;
              }
              completion:^(BOOL finished) {
                [self.navigationController popViewControllerAnimated:NO];
              }];
        } else {
          [UIView animateWithDuration:0.15
              animations:^{
                CGRect previousFrame = self.previousViewControllerView.frame;
                previousFrame.size.width = 0;
                self.previousViewControllerView.frame = previousFrame;

                CGRect currentFrame = self.view.frame;
                currentFrame.origin.x = 0;
                self.view.frame = currentFrame;
              }
              completion:^(BOOL finished) {
                [self.previousViewControllerView removeFromSuperview];
                self.previousViewControllerView = nil;
              }];
        }
      }
      break;
    }
    default:
      break;
  }
}

- (void)backButtonTapped {
  [self.navigationController popViewControllerAnimated:YES];
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
    UIWindow *window = UIApplication.sharedApplication.keyWindow;
    UIEdgeInsets safeAreaInsets = window.safeAreaInsets;
    return safeAreaInsets.top > 20;
  }

  return NO;
}

- (NSString *)getStorageItem:(NSString *)key {
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  return [defaults objectForKey:key];
}

@end
