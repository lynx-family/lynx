// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLogNotification.h"
#import "LynxLogBoxHelper.h"
#import "LynxLogBoxManager.h"

@interface LynxLogNotificationManager ()

- (void)hideNotificationOfIndex:(NSInteger)index;

@end

#pragma mark - LynxLogNotificationView

@interface LynxLogNotificationView : UIView

@property(nonatomic, readwrite) NSInteger currentMsgCount;
@property(nonatomic, readwrite) NSInteger layoutIndex;

- (instancetype)initWithManager:(LynxLogNotificationManager *)manager
                      withLevel:(LynxLogBoxLevel)level;
- (void)updateLayout:(NSInteger)index;
- (void)showMessage:(NSString *)msg;
- (void)updateMessage:(NSString *)msg;
- (void)updateMessageCount:(NSNumber *)count;
- (void)close;
- (BOOL)isShowing;
- (void)show;
- (void)hide;

@end

@implementation LynxLogNotificationView {
  UILabel *_textLabel;
  UILabel *_countLabel;
  UILabel *_separateLabel;
  UIButton *_closeButton;
#if OS_OSX
  NSView *_countView;
  NSImageView *_closeImage;
#endif
  LynxLogBoxLevel _level;
  __weak LynxLogNotificationManager *_manager;
}

static CGFloat frameX = 5.0;
static CGFloat frameHeight = 45.0;
static CGFloat commonMargin = 9.0;
static CGFloat closeMargin = 11.0;
static CGFloat imageMargin = 3.0;

// _textLabel - The latest msg for the current level.
// _countLabel - The total number of all messages for the current level.
// _separateLabel - Vertical line which separate _textLabel and _countLabel.
// _closeButton - When click this btn, notification view will be closed and all messages for the
// current level will be cleared.
// _countView - Only used on MacOS. Show the color for the current level, _countLabel just shows the
// number.
// _closeImage - Only used on MacOS. Show the image of _closeButton.
- (instancetype)initWithManager:(LynxLogNotificationManager *)manager
                      withLevel:(LynxLogBoxLevel)level {
  _manager = manager;
  _currentMsgCount = 0;
  _layoutIndex = 0;
  _level = level;

  UIColor *frameColor = [UIColor colorWithRed:21.0 / 255
                                        green:21.0 / 255
                                         blue:21.0 / 255
                                        alpha:0.95];
  UIColor *warnColor = [UIColor colorWithRed:239.0 / 255
                                       green:189.0 / 255
                                        blue:54.0 / 255
                                       alpha:1.0];
  UIColor *errorColor = [UIColor colorWithRed:228.0 / 255
                                        green:67.0 / 255
                                         blue:105.0 / 255
                                        alpha:1.0];
  UIColor *separateColor = [UIColor colorWithRed:92.0 / 255
                                           green:93.0 / 255
                                            blue:93.0 / 255
                                           alpha:1.0];
  UIColor *closeColor = [UIColor colorWithRed:113.0 / 255
                                        green:114.0 / 255
                                         blue:114.0 / 255
                                        alpha:1.0];
  UIColor *whiteColor = [UIColor whiteColor];
  UIColor *blackColor = [UIColor blackColor];

  UIColor *levelColor;
  if (_level == LynxLogBoxLevelWarning) {
    levelColor = warnColor;
  } else if (_level == LynxLogBoxLevelError) {
    levelColor = errorColor;
  } else {
    levelColor = blackColor;
  }

// Calculate the size of frame.
#if OS_OSX
  CGFloat frameWidth =
      [NSApplication sharedApplication].mainWindow.contentView.frame.size.width - 2 * frameX;
  CGFloat frameY = frameX;
#else
  CGFloat frameWidth = [UIScreen mainScreen].bounds.size.width - 2 * frameX;
  CGFloat frameY =
      [UIScreen mainScreen].bounds.size.height - frameHeight - [self getWindowSafeAreaBottom];
#endif
  CGRect frame = CGRectMake(frameX, frameY, frameWidth, frameHeight);

  if (self = [super initWithFrame:frame]) {
#if OS_IOS
    self.backgroundColor = frameColor;
#else
    self.wantsLayer = YES;
    self.layer.backgroundColor = frameColor.CGColor;
#endif
    self.layer.cornerRadius = 8.0;

    // Calculate the size of subview.
    CGFloat textLabelFrameX = frameHeight + commonMargin;
    CGFloat textLabelWidth = frameWidth - (frameHeight + commonMargin) * 2;
    CGFloat separateLabelWidth = 1;
    CGFloat separateLabelHeight = frameHeight - commonMargin * 2;
    CGFloat separateLabelFrameX = frameHeight - separateLabelWidth / 2.0;
    CGFloat countLabelWidth = frameHeight - commonMargin * 2;
    CGFloat closeBtnFrameX = frameWidth - frameHeight + closeMargin;
    CGFloat closeBtnWidth = frameHeight - closeMargin * 2;
#if OS_IOS
    CGFloat textLabelFrameY = 0;
    CGFloat textLabelHeight = frameHeight;
#else
    CGFloat textLabelHeight = 20;
    CGFloat textLabelFrameY = (frameHeight - textLabelHeight) / 2.0;
    CGFloat closeImageFrameX = frameWidth - frameHeight + closeMargin + imageMargin;
    CGFloat closeImageFrameY = closeMargin + imageMargin;
    CGFloat closeImageWidth = frameHeight - (closeMargin + imageMargin) * 2;
#endif

    // Init _textLabel.
    _textLabel = [[UILabel alloc] initWithFrame:CGRectMake(textLabelFrameX, textLabelFrameY,
                                                           textLabelWidth, textLabelHeight)];
#if OS_OSX
    _textLabel.bezeled = NO;
    _textLabel.editable = NO;
    _textLabel.drawsBackground = NO;
    _textLabel.maximumNumberOfLines = 1;
#endif
    _textLabel.textColor = [UIColor whiteColor];
    _textLabel.font = [UIFont systemFontOfSize:15];

    // Init _separateLabel.
    _separateLabel =
        [[UILabel alloc] initWithFrame:CGRectMake(separateLabelFrameX, commonMargin,
                                                  separateLabelWidth, separateLabelHeight)];
    _separateLabel.backgroundColor = separateColor;
#if OS_OSX
    _separateLabel.bezeled = NO;
    _separateLabel.editable = NO;
#endif

// Init _countView(only on MacOS) and _countLabel.
#if OS_OSX
    _countView = [[NSView alloc]
        initWithFrame:CGRectMake(commonMargin, commonMargin, countLabelWidth, countLabelWidth)];
    _countView.wantsLayer = YES;
    _countView.layer.cornerRadius = _countView.bounds.size.width / 2.0;
    _countView.layer.masksToBounds = YES;
    _countView.layer.borderWidth = 2;
    _countView.layer.borderColor = whiteColor.CGColor;
    _countView.layer.backgroundColor = levelColor.CGColor;
    _countLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 12, 45, 20)];
    _countLabel.wantsLayer = YES;
    _countLabel.drawsBackground = NO;
    _countLabel.bezeled = NO;
    _countLabel.editable = NO;
    _countLabel.alignment = NSTextAlignmentCenter;
