// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ExplorerLynxTestModule.h"
#import <Lynx/LynxTemplateData.h>

@interface ExplorerLynxTestModule ()
@property(atomic, assign) BOOL destroyed;
@end

@implementation ExplorerLynxTestModule {
  __weak LynxContext *_context;
  NSMutableArray<UIButton *> *_buttons;
}

+ (NSString *)name {
  return @"LynxTestModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"eventTest" : NSStringFromSelector(@selector(eventTest:)),
    @"valueTest" : NSStringFromSelector(@selector(valueTest:)),
    @"back" : NSStringFromSelector(@selector(back)),
    @"reload" : NSStringFromSelector(@selector(reload)),
    @"call" : NSStringFromSelector(@selector(call:params:callback:)),
    @"invoke" : NSStringFromSelector(@selector(invokeParams:callback:)),
    @"callSync" : NSStringFromSelector(@selector(call:params:)),
    @"updateData" : NSStringFromSelector(@selector(updateData:)),
    @"resetData" : NSStringFromSelector(@selector(resetData:)),
    @"updateGlobalProps" : NSStringFromSelector(@selector(updateGlobalProps:)),
    @"reloadTemplate" : NSStringFromSelector(@selector(reloadTemplate:props:)),
    @"getPageDataByKey" : NSStringFromSelector(@selector(getPageDataByKey:callback:)),
    @"updateScreenMatrix" : NSStringFromSelector(@selector(updateScreenMatrix:)),
    @"addButton" : NSStringFromSelector(@selector(addButton:)),
    @"setDefaultValueForSetting" : NSStringFromSelector(@selector(setDefaultValueForSetting:)),
  };
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    _context = context;
    _buttons = [NSMutableArray array];
  }
  return self;
}

- (LynxView *)currentView {
  LynxContext *context = _context;
  return !self.destroyed && !context.hasLynxViewDestroyed ? [context getLynxView] : nil;
}

- (void)withView:(void (^)(LynxView *))action {
  dispatch_block_t work = ^{
    LynxView *view = [self currentView];
    if (view) {
      action(view);
    }
  };
  if ([NSThread isMainThread]) {
    work();
  } else {
    dispatch_async(dispatch_get_main_queue(), work);
  }
}

- (UIViewController *)controllerForView:(UIView *)view {
  UIResponder *responder = view;
  while (responder && ![responder isKindOfClass:UIViewController.class]) {
    responder = responder.nextResponder;
  }
  return (UIViewController *)responder;
}

- (void)eventTest:(NSString *)value {
  [self withView:^(LynxView *view) {
    [self->_context sendGlobalEvent:@"test" withParams:@[ @10, value ]];
  }];
}

- (void)valueTest:(NSString *)value {
  NSData *data = [value dataUsingEncoding:NSUTF8StringEncoding];
  id parsed = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
  id result = [parsed isKindOfClass:NSDictionary.class] || [parsed isKindOfClass:NSArray.class]
                  ? parsed
                  : value;
  [self withView:^(LynxView *view) {
    [self->_context sendGlobalEvent:@"test" withParams:@[ result ]];
  }];
}

- (void)back {
  [self withView:^(LynxView *view) {
    UIViewController *controller = [self controllerForView:view];
    if (controller.navigationController.topViewController == controller) {
      [controller.navigationController popViewControllerAnimated:YES];
    }
  }];
}

- (void)reload {
  [self withView:^(LynxView *view) {
    NSString *url = view.url;
    if (url.length == 0) {
      return;
    }
    NSURL *resourceURL = [NSURL URLWithString:url];
    if ([url hasPrefix:@"/"] || resourceURL.isFileURL) {
      NSString *path = resourceURL.isFileURL ? resourceURL.path : url;
      NSData *data = [NSData dataWithContentsOfFile:path];
      if (data) {
        [view loadTemplate:data withURL:url];
      }
    } else {
      // Keep URL resolution in the page's existing template provider/fetcher.
      [view loadTemplateFromURL:url initData:nil];
    }
  }];
}

- (void)call:(NSString *)name params:(NSDictionary *)params callback:(LynxCallbackBlock)callback {
  [self invokeParams:params callback:callback];
}

