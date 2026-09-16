// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxMarkdownEventDispatcher.h"

static NSString *LynxMarkdownOverflowType(ServalMarkdownTextOverflow overflow) {
  return overflow == kServalMarkdownTextOverflowEllipsis ? @"ellipsis" : @"clip";
}

static NSString *LynxMarkdownHandleType(ServalMarkdownSelectionHandleType handle) {
  return handle == kServalMarkdownSelectionHandleTypeRight ? @"right" : @"left";
}

static NSString *LynxMarkdownSelectionState(ServalMarkdownSelectionState state) {
  switch (state) {
    case kServalMarkdownSelectionStateEnter:
      return @"enter";
    case kServalMarkdownSelectionStateMove:
      return @"move";
    case kServalMarkdownSelectionStateStop:
      return @"stop";
    case kServalMarkdownSelectionStateExit:
      return @"exit";
  }
  return @"enter";
}

@implementation LynxMarkdownEventDispatcher {
  __weak id<LynxMarkdownEventDispatcherHost> _host;
  NSInteger _lastSelectionStart;
  NSInteger _lastSelectionEnd;
}

- (instancetype)initWithHost:(id<LynxMarkdownEventDispatcherHost>)host {
  self = [super init];
  if (self != nil) {
    _host = host;
    _lastSelectionStart = -1;
    _lastSelectionEnd = -1;
  }
  return self;
}

- (void)onParseEnd {
  id<LynxMarkdownEventDispatcherHost> host = _host;
  if (host == nil) {
    return;
  }
  [host dispatchMarkdownEvent:@"parseEnd"
                       detail:@{
                         @"id" : [host markdownParseEndContentID] ?: @"",
                       }];
}

- (void)onTextOverflow:(ServalMarkdownTextOverflow)overflow {
  [_host dispatchMarkdownEvent:@"overflow"
                        detail:@{
                          @"type" : LynxMarkdownOverflowType(overflow),
                        }];
}

- (void)onDrawStart {
  [_host dispatchMarkdownEvent:@"drawStart" detail:nil];
}

- (void)onDrawEnd {
  [_host dispatchMarkdownEvent:@"drawEnd" detail:nil];
}

- (void)onAnimationStep:(NSInteger)animationStep MaxAnimationStep:(NSInteger)maxAnimationStep {
  id<LynxMarkdownEventDispatcherHost> host = _host;
  if (host == nil) {
    return;
  }
  [host dispatchMarkdownEvent:@"animationStep"
                       detail:@{
                         @"animationStep" : @(animationStep),
                         @"maxAnimationStep" : @(maxAnimationStep),
                         @"contentLength" : @([host markdownContentLength]),
                       }];
}

- (void)onLinkClicked:(nonnull NSString *)url Content:(nonnull NSString *)content {
  [_host dispatchMarkdownEvent:@"link"
                        detail:@{
                          @"url" : url ?: @"",
                          @"content" : content ?: @"",
                        }];
}

- (void)onImageClicked:(nonnull NSString *)url {
  [_host dispatchMarkdownEvent:@"imageTap"
                        detail:@{
                          @"url" : url ?: @"",
                        }];
}

- (void)onTextClicked:(NSString *)identifier {
  [_host dispatchMarkdownEvent:@"textClick" detail:@{@"id" : identifier}];
}

- (void)onSelectionChanged:(NSInteger)startIndex
                  EndIndex:(NSInteger)endIndex
                    Handle:(ServalMarkdownSelectionHandleType)handle
                     State:(ServalMarkdownSelectionState)state {
  BOOL forward = _lastSelectionStart == -1
                     ? startIndex < endIndex
                     : _lastSelectionStart < startIndex || _lastSelectionEnd < endIndex;
  _lastSelectionStart = startIndex;
  _lastSelectionEnd = endIndex;
  [_host dispatchMarkdownEvent:@"selectionchange"
                        detail:@{
                          @"start" : @(MIN(startIndex, endIndex)),
                          @"end" : @(MAX(startIndex, endIndex)),
                          @"direction" : forward ? @"forward" : @"backward",
                          @"handle" : @(handle),
                          @"state" : @(state),
                          @"handleType" : LynxMarkdownHandleType(handle),
                          @"selectionState" : LynxMarkdownSelectionState(state),
                        }];
}

@end
