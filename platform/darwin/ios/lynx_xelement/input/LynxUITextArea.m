// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUITextArea.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxNativeLayoutNode.h>

static NSInteger gTextareaLightTag = 23333;
static CGFloat kLynxTextAreaEpsilonThreshold = 1.0f;
static NSInteger kLynxTextAreaOutOfMaxlines = -1;

@implementation LynxUITextAreaShadowNode

@end

@interface LynxUITextArea () <UITextViewDelegate>
@property (nonatomic, assign) CGFloat preHeight;
@property (nonatomic, strong) NSString *lastValue;
@property (nonatomic, assign) NSInteger maxlines;
@property (nonatomic, strong) UITextView *placeholderView;

@end

@implementation LynxUITextArea

- (instancetype)init {
  if (self = [super init]) {
    _preHeight = -1.0;
    _maxlines = NSIntegerMax;
  }
  return self;
}

- (UITextView *)createView {
  UITextView* textView = [[UITextView alloc] init];
  textView.autoresizesSubviews = NO;
  textView.clipsToBounds = YES;
  textView.delegate = self;
  textView.secureTextEntry = NO;
  textView.backgroundColor = [UIColor clearColor];
  // Default value is 5.0. UITextView(UIScrollView) did not set it to 0.
  textView.textContainer.lineFragmentPadding = 0;

  textView.tag = gTextareaLightTag++;
  
  kLynxTextAreaEpsilonThreshold = UIScreen.mainScreen.scale;
  
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onWillShowKeyboard:) name:UIKeyboardWillShowNotification object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onWillHideKeyboard:) name:UIKeyboardWillHideNotification object:nil];
  
  return textView;
}

LYNX_PROP_SETTER("maxlines", setMaxLines, NSInteger) {
  [[self.view textContainer] setMaximumNumberOfLines:value];
  self.maxlines = value;
}

LYNX_PROP_SETTER("bounces", setBounces, BOOL) {
  self.view.bounces = value;
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
  self.view.textContainerInset = self.padding;
  self.placeholderView.textContainerInset = self.padding;
  self.placeholderView.frame = CGRectMake(0, 0, frame.size.width, frame.size.height);
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  self.view.typingAttributes = self.inputAttrs;
  if (self.placeholder.length) {
    if (!_placeholderView) {
      UITextView* textView = [[UITextView alloc] init];
      textView.autoresizesSubviews = NO;
      textView.clipsToBounds = YES;
      textView.secureTextEntry = NO;
      textView.backgroundColor = [UIColor clearColor];
      textView.textContainer.lineFragmentPadding = 0;
      _placeholderView = textView;
      _placeholderView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
      _placeholderView.userInteractionEnabled = NO;
      _placeholderView.editable = NO;
      [self.view addSubview:_placeholderView];
    }
    _placeholderView.typingAttributes = self.placeholderAttrs;
    _placeholderView.text = self.placeholder;
  } else {
    [_placeholderView removeFromSuperview];
    _placeholderView = nil;
  }
}

- (void)layoutDidFinished {
  __weak __typeof(self) weakSelf = self;
  
  dispatch_async(dispatch_get_main_queue(), ^{
    [weakSelf updateUISize];
    [weakSelf triggerLayoutIfNeeded];
  });
}

LYNX_UI_METHOD(setValue) {
  NSString *value = params[@"value"];
  NSInteger cursor = [(params[@"cursor"] ? : @(-1)) integerValue];
  
  [self.view setText:value];
  
  if (cursor >= 0) {
    UITextPosition* beginning = self.view.beginningOfDocument;
    UITextPosition* cursorPosition = [self.view positionFromPosition:beginning offset:cursor];
    self.view.selectedTextRange = [self.view textRangeFromPosition:cursorPosition toPosition:cursorPosition];
    [self.view scrollRangeToVisible:NSMakeRange(cursor, 0)];
  }
  
  // Trigger update manually, beacuase @selector(textViewDidChange:) will not be triggered
  if (CGRectGetWidth(self.view.frame) == 0 || CGRectGetHeight(self.view.frame) == 0) {
    [self updateUISize];
    [self triggerLayoutIfNeeded];
  }
  
  [self textViewDidChange:self.view];

  callback(kUIMethodSuccess, nil);
}


