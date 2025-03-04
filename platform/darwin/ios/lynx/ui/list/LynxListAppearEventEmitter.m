// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListAppearEventEmitter.h"
#import "LynxUICollection+Internal.h"

NSString* const kLynxListNodeAppearEvent = @"nodeappear";
NSString* const kLynxListNodeDisappearEvent = @"nodedisappear";

@interface LynxListAppearEventEmitter ()
@property(nonatomic, readonly) LynxEventEmitter* emitter;
@end

@interface LynxAppearEvent : LynxDetailEvent

@property BOOL valid;

@end

@implementation LynxAppearEvent

- (instancetype)initWithName:(NSString*)name
                  targetSign:(NSInteger)target
                      detail:(NSDictionary*)detail {
  return [super initWithName:name targetSign:target detail:detail];
}

@end

@implementation LynxListAppearEventEmitter {
  NSMutableDictionary* _flushingEvent;
  NSMutableDictionary* _pendingEvent;
  NSTimer* _timer;
  __weak LynxUICollection* _listUI;
}

- (instancetype)initWithEmitter:(LynxEventEmitter*)emitter {
  if (self = [super init]) {
    _emitter = emitter;
    _flushingEvent = [NSMutableDictionary dictionary];
    _pendingEvent = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)setListUI:(LynxUICollection*)ui {
  _listUI = ui;
}

- (void)startTimerIfNeeded {
  if (!_timer.isValid) {
    CGFloat const interval = 0.01;  // send event per 10 ms.
    _timer = [NSTimer timerWithTimeInterval:interval
                                     target:self
                                   selector:@selector(flush)
                                   userInfo:nil
                                    repeats:YES];
    [[NSRunLoop mainRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
  }
}

- (NSString*)eventKeyForUI:(LynxUIComponent*)ui atIndexPath:(NSIndexPath*)indexPath {
  if (ui.itemKey) {
    return ui.itemKey;
  }
  return [NSString stringWithFormat:@"%ld-%ld", (long)indexPath.section, (long)indexPath.row];
}

- (LynxDetailEvent*)eventWithName:(NSString*)name
                               ui:(LynxUIComponent*)ui
                              key:(NSString*)key
                         position:(NSInteger)position {
  NSDictionary* detail = @{
    @"position" : @(position),
    @"key" : key,
  };
  LynxAppearEvent* event = [[LynxAppearEvent alloc] initWithName:name
                                                      targetSign:ui.sign
                                                          detail:detail];
  event.valid = ([ui.eventSet objectForKey:name] != nil);
  return event;
}

- (void)onCellAppear:(LynxUIComponent*)ui atIndexPath:(NSIndexPath*)indexPath {
  if (_listUI.needsInternalCellAppearNotification) {
    [ui onListCellAppear:ui.itemKey withList:_listUI];
  }
  NSString* key = [self eventKeyForUI:ui atIndexPath:indexPath];
  LynxCustomEvent* event = [_flushingEvent objectForKey:key];
  if (event != nil) {
    LYNX_LIST_DEBUG_COND_LOG([event.eventName isEqualToString:kLynxListNodeDisappearEvent],
                             @"Duplicated appear event found");
    if (_listUI && _listUI.debugInfoLevel >= LynxListDebugInfoLevelError &&
        [_listUI shouldGenerateDebugInfo]) {
      [_listUI sendListDebugInfoEvent:@"Duplicated appear event found"];
    }
    [_flushingEvent removeObjectForKey:key];
    return;
  }
  event = [_pendingEvent objectForKey:key];
  if (event != nil) {
    LYNX_LIST_DEBUG_COND_LOG([event.eventName isEqualToString:kLynxListNodeDisappearEvent],
                             @"Duplicated appear event found");
    if (_listUI && _listUI.debugInfoLevel >= LynxListDebugInfoLevelError &&
        [_listUI shouldGenerateDebugInfo]) {
      [_listUI sendListDebugInfoEvent:@"Duplicated appear event found"];
    }
    [_pendingEvent removeObjectForKey:key];
    return;
  }
  event = [self eventWithName:kLynxListNodeAppearEvent ui:ui key:key position:indexPath.row];
  [_pendingEvent setObject:event forKey:key];
  [self startTimerIfNeeded];
}

- (void)onCellDisappear:(LynxUIComponent*)ui atIndexPath:(NSIndexPath*)indexPath {
  if (_listUI.needsInternalCellDisappearNotification) {
    [ui onListCellDisappear:ui.itemKey
                      exist:[_listUI.currentItemKeys containsObject:ui.itemKey]
                   withList:_listUI];
  }
  NSString* key = [self eventKeyForUI:ui atIndexPath:indexPath];
  LynxCustomEvent* event = [_flushingEvent objectForKey:key];
  if (event != nil) {
    LYNX_LIST_DEBUG_COND_LOG([event.eventName isEqualToString:kLynxListNodeAppearEvent],
                             @"Duplicated disappear event found");
    if (_listUI && _listUI.debugInfoLevel >= LynxListDebugInfoLevelError &&
        [_listUI shouldGenerateDebugInfo]) {
      [_listUI sendListDebugInfoEvent:@"Duplicated disappear event found"];
    }
    [_flushingEvent removeObjectForKey:key];
    return;
  }
  event = [_pendingEvent objectForKey:key];
  if (event != nil) {
    LYNX_LIST_DEBUG_COND_LOG([event.eventName isEqualToString:kLynxListNodeAppearEvent],
                             @"Duplicated disappear event found");
    if (_listUI && _listUI.debugInfoLevel >= LynxListDebugInfoLevelError &&
        [_listUI shouldGenerateDebugInfo]) {
      [_listUI sendListDebugInfoEvent:@"Duplicated disappear event found"];
    }
    [_pendingEvent removeObjectForKey:key];
    return;
  }
  event = [self eventWithName:kLynxListNodeDisappearEvent ui:ui key:key position:indexPath.row];
  [_pendingEvent setObject:event forKey:key];
  [self startTimerIfNeeded];
}

- (void)flush {
  for (LynxAppearEvent* event in _flushingEvent.allValues) {
    if (event.valid) {
      [_emitter sendCustomEvent:event];
    }
  }
  _flushingEvent = _pendingEvent;
  _pendingEvent = [NSMutableDictionary dictionary];
  if (_flushingEvent.count == 0) {
    [_timer invalidate];
  }
}

- (void)invalidate {
  [_timer invalidate];
}

@end