- (void)invokeParams:(NSDictionary *)params callback:(LynxCallbackBlock)callback {
  if (!callback || self.destroyed) {
    return;
  }
  // Deliberate blocking work followed by a delayed callback for bridge timing tests.
  [NSThread sleepForTimeInterval:2.0];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    if ([self currentView]) {
      callback(@{@"cb_data" : @"44444"});
    }
  });
}

- (NSDictionary *)call:(NSString *)name params:(NSDictionary *)params {
  return @{@"cb_data" : @"5555"};
}

- (void)updateData:(NSDictionary *)data {
  [self withView:^(LynxView *view) {
    [view updateDataWithDictionary:data];
  }];
}

- (void)resetData:(NSDictionary *)data {
  [self withView:^(LynxView *view) {
    [view resetDataWithTemplateData:[[LynxTemplateData alloc] initWithDictionary:data]];
  }];
}

- (void)updateGlobalProps:(NSDictionary *)props {
  [self withView:^(LynxView *view) {
    [view updateGlobalPropsWithDictionary:props];
  }];
}

- (void)reloadTemplate:(NSDictionary *)data props:(NSDictionary *)props {
  [self withView:^(LynxView *view) {
    [view reloadTemplateWithTemplateData:[[LynxTemplateData alloc] initWithDictionary:data]
                             globalProps:[[LynxTemplateData alloc] initWithDictionary:props]];
  }];
}

- (void)getPageDataByKey:(NSArray *)keys callback:(LynxCallbackBlock)callback {
  [self withView:^(LynxView *view) {
    if (callback) {
      callback([view getPageDataByKey:keys]);
    }
  }];
}

- (void)updateScreenMatrix:(NSDictionary *)matrix {
  [self withView:^(LynxView *view) {
    [view updateScreenMetricsWithWidth:[matrix[@"width"] floatValue]
                                height:[matrix[@"height"] floatValue]];
    [view triggerLayout];
  }];
}

- (void)addButton:(NSDictionary *)info {
  [self withView:^(LynxView *view) {
    if (!view.superview) {
      return;
    }
    UIButton *button = [[UIButton alloc]
        initWithFrame:CGRectMake([info[@"left"] floatValue], [info[@"top"] floatValue], 0, 0)];
    [button setTitle:@"0" forState:UIControlStateNormal];
    [button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    button.backgroundColor = UIColor.blueColor;
    button.layer.cornerRadius = 8.0;
    if (info[@"fontSize"]) {
      button.titleLabel.font = [UIFont systemFontOfSize:[info[@"fontSize"] floatValue]];
    }
    [button sizeToFit];
    [button addTarget:self
                  action:@selector(buttonTapped:)
        forControlEvents:UIControlEventTouchUpInside];
    [view.superview insertSubview:button belowSubview:view];
    [self->_buttons addObject:button];
  }];
}

- (void)buttonTapped:(UIButton *)button {
  NSInteger count = [button.currentTitle integerValue] + 1;
  [button setTitle:[NSString stringWithFormat:@"%ld", (long)count] forState:UIControlStateNormal];
  [button sizeToFit];
}

- (void)setDefaultValueForSetting:(NSDictionary *)info {
  if (![NSPropertyListSerialization propertyList:info
                                isValidForFormat:NSPropertyListBinaryFormat_v1_0]) {
    [_context reportModuleCustomError:@"Settings must contain property-list values."];
    return;
  }
  [self withView:^(LynxView *view) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    [info enumerateKeysAndObjectsUsingBlock:^(NSString *key, id value, BOOL *stop) {
      [defaults setObject:value forKey:key];
    }];
    [defaults synchronize];
    UIAlertController *alert = [UIAlertController
        alertControllerWithTitle:@"Settings updated"
                         message:@"Restart the app manually to apply the updated settings."
                  preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK"
                                              style:UIAlertActionStyleDefault
                                            handler:nil]];
    UIViewController *controller = [self controllerForView:view];
    while (controller.presentedViewController &&
           !controller.presentedViewController.isBeingDismissed) {
      controller = controller.presentedViewController;
    }
    [controller presentViewController:alert animated:YES completion:nil];
  }];
}

- (void)destroy {
  self.destroyed = YES;
  dispatch_async(dispatch_get_main_queue(), ^{
    for (UIButton *button in self->_buttons) {
      [button removeFromSuperview];
    }
    [self->_buttons removeAllObjects];
  });
}

@end
