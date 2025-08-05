// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIInput.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxNativeLayoutNode.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxConverter+UI.h>
#import <Lynx/LynxColorUtils.h>

@interface LynxTextFieldLite : UITextField

@property(nonatomic, assign) UIEdgeInsets padding;

@end

@implementation LynxTextFieldLite

- (UIEditingInteractionConfiguration)editingInteractionConfiguration API_AVAILABLE(ios(13.0)){
    return UIEditingInteractionConfigurationNone;
}

- (void)scrollTextFieldToVisibleIfNecessary {
}

- (void)setPadding:(UIEdgeInsets)padding {
    _padding = padding;
    [self setNeedsLayout];
}

- (CGRect)textRectForBounds:(CGRect)bounds{
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


@implementation LynxUIInputShadowNode

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  if (self = [super initWithSign:sign tagName:tagName]) {
    self.widthForMeasure = CGFLOAT_MAX;
  }
  return self;
}

@end

@interface LynxUIInput () <UITextFieldDelegate>

@end

@implementation LynxUIInput

- (UITextField *)createView {
  UITextField* textField = [[LynxTextFieldLite alloc] init];
  textField.autoresizesSubviews = NO;
  textField.clipsToBounds = YES;
  textField.delegate = self;
  textField.secureTextEntry = NO;
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onWillShowKeyboard:) name:UIKeyboardWillShowNotification object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onWillHideKeyboard:) name:UIKeyboardWillHideNotification object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(textFieldDidChange:) name:UITextFieldTextDidChangeNotification object:textField];

  return textField;
}

LYNX_PROP_SETTER("show-soft-input-on-focus", setShowSoftInputOnFocus, BOOL) {
  self.view.inputView = value ? nil : [[UIView alloc] initWithFrame:CGRectMake(0, 0, 1, 1)];
  self.view.inputAccessoryView = value ? nil : [[UIView alloc] initWithFrame:CGRectMake(0, 0, 1, 1)];
  if (!value) {
    if (@available(iOS 9.0, *)) {
      UITextInputAssistantItem *item = self.view.inputAssistantItem;
      item.leadingBarButtonGroups = @[];
      item.trailingBarButtonGroups = @[];
    }
  }
}

- (void)updateFrame:(CGRect)frame withPadding:(UIEdgeInsets)padding border:(UIEdgeInsets)border margin:(UIEdgeInsets)margin withLayoutAnimation:(BOOL)with {
  [super updateFrame:frame withPadding:padding border:border margin:margin withLayoutAnimation:with];
  ((LynxTextFieldLite *)self.view).padding = self.padding;
}

- (void)propsDidUpdate {
  [super propsDidUpdate];  

  self.view.defaultTextAttributes = self.inputAttrs;
  if (self.placeholder) {
    self.view.attributedPlaceholder = [[NSAttributedString alloc] initWithString:self.placeholder attributes:self.placeholderAttrs];
  }
}


LYNX_UI_METHOD(setValue) {
  NSString *value = params[@"value"];
  NSInteger cursor = [(params[@"cursor"] ? : @(-1)) integerValue];
  
  if (![self inputView:self.view checkInputValidity:value]) {
  } else {
    [self.view setText:value];
  }
  
  if (cursor >= 0) {
    UITextPosition* beginning = self.view.beginningOfDocument;
    UITextPosition* cursorPosition = [self.view positionFromPosition:beginning offset:cursor];
    self.view.selectedTextRange = [self.view textRangeFromPosition:cursorPosition toPosition:cursorPosition];
  }

  callback(kUIMethodSuccess, nil);
}

- (NSString *)getText {
  return self.view.attributedText.string ? : (self.view.text ? : @"");
}

- (void)inputWillBeFilteredFrom:(NSString *)source to:(NSString *)dest {
  self.view.text = dest;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
  return [self inputViewShouldReturn:textField];
}

- (void)textFieldDidChange:(NSNotification *)notification {
  [self inputViewDidChange:self.view];
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
  return [self inputView:textField shouldChangeCharactersInRange:range replacementString:string];
}

- (void)textFieldDidBeginEditing:(UITextField *)textField {
  [self inputViewDidBeginEditing:textField];
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
  [self inputViewDidEndEditing:textField];
}

- (BOOL)textFieldShouldBeginEditing:(UITextField *)textField {
  return [self inputViewShouldBeginEditing:textField];
}

- (void)textFieldDidChangeSelection:(UITextField *)textField {
  [self emitEvent:@"selection" detail:@{
    @"selectionStart": @(self.view.selectedTextRange ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.start] : -1),
    @"selectionEnd": @(self.view.selectedTextRange ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.end] : -1),
  }];
}


@end
