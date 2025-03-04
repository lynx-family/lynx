// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxPageReloadHelper.h"
#import "LynxClassAliasDefines.h"
#import "LynxLog.h"
#import "LynxPageReloadHelper+Internal.h"
#if OS_IOS
#import "LynxTextRendererCache.h"
#endif

#pragma mark - LynxPageReloadHelper
@implementation LynxPageReloadHelper {
  __weak LynxView* _lynxView;

  Boolean _initWithBinary;
  NSData* _binary;
  Boolean _initWithUrl;
  NSString* _url;
  Boolean _initWithBundle;
  LynxTemplateBundle* _bundle;

  LynxTemplateData* _initData;

  NSString* _fileUrl;
  TEXTVIEW_CLASS* _textView;

  NSMutableData* _templateFragments;
  BOOL _ignoreCache;
}

- (nonnull instancetype)initWithLynxView:(LynxView*)view {
  _lynxView = view;

  _initWithBinary = NO;
  _binary = nil;
  _initWithUrl = NO;
  _url = nil;
  _initWithBundle = NO;
  _bundle = nil;

  _initData = nil;

  _textView = nil;

  _templateFragments = nil;
  _ignoreCache = NO;
  return self;
}

- (void)loadFromLocalFile:(NSData*)tem withURL:(NSString*)url initData:(LynxTemplateData*)data {
  _initWithBinary = YES;
  _initWithUrl = NO;
  _initWithBundle = NO;
  _binary = tem;
  _url = url;
  _bundle = nil;

  _initData = data;
  _fileUrl = url;
}

- (void)loadFromURL:(NSString*)url initData:(LynxTemplateData*)data {
  _initWithBinary = NO;
  _initWithUrl = YES;
  _initWithBundle = NO;
  _binary = nil;
  _url = url;
  _bundle = nil;

  _initData = data;
  _fileUrl = url;
}

- (void)loadFromBundle:(LynxTemplateBundle*)bundle
               withURL:(NSString*)url
              initData:(LynxTemplateData*)data {
  _initWithBinary = NO;
  _initWithUrl = NO;
  _initWithBundle = YES;
  _binary = nil;
  _url = url;
  _bundle = bundle;

  _initData = data;
  _fileUrl = url;
}

- (nonnull NSString*)getURL {
  return _fileUrl;
}

- (LynxTemplateData*)getTemplateData {
  return _initData;
}

- (void)reloadLynxView:(BOOL)ignoreCache {
  [self reloadLynxView:ignoreCache withTemplateBin:nil];
}

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(NSString*)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size {
  if (templateBin && templateBin.length > 0) {
    LLogInfo(@"PageReloadHelper: reload with single template binary transferred by usb");
    NSData* binary = [[NSData alloc] initWithBase64EncodedString:templateBin options:0];
    [self reloadLynxView:ignoreCache withTemplateBin:binary];
    return;
  }

  if (fromFragments && size > 0) {
    LLogInfo(@"PageReloadHelper: reload with template fragments transferred by usb");
    _templateFragments = [[NSMutableData alloc] initWithCapacity:size];
    _ignoreCache = ignoreCache;
    return;
  }

  [self reloadLynxView:ignoreCache withTemplateBin:nil];
}

- (void)reloadLynxView:(BOOL)ignoreCache withTemplateBin:(NSData*)binary {
  if (ignoreCache) {
#if OS_IOS
    [[LynxTextRendererCache cache] clearCache];
#endif
  }

  if (_initData) {
    _initData = [_initData deepClone];
  }

  if (_lynxView) {
    // if binary is nil, reload with template or url stored before
    if (!binary && _initWithBinary && ![_url hasPrefix:@"http"]) {
      LLogInfo(@"PageReloadHelper: reload with stored template");
      binary = _binary;
    }
    if (binary) {
      [_lynxView loadTemplate:binary withURL:_url initData:_initData];
    } else if (_initWithBundle && _bundle) {
#if OS_IOS
      LLogInfo(@"PageReloadHelper: reload with template bundle");
      [_lynxView loadTemplateBundle:_bundle withURL:_url initData:_initData];
#endif
    } else {
      LLogInfo(@"PageReloadHelper: reload with url");
      [_lynxView loadTemplateFromURL:_url initData:_initData];
    }
  }
}

