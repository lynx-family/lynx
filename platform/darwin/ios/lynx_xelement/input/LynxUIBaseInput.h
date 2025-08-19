// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI.h>
NS_ASSUME_NONNULL_BEGIN

@interface LynxUIBaseInput<__covariant V : UIView<UITextInput>*> : LynxUI

@property (nonatomic, assign) CGFloat placeholderFontSize;
@property (nonatomic, strong) NSString *fontFamily;
@property (nonatomic, strong) NSString *placeholderFontFamily;
@property (nonatomic, assign) LynxFontStyleType fontStyle;
@property (nonatomic, assign) LynxFontStyleType placeholderFontStyle;
@property (nonatomic, assign) UIFontWeight fontWeight;
@property (nonatomic, assign) UIFontWeight placeholderFontWeight;
@property (nonatomic, strong) NSMutableDictionary *inputAttrs;
@property (nonatomic, strong) NSMutableParagraphStyle *inputParagraphStyle;
@property (nonatomic, strong) NSMutableDictionary *placeholderAttrs;

@property (nonatomic, assign) NSInteger maxLength;
@property (nonatomic, assign) BOOL readonly;
@property (nonatomic, assign) BOOL sendComposingInputEvent;
@property (nonatomic, assign) BOOL confirmEnter;

@property (nonatomic, strong) UIFont *font;

@property (nonatomic, strong) UIFont *placeholderFont;

@property (nonatomic, strong) NSString *placeholder;


- (nonnull V)view;

- (NSTextAlignment)textAlignment;

- (NSString *)getText;

- (void)inputWillBeFilteredFrom:(NSString *)source to:(NSString *)dest;

- (BOOL)isComposing;

- (BOOL)inputViewShouldReturn:(id<UITextInput>)input;

- (BOOL)inputViewDidChange:(id<UITextInput>)input;

- (BOOL)inputView:(id<UITextInput>)input shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string ;

- (void)inputViewDidBeginEditing:(id<UITextInput>)input ;

- (void)inputViewDidEndEditing:(id<UITextInput>)input ;

- (BOOL)inputViewShouldBeginEditing:(id<UITextInput>)input ;

- (void)emitEvent:(NSString*) name detail:(NSDictionary*)detail;

- (void)onWillShowKeyboard:(NSNotification *)notification;

- (void)onWillHideKeyboard:(NSNotification *)notification;

- (NSAttributedString *)getAttributedString;

- (void)triggerLayoutIfNeeded;

- (CGSize)adjustViewSize:(CGSize)viewSize;

- (void)updateUISize;

- (NSValue *)getContentSize;

- (BOOL)inputView:(id<UITextInput>)input checkInputValidity:(NSString *)source;

- (void)sendInputEvent;

@end

NS_ASSUME_NONNULL_END