- (NSString *)getText {
  return self.view.attributedText.string ? : (self.view.text ? : @"");
}

- (NSAttributedString *)getAttributedString {
  return self.view.attributedText;
}

- (UITextPosition *)trimToMaxlines:(UITextView*)textView {
  
  CGRect firstRect = [textView firstRectForRange:[textView textRangeFromPosition:textView.beginningOfDocument toPosition:textView.beginningOfDocument]];
  
  UITextPosition *pos = textView.endOfDocument;
  while ([self calcLines:textView
               firstRect:firstRect
                lastRect:[textView firstRectForRange:[textView textRangeFromPosition:pos toPosition:pos]]] == kLynxTextAreaOutOfMaxlines) {
    UITextPosition * newPos = [textView positionFromPosition:pos offset:-1];
    if (newPos != nil) {
      pos = newPos;
    } else {
      pos = textView.beginningOfDocument;
      break;
    }
  }
  
  if ([textView comparePosition:pos toPosition:textView.beginningOfDocument] != NSOrderedSame) {
    // There is a bug from UIKit here: the position must be incremented by 1 to be correct.
    pos = [textView positionFromPosition:pos offset:1];
  }
  
  return pos;
}

- (NSInteger)calcLines:(UITextView*)textView firstRect:(CGRect)firstRect lastRect:(CGRect)lastRect {
  
  NSInteger line = 1;
  
  if (lastRect.origin.y == INFINITY) {
    // out of max lines
    line = -1;
  } else if (firstRect.origin.y == lastRect.origin.y) {
    // at first line
    line = 1;
  } else {
    CGFloat lineHeightEnumerator = firstRect.origin.y;
    while (lastRect.origin.y - lineHeightEnumerator > 1.0 / UIScreen.mainScreen.scale) {
      line++;
      lineHeightEnumerator += self.inputParagraphStyle.minimumLineHeight ?: self.font.lineHeight + self.inputParagraphStyle.lineSpacing;
    }
  }
  
  if (line > self.maxlines) {
    // For iOS 15, lastRect may be incorrect
    line = kLynxTextAreaOutOfMaxlines;
  }
  
  return line;
  
}

- (void)textViewDidChange:(UITextView*)textView {
  if (![self inputViewDidChange:textView]) {
    self.lastValue = [self getText];
    return;
  }
  self.placeholderView.hidden = textView.text.length != 0;
  self.placeholderView.frame = CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.height);

  if (textView.text.length == 0) {
    textView.contentSize = CGSizeMake(self.view.frame.size.width, self.view.frame.size.height);
    textView.contentOffset = CGPointMake(0, 0);
  }
  
  CGRect firstRect = [textView firstRectForRange:[textView textRangeFromPosition:textView.beginningOfDocument toPosition:textView.beginningOfDocument]];
  CGRect lastRect = [textView firstRectForRange:[textView textRangeFromPosition:textView.endOfDocument toPosition:textView.endOfDocument]];

  NSInteger systemVersion = [[UIDevice currentDevice] systemVersion].integerValue;
  
  if (systemVersion < 16) {
    // For iOS 15, the the calculation of endOfDocument is not correct, use contentSize instead
    UITextPosition *lastPosition = [textView positionFromPosition:textView.endOfDocument offset:-1];
    if (lastPosition) {
      lastRect.origin.y = textView.contentSize.height - firstRect.size.height;
    } else {
      lastRect = firstRect;
    }
  }
  
  if (lastRect.origin.y != _preHeight) {
    
    NSInteger line = [self calcLines:textView firstRect:firstRect lastRect:lastRect];
    
    
    [self emitEvent:@"line" detail:@{
      @"line" : @(line) // include kLynxTextAreaOutOfMaxlines
    }];
        
    if (line == kLynxTextAreaOutOfMaxlines) {
      if (systemVersion < 16) {
        // For iOS 15, forbid this input
        textView.text = self.lastValue;
        textView.selectedTextRange = [textView textRangeFromPosition:textView.endOfDocument toPosition:textView.endOfDocument];
        lastRect.origin.y = _preHeight;
      } else {
        // Try to trim to maxlines
        UITextPosition *pos = [self trimToMaxlines:textView];
        if ([textView comparePosition:pos toPosition:textView.endOfDocument] != NSOrderedSame) {
          UITextRange *filteredRange = [textView textRangeFromPosition:textView.beginningOfDocument toPosition:pos];
          NSString *filteredText = [textView textInRange:filteredRange];
          textView.text = filteredText;
          // Adjust UI
          [textView scrollRangeToVisible:NSMakeRange(filteredText.length, 0)];
          dispatch_async(dispatch_get_main_queue(), ^{
            // Have to update the cursor at next loop (can't be applied on current loop, being restricted by UIKit)
            textView.selectedTextRange = [textView textRangeFromPosition:textView.endOfDocument toPosition:textView.endOfDocument];
          });
          lastRect = [textView firstRectForRange:[textView textRangeFromPosition:textView.endOfDocument toPosition:textView.endOfDocument]];
        }
      }
    }
    [self updateUISize];
    [self triggerLayoutIfNeeded];
    _preHeight = lastRect.origin.y;
  }
  if (![[self getText] isEqualToString:(self.lastValue ? : @"")]) {
    [self sendInputEvent];
  }
  self.lastValue = [self getText];

}