- (void)onReceiveTemplateFragment:(NSString*)fragment withEof:(BOOL)eof {
  LLogInfo(@"PageReloadHelper: on receive template fragment");
  if (fragment && fragment.length > 0) {
    NSData* decodedFragment = [[NSData alloc] initWithBase64EncodedString:fragment options:0];
    if (decodedFragment && decodedFragment.length > 0) {
      [_templateFragments appendData:decodedFragment];
    }
  }

  if (eof) {
    LLogInfo(@"PageReloadHelper: end of template fragments");
    if (_templateFragments && _templateFragments.length > 0) {
      [self reloadLynxView:_ignoreCache withTemplateBin:_templateFragments];
    } else {
      [self reloadLynxView:_ignoreCache withTemplateBin:nil];
    }
    _templateFragments = nil;
  }
}

- (void)navigateLynxView:(nonnull NSString*)url {
  _initWithUrl = YES;
  _initWithBinary = NO;
  _initWithBundle = NO;
  _url = url;
  _binary = nil;
  _bundle = nil;

  _fileUrl = url;
  if (_lynxView != nil) {
    [_lynxView loadTemplateFromURL:url];
  }
}

- (void)setTextLable:(NSInteger)label {
#if OS_IOS
  _textView = [[UITextView alloc] init];
  _textView.text = @(label).stringValue;
  _textView.textColor = [UIColor whiteColor];
  _textView.textContainer.lineFragmentPadding = 0;
  [_textView sizeToFit];
  _textView.frame = CGRectMake(0, 0, _textView.contentSize.width, _textView.contentSize.height);
  _textView.backgroundColor = [[UIColor alloc] initWithRed:0 green:0 blue:0 alpha:0.5];
  if (_lynxView != nil) {
    [_lynxView insertSubview:_textView atIndex:0];
  }
#elif OS_OSX
  _textView = [[NSTextView alloc] init];
  _textView.string = @(label).stringValue;
  _textView.textColor = [NSColor whiteColor];
  _textView.textContainer.lineFragmentPadding = 0;
  [_textView sizeToFit];
  _textView.backgroundColor = [NSColor colorWithRed:0 green:0 blue:0 alpha:0.5];
  if (_lynxView != nil) {
    [_textView removeFromSuperview];
    [_lynxView addSubview:_textView positioned:NSWindowAbove relativeTo:nil];
  }
#endif
}

- (void)removeTextLabel {
  [_textView removeFromSuperview];
  _textView = nil;
}

- (void)popTextLabel {
#if OS_IOS
  if (_textView != nil && _lynxView != nil) {
    [_lynxView bringSubviewToFront:_textView];
  }
#elif OS_OSX
  if (_textView != nil && _lynxView != nil) {
    [_textView removeFromSuperview];
    [_lynxView addSubview:_textView positioned:NSWindowAbove relativeTo:nil];
  }
#endif
}

- (void)attachLynxView:(nonnull LynxView*)lynxView {
  _lynxView = lynxView;
}

- (NSString*)getTemplateJsInfo:(uint32_t)offset withSize:(uint32_t)size {
  if (_binary && offset < _binary.length) {
    const uint32_t length = static_cast<uint32_t>(_binary.length);
    size = (offset + size) > length ? length - offset : size;
    NSData* content = [_binary subdataWithRange:NSMakeRange(offset, size)];
    return [content base64EncodedStringWithOptions:0];
  }
  return nil;
}

- (void)onTemplateLoadSuccess:(nullable NSData*)tem {
  _binary = tem;
}

@end