#else
    _countLabel = [[UILabel alloc]
        initWithFrame:CGRectMake(commonMargin, commonMargin, countLabelWidth, countLabelWidth)];
    _countLabel.layer.cornerRadius = _countLabel.bounds.size.width / 2.0;
    _countLabel.layer.masksToBounds = YES;
    _countLabel.layer.borderWidth = 2;
    _countLabel.layer.borderColor = whiteColor.CGColor;
    _countLabel.backgroundColor = levelColor;
    _countLabel.textAlignment = NSTextAlignmentCenter;
#endif
    _countLabel.textColor = whiteColor;
    _countLabel.font = [UIFont systemFontOfSize:16];

    // Init _closeButton.
    _closeButton = [[UIButton alloc]
        initWithFrame:CGRectMake(closeBtnFrameX, closeMargin, closeBtnWidth, closeBtnWidth)];
#if OS_OSX
    _closeButton.wantsLayer = YES;
    _closeButton.layer.backgroundColor = closeColor.CGColor;
#else
    _closeButton.backgroundColor = closeColor;
#endif
    _closeButton.layer.cornerRadius = _closeButton.bounds.size.width / 2.0;
    _closeButton.layer.masksToBounds = YES;

    // Set image to _closeButton(iOS)/_closeImage(MacOS).
    NSURL *debugBundleUrl = [[NSBundle mainBundle] URLForResource:@"LynxDebugResources"
                                                    withExtension:@"bundle"];
    if (debugBundleUrl) {
      NSBundle *bundle = [NSBundle bundleWithURL:debugBundleUrl];
      NSString *path = [bundle pathForResource:@"notification_cancel" ofType:@"png"];
      if (path) {
#if OS_OSX
        _closeImage =
            [[NSImageView alloc] initWithFrame:CGRectMake(closeImageFrameX, closeImageFrameY,
                                                          closeImageWidth, closeImageWidth)];
        NSImage *_btnImage = [[NSImage alloc] initWithContentsOfFile:path];
        [_closeImage setImage:_btnImage];
        [_closeImage setImageScaling:NSImageScaleProportionallyDown];
        _closeImage.alphaValue = 0.8;
#else
        UIImage *_btnImage = [UIImage imageWithContentsOfFile:path];
        [_closeButton setImage:_btnImage forState:UIControlStateNormal];
        [_closeButton setImageEdgeInsets:UIEdgeInsetsMake(imageMargin, imageMargin, imageMargin,
                                                          imageMargin)];
        _closeButton.imageView.contentMode = UIViewContentModeScaleAspectFit;
        _closeButton.imageView.alpha = 0.8;
#endif
      }
    }

