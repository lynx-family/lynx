// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolOverlayDelegate.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <Lynx/OverlayService.h>
#import <XElement/LynxOverlayGlobalManager.h>
/**
 * Implement LynxOverlayGlobalContainer to customize hitTest
 */
@interface LynxOverlayGlobalContainer : UIView

@end

@implementation LynxOverlayGlobalContainer

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  NSEnumerator *enumerator = [self.subviews reverseObjectEnumerator];
  UIView *subview = nil;

  // enumerate subviews reversely to dispatch hitTest from upper level to lower level
  while (subview = [enumerator nextObject]) {
    if (!subview.hidden) {
      UIView *view = [subview hitTest:point withEvent:event];
      if (view) {
        return view;
      }
    }
  }
  return nil;
}

@end

@interface LynxOverlayServiceImpl : NSObject <OverlayService>

@end

@implementation LynxOverlayServiceImpl

- (NSArray<NSNumber *> *)getAllVisibleOverlaySign {
  return [LynxOverlayGlobalManager getAllVisibleOverlay];
}

@end

@interface LynxOverlayGlobalManager ()

@property(nonatomic, strong)
    NSMutableDictionary<NSNumber *, NSMutableDictionary<NSNumber *, UIView *> *> *levelContainers;

@end

@implementation LynxOverlayGlobalManager

- (instancetype)init {
  self = [super init];
  [DevToolOverlayDelegate.sharedInstance initWithService:[[LynxOverlayServiceImpl alloc] init]];
  return self;
}

+ (instancetype)sharedInstance {
  static LynxOverlayGlobalManager *instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[LynxOverlayGlobalManager alloc] init];
    instance.levelContainers = [NSMutableDictionary dictionary];
  });
  return instance;
}

+ (NSMutableArray *)getAllVisibleOverlay {
  NSMutableArray *array = [NSMutableArray array];
  NSMutableDictionary<NSNumber *, NSMutableDictionary<NSNumber *, UIView *> *> *levelContainers =
      LynxOverlayGlobalManager.sharedInstance.levelContainers;
  for (id key in levelContainers) {
    NSMutableDictionary *allLevelContainerAtTheMode = levelContainers[key];
    NSArray *sortKey =
        [[allLevelContainerAtTheMode allKeys] sortedArrayUsingComparator:^NSComparisonResult(
                                                  id _Nonnull levelLeft, id _Nonnull levelRight) {
          NSInteger integerLevelLeft = [levelLeft integerValue];
          NSInteger integerLevelRight = [levelRight integerValue];

          if (integerLevelLeft > integerLevelRight) {
            return (NSComparisonResult)NSOrderedAscending;
          }
          if (integerLevelLeft < integerLevelRight) {
            return (NSComparisonResult)NSOrderedDescending;
          }
          return (NSComparisonResult)NSOrderedSame;
        }];
    [sortKey enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
      UIView *levelContainer = allLevelContainerAtTheMode[obj];
      [levelContainer.subviews
          enumerateObjectsUsingBlock:^(__kindof LynxOverlayContainer *_Nonnull obj, NSUInteger idx,
                                       BOOL *_Nonnull stop) {
            if ([obj isKindOfClass:LynxOverlayContainer.class] && !obj.hidden) {
              [array addObject:[NSNumber numberWithInteger:obj.uiDelegate.getSign]];
            }
          }];
    }];
  }
  return array;
}

- (UIView *)getTopViewControllerWithMode:(LynxOverlayMode)mode
                    customViewController:(UIViewController *)customViewController {
  UIView *topContainer = nil;
  switch (mode) {
    case LynxOverlayModeWindow:
      topContainer = [self windowContainer];
      break;
    case LynxOverlayModeTopController:
      topContainer = [self topContainer];
      break;
    case LynxOverlayModeCustom:
      if ([customViewController isKindOfClass:UIViewController.class]) {
        topContainer = customViewController.view;
      } else if ([customViewController isKindOfClass:UIView.class]) {
        topContainer = (UIView *)customViewController;
      } else {
        topContainer = [self windowContainer];
      }
      break;
    default:
      topContainer = [self pageContainer];
      break;
  }
  return topContainer;
}

- (void)destoryOverlayView:(UIView *)overlay
                   atLevel:(NSInteger)level
                  withMode:(LynxOverlayMode)mode
      customViewController:(UIViewController *)customViewController {
  // Firstly, get the top container by the mode

  UIView *topContainer = [self getTopViewControllerWithMode:mode
                                       customViewController:customViewController];

  NSInteger identifier = [self identifierWithModel:mode withContainer:topContainer];

  NSMutableDictionary *allLevelContainerAtTheMode = self.levelContainers[@(identifier)];

  UIView *view = allLevelContainerAtTheMode[@(level)];

  [overlay removeFromSuperview];

  if (view.subviews.count == 0) {
    [allLevelContainerAtTheMode removeObjectForKey:@(level)];

    if (allLevelContainerAtTheMode.count == 0) {
      [self.levelContainers removeObjectForKey:@(identifier)];
    }

    [view removeFromSuperview];
  }
}

- (void)destoryUnattachedOverlay:(UIView *)overlay {
  [overlay removeFromSuperview];

  // Enumerate all the containers to check if they do have some children.

  [[self.levelContainers copy]
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull identifier,
          NSMutableDictionary<NSNumber *, UIView *> *_Nonnull allLevelContainerAtTheMode,
          BOOL *_Nonnull stop) {
        [[allLevelContainerAtTheMode copy]
            enumerateKeysAndObjectsUsingBlock:^(NSNumber *_Nonnull level, UIView *_Nonnull view,
                                                BOOL *_Nonnull stop) {
              if (view.subviews.count == 0) {
                [allLevelContainerAtTheMode removeObjectForKey:level];

                if (allLevelContainerAtTheMode.count == 0) {
                  [self.levelContainers removeObjectForKey:identifier];
                }
                [view removeFromSuperview];
              }
            }];
      }];
}

