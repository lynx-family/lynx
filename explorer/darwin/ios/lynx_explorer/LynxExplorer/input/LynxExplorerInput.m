// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxExplorerInput.h"
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIOwner.h>
#import "UIHelper.h"

@implementation LynxTextField

- (UIEditingInteractionConfiguration)editingInteractionConfiguration API_AVAILABLE(ios(13.0)) {
  return UIEditingInteractionConfigurationNone;
}

- (void)setPadding:(UIEdgeInsets)padding {
  _padding = padding;
  [self setNeedsLayout];
}

- (CGRect)textRectForBounds:(CGRect)bounds {
  CGFloat x = self.padding.left;
  CGFloat y = self.padding.top;
  CGFloat width = bounds.size.width - self.padding.left - self.padding.right;
  CGFloat height = bounds.size.height - self.padding.top - self.padding.bottom;

  return CGRectMake(x, y, width, height);
}

- (CGRect)editingRectForBounds:(CGRect)bounds {
  return [self textRectForBounds:bounds];
}
@end

@implementation LynxExplorerInput

LYNX_LAZY_REGISTER_UI("input")

- (UITextField *)createView {
  UITextField *textField = [[LynxTextField alloc] init];
  textField.autoresizesSubviews = NO;
  textField.clipsToBounds = YES;
  textField.delegate = self;
  textField.secureTextEntry = NO;
  textField.font = [UIFont systemFontOfSize:14];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(textFieldDidChange:)
                                               name:UITextFieldTextDidChangeNotification
                                             object:textField];

  return textField;
}

- (void)layoutDidFinished {
  self.view.padding = self.padding;
}

LYNX_PROP_SETTER("value", setValue, NSString *) { self.view.text = value; }

LYNX_PROP_SETTER("placeholder", setPlaceholder, NSString *) { self.view.placeholder = value; }

LYNX_PROP_SETTER("text-color", setTextColor, NSString *) {
  self.view.textColor = [UIHelper colorWithHexString:value];
}

- (void)textFieldDidChange:(NSNotification *)notification {
  [self emitEvent:@"input"
           detail:@{
             @"value" : [self.view text] ?: @"",
           }];
}

LYNX_UI_METHOD(focus) {
  if ([self.view becomeFirstResponder]) {
    callback(kUIMethodSuccess, nil);
  } else {
    callback(kUIMethodUnknown, @"fail to focus");
  }
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
  [self emitEvent:@"blur" detail:@{@"value" : [self.view text] ?: @""}];
}

- (void)emitEvent:(NSString *)name detail:(NSDictionary *)detail {
  LynxCustomEvent *eventInfo = [[LynxDetailEvent alloc] initWithName:name
                                                          targetSign:[self sign]
                                                              detail:detail];
  [self.context.eventEmitter dispatchCustomEvent:eventInfo];
}

@end