// Add gesture selector for _closeButton.
#if OS_OSX
    NSClickGestureRecognizer *click =
        [[NSClickGestureRecognizer alloc] initWithTarget:self action:@selector(close)];

    [_closeButton addGestureRecognizer:click];
    [_closeImage addGestureRecognizer:click];
#else
    [_closeButton addTarget:self
                     action:@selector(close)
           forControlEvents:UIControlEventTouchUpInside];
#endif

// Add observer when the window size changes.
#if OS_OSX
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(resizeFrame:)
                                                 name:NSWindowDidResizeNotification
                                               object:[NSApplication sharedApplication].mainWindow];
#endif

    // Add subview.
    // The order must be "_closeButton > _closeImage" and "_countView > _countLabel" on MacOS.
    [self addSubview:_closeButton];
#if OS_OSX
    [self addSubview:_closeImage];
    [self addSubview:_countView];
#endif
    [self addSubview:_textLabel];
    [self addSubview:_separateLabel];
    [self addSubview:_countLabel];
  }
  return self;
}

- (void)updateLayout:(NSInteger)index {
  if ([NSThread isMainThread]) {
    [self updateLayoutOnMainThread:index];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self updateLayoutOnMainThread:index];
    });
  }
}

- (void)updateLayoutOnMainThread:(NSInteger)index {
  if (index == _layoutIndex || index < 0) return;
  _layoutIndex = index;
#if OS_OSX
  CGFloat y = _layoutIndex * self.frame.size.height + (_layoutIndex + 1) * 5;
#else
  CGFloat y = [UIScreen mainScreen].bounds.size.height - [self getWindowSafeAreaBottom] -
              (_layoutIndex + 1) * self.frame.size.height - _layoutIndex * 5;
#endif
  CGRect frame = CGRectMake(self.frame.origin.x, y, self.frame.size.width, self.frame.size.height);
  [self setFrame:frame];
#if OS_IOS
  [self setNeedsLayout];
  [self layoutIfNeeded];
#endif
}

// Recalculate the size of frame when the window size changes.
#if OS_OSX
- (void)resizeFrame:(NSNotification *)notification {
  if ([self isShowing]) {
    CGFloat width = [NSApplication sharedApplication].mainWindow.contentView.frame.size.width -
                    2 * self.frame.origin.x;
    if ([NSApplication sharedApplication].mainWindow.contentView.frame.size.width <
        (self.frame.size.height + commonMargin) * 2) {
      width = self.frame.size.height * 2;
    }
    [self setFrame:CGRectMake(self.frame.origin.x, self.frame.origin.y, width,
                              self.frame.size.height)];
    [_textLabel setFrame:CGRectMake(_textLabel.frame.origin.x, _textLabel.frame.origin.y,
                                    width - (self.frame.size.height + commonMargin) * 2,
                                    _textLabel.frame.size.height)];
    [_closeButton
        setFrame:CGRectMake(width - self.frame.size.height + closeMargin, closeMargin,
                            _closeButton.frame.size.width, _closeButton.frame.size.height)];
    [_closeImage setFrame:CGRectMake(width - self.frame.size.height + closeMargin + imageMargin,
                                     closeMargin + imageMargin, _closeImage.frame.size.width,
                                     _closeImage.frame.size.height)];
  }
}
#endif

- (void)showMessage:(NSString *)msg {
  [self updateMessageCount:[NSNumber numberWithInteger:1]];
  [self updateMessage:msg];
}