- (void)textViewDidBeginEditing:(UITextView *)textView {
  [self inputViewDidBeginEditing:textView];
  if (textView.text.length == 0) {
    textView.contentSize = CGSizeMake(self.view.frame.size.width, self.view.frame.size.height);
    textView.contentOffset = CGPointMake(0, 0);
  }
}

- (void)textViewDidEndEditing:(UITextView *)textView {
  [self inputViewDidEndEditing:textView];
}

- (void)textViewDidChangeSelection:(UITextView *)textView {
  [self emitEvent:@"selection" detail:@{
    @"selectionStart": @(self.view.selectedTextRange ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.start] : -1),
    @"selectionEnd": @(self.view.selectedTextRange  ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.end] : -1),
  }];
}

- (BOOL)inputViewDidChange:(id<UITextInput>)input {
  if (![self inputView:input checkInputValidity:[self getText]]) {
    return NO;
  }
    
  return YES;
}

- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text {
  if ([text isEqualToString:@"\n"] && textView.returnKeyType != UIReturnKeyDefault) {
    // If the confirm-type is not "default"(next-line), send confirm manually, cause UITextView do not have the callback of `textFieldShouldReturn:`
    [self inputViewShouldReturn:textView];
    return NO;
  }
  
  // The last line needs to be filtered when it is '\n'. This is essentially to be compatible with a bug from UIKit.
  NSArray<NSString *> *currentLines = [textView.text componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet];
  NSArray<NSString *> *comingLines = [text componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet];
  if ( currentLines.count + comingLines.count - 1 > self.maxlines) {
    return NO;
  }
  
  return [self inputView:textView shouldChangeCharactersInRange:range replacementString:text];
}

- (void)inputWillBeFilteredFrom:(NSString *)source to:(NSString *)dest {
  self.view.text = dest;
}

- (NSValue *)getContentSize {
  return @(self.view.contentSize);
}

- (CGSize)adjustViewSize:(CGSize)viewSize {
  if (self.placeholder.length) {
    CGFloat width = self.placeholderView.bounds.size.width ? : UIScreen.mainScreen.bounds.size.width;
    CGSize placeholderSize = [self.placeholderView sizeThatFits:CGSizeMake(width, CGFLOAT_MAX)];
    viewSize.width = MAX(viewSize.width, placeholderSize.width);
    viewSize.height = MAX(viewSize.height, placeholderSize.height);
  }
  return viewSize;
}


@end