- (UIView *)showOverlayView:(UIView *)overlay
                    atLevel:(NSInteger)level
                   withMode:(LynxOverlayMode)mode
       customViewController:(UIViewController *)customViewController {
  // Firstly, get the top container by the mode
  UIView *topContainer = [self getTopViewControllerWithMode:mode
                                       customViewController:customViewController];

  // Secondly, get the level container inside the top container with the specific level
  UIView *levelContainer = [self levelContainerAt:level withModel:mode withContainer:topContainer];

  // Thirdly, add the OverlayView to the level container
  if (levelContainer != overlay.superview) {
    [overlay removeFromSuperview];
    [levelContainer addSubview:overlay];
  }
  if (levelContainer.superview != topContainer) {
    [levelContainer removeFromSuperview];
    [topContainer addSubview:levelContainer];
  }

  // Finally, sort all the views inside the top container, because we may insert a new level
  [self sortViewsWithModel:mode withContainer:topContainer];
  return topContainer;
}

- (UIView *)levelContainerAt:(NSInteger)level
                   withModel:(LynxOverlayMode)mode
               withContainer:(UIView *)topContainer {
  NSInteger identifier = [self identifierWithModel:mode withContainer:topContainer];

  if (!self.levelContainers[@(identifier)]) {
    self.levelContainers[@(identifier)] = [NSMutableDictionary dictionary];
  }

  NSMutableDictionary *allLevelContainerAtTheMode = self.levelContainers[@(identifier)];
  if (!allLevelContainerAtTheMode) {
    allLevelContainerAtTheMode = [NSMutableDictionary dictionary];
    self.levelContainers[@(identifier)] = allLevelContainerAtTheMode;
  }

  if (!allLevelContainerAtTheMode[@(level)]) {
    UIView *view = [[LynxOverlayGlobalContainer alloc] initWithFrame:UIScreen.mainScreen.bounds];
    allLevelContainerAtTheMode[@(level)] = view;
    [topContainer addSubview:view];
  }
  return allLevelContainerAtTheMode[@(level)];
}

/**
 * Rearrange all subviews inside topContainer by level
 */
- (void)sortViewsWithModel:(LynxOverlayMode)mode withContainer:(UIView *)topContainer {
  NSInteger identifier = [self identifierWithModel:mode withContainer:topContainer];

  NSMutableDictionary *allLevelContainerAtTheMode = self.levelContainers[@(identifier)];
  NSArray *sortKey =
      [[allLevelContainerAtTheMode allKeys] sortedArrayUsingComparator:^NSComparisonResult(
                                                id _Nonnull levelLeft, id _Nonnull levelRight) {
        NSInteger integerLevelLeft = [levelLeft integerValue];
        NSInteger integerLevelRight = [levelRight integerValue];

        if (integerLevelLeft > integerLevelRight) {
          return (NSComparisonResult)NSOrderedAscending;
        }
        if (integerLevelLeft < integerLevelRight) {
          return (NSComparisonResult)NSOrderedDescending;
        }
        return (NSComparisonResult)NSOrderedSame;
      }];

  [sortKey enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
    UIView *levelContainer = allLevelContainerAtTheMode[obj];
    [topContainer bringSubviewToFront:levelContainer];
  }];
}

/**
 * Generate identifier to identify the container instance at specified mode
 */
- (NSInteger)identifierWithModel:(LynxOverlayMode)mode withContainer:(UIView *)container {
  return mode + (uintptr_t)container;
}

- (UIView *)modalContainer {
  UIViewController *topController = [LynxUIKitAPIAdapter getForegroundKeyWindow].rootViewController;
  while (topController.presentedViewController) {
    topController = topController.presentedViewController;
  }
  return topController.view;
}

- (UIView *)topContainer {
  UIViewController *rootController =
      [LynxUIKitAPIAdapter getForegroundKeyWindow].rootViewController;
  return [self topViewControllerForController:rootController].view;
}

- (UIViewController *)topViewControllerForController:(UIViewController *)rootViewController {
  if ([rootViewController isKindOfClass:[UINavigationController class]]) {
    UINavigationController *navigationController = (UINavigationController *)rootViewController;
    return [self topViewControllerForController:[navigationController.viewControllers lastObject]];
  }
  if ([rootViewController isKindOfClass:[UITabBarController class]]) {
    UITabBarController *tabController = (UITabBarController *)rootViewController;
    return [self topViewControllerForController:tabController.selectedViewController];
  }
  if (rootViewController.presentedViewController) {
    return [self topViewControllerForController:rootViewController.presentedViewController];
  }
  return rootViewController;
}

- (UIView *)pageContainer {
  UIViewController *topController;
  UIViewController *rootController =
      [LynxUIKitAPIAdapter getForegroundKeyWindow].rootViewController;
  if ([rootController isKindOfClass:UINavigationController.class]) {
    topController = ((UINavigationController *)rootController);
  } else {
    topController = rootController.navigationController ?: rootController;
  }
  return topController.view;
}

- (UIView *)windowContainer {
  return [LynxUIKitAPIAdapter getForegroundKeyWindow];
}

+ (CGRect)windowBounds {
  UIWindow *window = [LynxUIKitAPIAdapter getKeyWindow];
  if (window) {
    return window.bounds;
  }
  return [UIScreen mainScreen].bounds;
}

@end