- (void)updateMessage:(NSString *)msg {
  if ([NSThread isMainThread]) {
    [self updateMessageOnMainThread:msg];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self updateMessageOnMainThread:msg];
    });
  }
}

- (void)updateMessageOnMainThread:(NSString *)msg {
#if OS_OSX
  _textLabel.stringValue = msg;
#else
  [_textLabel setText:msg];
#endif
}

- (void)updateMessageCount:(NSNumber *)count {
  if ([NSThread isMainThread]) {
    [self updateMessageCountOnMainThread:count];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self updateMessageCountOnMainThread:count];
    });
  }
}

- (void)updateMessageCountOnMainThread:(NSNumber *)count {
  if ((_currentMsgCount += [count integerValue]) < 0) {
    _currentMsgCount = 0;
  }
#if OS_OSX
  _countLabel.stringValue = @(_currentMsgCount).stringValue;
#else
  [_countLabel setText:@(_currentMsgCount).stringValue];
#endif
}

- (void)close {
  __strong typeof(_manager) manager = _manager;
  [manager closeNotification:_level];
}

- (BOOL)isShowing {
  if ([self superview] && ![self isHidden]) {
    return YES;
  } else {
    return NO;
  }
}

- (void)show {
  if ([NSThread isMainThread]) {
    [self showOnMainThread];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self showOnMainThread];
    });
  }
}

- (void)showOnMainThread {
  if (self.superview == nil) {
#if OS_OSX
    [[NSApplication sharedApplication].mainWindow.contentView addSubview:self];
#else
    UIWindow *window = [self getKeyWindow];
    [window addSubview:self];
#endif
  }
  [self setHidden:NO];
}

- (void)hide {
  if ([NSThread isMainThread]) {
    [self hideOnMainThread];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self hideOnMainThread];
    });
  }
}

- (void)hideOnMainThread {
  [self setHidden:YES];
  __strong typeof(_manager) manager = _manager;
  [manager hideNotificationOfIndex:_layoutIndex];
}

#if OS_IOS
- (UIWindow *)getKeyWindow {
  UIWindow *keyWindow;
  if (@available(iOS 13.0, *)) {
    // For iOS 13 and above, we need to find the key window through UIScene
    // If no UIScene is found, it means the app is not using UIScene-based lifecycle
    NSSet<UIScene *> *connectedScenes = [UIApplication sharedApplication].connectedScenes;
    for (UIScene *scene in connectedScenes) {
      if (![scene isKindOfClass:[UIWindowScene class]] ||
          scene.activationState != UISceneActivationStateForegroundActive) {
        continue;
      }
      UIWindowScene *windowScene = (UIWindowScene *)scene;
      for (UIWindow *window in windowScene.windows) {
        if (window.isKeyWindow) {
          keyWindow = window;
          break;
        }
      }
    }
    // If no key window is found through UIScene, the app might not be using UIScene-based lifecycle
    // In this case, we fall back to accessing window through UIApplicationDelegate
    if (!keyWindow &&
        [[UIApplication sharedApplication].delegate respondsToSelector:@selector(window)]) {
      keyWindow = [[UIApplication sharedApplication].delegate window];
    }
  } else if (@available(iOS 11.0, *) &&
             [[UIApplication sharedApplication].delegate respondsToSelector:@selector(window)]) {
    keyWindow = [[UIApplication sharedApplication].delegate window];
  }
  return keyWindow;
}
#endif

- (CGFloat)getWindowSafeAreaBottom {
#if OS_IOS
  UIWindow *window = [self getKeyWindow];
  if (!window) {
    return 0;
  }
  if (@available(iOS 11.0, *)) {
    return window.safeAreaInsets.bottom;
  }
#endif
  return 0;
}

@end

#pragma mark - LynxLogNotificationManager
@implementation LynxLogNotificationManager {
  NSMutableDictionary<NSNumber *, LynxLogNotificationView *> *_notificationViews;
  NSMutableDictionary<NSNumber *, NSString *> *_tapSelectors;
  __weak LynxLogBoxManager *_manager;
  NSInteger _currentLayoutIndex;
}

- (instancetype)initWithLogBoxManager:(LynxLogBoxManager *)manager {
  self = [super init];
  if (self) {
    _manager = manager;
    _currentLayoutIndex = -1;
    _notificationViews = [NSMutableDictionary dictionary];
    _tapSelectors = [NSMutableDictionary dictionary];
    [_tapSelectors setObject:@"tapWarnNotification"
                      forKey:[NSNumber numberWithInteger:LynxLogBoxLevelWarning]];
    [_tapSelectors setObject:@"tapErrorNotification"
                      forKey:[NSNumber numberWithInteger:LynxLogBoxLevelError]];
  }
  return self;
}

- (void)showNotificationWithMsg:(NSString *)msg withLevel:(LynxLogBoxLevel)level {
  NSNumber *levelNum = [NSNumber numberWithInteger:level];
  if ([self getNotificationViewWithLevel:level] == nil) {
    LynxLogNotificationView *view = [[LynxLogNotificationView alloc] initWithManager:self
                                                                           withLevel:level];
    SEL tapSel = NSSelectorFromString([_tapSelectors objectForKey:levelNum]);
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self
                                                                          action:tapSel];
#if OS_IOS
    view.userInteractionEnabled = YES;
#endif
    [view addGestureRecognizer:tap];
    @synchronized(_notificationViews) {
      [_notificationViews setObject:view forKey:levelNum];
    }
  }

  if (msg != nil) {
    [self showMessage:msg withLevel:level];
    [self showNotificationWithLevel:level];
  }
}

- (void)showMessage:(NSString *)msg withLevel:(LynxLogBoxLevel)level {
  [[self getNotificationViewWithLevel:level] showMessage:msg];
}

- (void)showNotificationWithLevel:(LynxLogBoxLevel)level {
  LynxLogNotificationView *view = [self getNotificationViewWithLevel:level];
  if (view != nil && ![view isShowing]) {
    [view updateLayout:++_currentLayoutIndex];
    [view show];
  }
}

- (void)updateNotificationMsg:(NSString *)msg withLevel:(LynxLogBoxLevel)level {
  if (msg != nil) {
    [[self getNotificationViewWithLevel:level] updateMessage:msg];
    [self showNotificationWithLevel:level];
  }
}

- (void)updateNotificationMsgCount:(NSNumber *)count withLevel:(LynxLogBoxLevel)level {
  if (count != nil) {
    [[self getNotificationViewWithLevel:level] updateMessageCount:count];
    [self showNotificationWithLevel:level];
  }
}

- (void)showNotification {
  for (NSNumber *level in _notificationViews) {
    LynxLogNotificationView *view = [_notificationViews objectForKey:level];
    if ([view currentMsgCount] > 0) {
      [self showNotificationWithLevel:[level integerValue]];
    }
  }
}

- (void)hideNotification {
  for (NSNumber *level in _notificationViews) {
    LynxLogNotificationView *view = [_notificationViews objectForKey:level];
    if ([view isShowing]) {
      [view hide];
    }
  }
}

- (void)hideNotificationOfIndex:(NSInteger)index {
  --_currentLayoutIndex;
  for (NSNumber *level in _notificationViews) {
    LynxLogNotificationView *view = [_notificationViews objectForKey:level];
    if (index < [view layoutIndex]) {
      [view updateLayout:[view layoutIndex] - 1];
    }
  }
}

- (void)tapWarnNotification {
  __strong typeof(_manager) manager = _manager;
  [manager showLogBoxWithLevel:LynxLogBoxLevelWarning];
}

- (void)tapErrorNotification {
  __strong typeof(_manager) manager = _manager;
  [manager showLogBoxWithLevel:LynxLogBoxLevelError];
}

- (void)removeNotificationWithLevel:(LynxLogBoxLevel)level {
  NSNumber *levelNum = [NSNumber numberWithInteger:level];
  LynxLogNotificationView *view = [self getNotificationViewWithLevel:level];
  [self hideNotificationOfIndex:[view layoutIndex]];
  [view removeFromSuperview];
  [_notificationViews removeObjectForKey:levelNum];
}

- (void)closeNotification:(LynxLogBoxLevel)level {
  __strong typeof(_manager) manager = _manager;
  [manager removeLogsWithLevel:level];
  [self removeNotificationWithLevel:level];
}

- (LynxLogNotificationView *)getNotificationViewWithLevel:(LynxLogBoxLevel)level {
  @synchronized(_notificationViews) {
    NSNumber *levelNum = [NSNumber numberWithInteger:level];
    return [_notificationViews objectForKey:levelNum];
  }
}

- (void)dealloc {
  for (NSNumber *level in _notificationViews) {
    [[_notificationViews objectForKey:level] removeFromSuperview];
  }
}

@end
