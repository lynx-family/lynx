// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTransferManager.h"

#import <Lynx/LynxTransferListener.h>

@interface LynxTransferRecord : NSObject

@property(nonatomic, copy) NSString* transferId;
@property(nonatomic, strong) UIView* view;
@property(nonatomic, copy) NSDictionary* dataset;
@property(nonatomic, weak, nullable) id<LynxTransferListener> listener;

@end

@implementation LynxTransferRecord
@end

@interface LynxTransferManager ()

@property(nonatomic, strong) NSMutableOrderedSet<id<LynxTransferListener>>* transferListeners;
@property(nonatomic, strong) NSMutableDictionary<NSString*, LynxTransferRecord*>* activeTransfers;
@property(nonatomic, strong) NSMutableDictionary<NSString*, LynxTransferRecord*>* pendingTransfers;

@end

@implementation LynxTransferManager

- (instancetype)init {
  self = [super init];
  if (self) {
    _transferListeners = [NSMutableOrderedSet orderedSet];
    _activeTransfers = [NSMutableDictionary dictionary];
    _pendingTransfers = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)registerTransferListener:(id<LynxTransferListener>)listener {
  if (listener == nil) {
    return;
  }
  [self.transferListeners addObject:listener];

  NSArray<NSString*>* keys = [self.pendingTransfers.allKeys copy];
  for (NSString* key in keys) {
    LynxTransferRecord* record = self.pendingTransfers[key];
    if (record == nil) {
      continue;
    }
    [self removeFromParent:record.view];
    if ([listener onCreate:record.transferId view:record.view dataset:record.dataset]) {
      record.listener = listener;
      self.activeTransfers[key] = record;
      [self.pendingTransfers removeObjectForKey:key];
    }
  }
}

- (void)unregisterTransferListener:(id<LynxTransferListener>)listener {
  if (listener == nil) {
    return;
  }
  [self.transferListeners removeObject:listener];
}

- (void)dispatchTransferCreate:(NSString*)transferId
                          view:(UIView*)view
                       dataset:(NSDictionary*)dataset {
  if (transferId.length == 0 || view == nil) {
    return;
  }

  NSString* key = [self transferKeyForView:view];
  LynxTransferRecord* record = [LynxTransferRecord new];
  record.transferId = transferId;
  record.view = view;
  record.dataset = dataset;

  [self.activeTransfers removeObjectForKey:key];
  [self.pendingTransfers removeObjectForKey:key];
  [self removeFromParent:view];

  NSArray<id<LynxTransferListener>>* listeners = self.transferListeners.array;
  for (id<LynxTransferListener> listener in listeners) {
    if ([listener onCreate:transferId view:view dataset:dataset]) {
      record.listener = listener;
      self.activeTransfers[key] = record;
      return;
    }
  }

  self.pendingTransfers[key] = record;
}

- (void)dispatchTransferDatasetUpdate:(UIView*)view dataset:(NSDictionary*)dataset {
  if (view == nil) {
    return;
  }

  NSString* key = [self transferKeyForView:view];
  LynxTransferRecord* record = self.activeTransfers[key] ?: self.pendingTransfers[key];
  if (record == nil) {
    return;
  }
  record.dataset = dataset;
  if (record.listener != nil && [self.transferListeners containsObject:record.listener]) {
    [record.listener onDatasetUpdate:record.transferId dataset:dataset];
  }
}

- (void)dispatchTransferRemove:(NSString*)transferId view:(UIView*)view {
  if (view == nil) {
    return;
  }

  NSString* key = [self transferKeyForView:view];
  LynxTransferRecord* activeRecord = self.activeTransfers[key];
  [self.activeTransfers removeObjectForKey:key];
  [self.pendingTransfers removeObjectForKey:key];
  if (activeRecord.listener != nil &&
      [self.transferListeners containsObject:activeRecord.listener]) {
    [activeRecord.listener onRemove:transferId view:view];
  }
  [self removeFromParent:view];
}

- (void)clearTransfers {
  NSMutableArray<LynxTransferRecord*>* records =
      [NSMutableArray arrayWithArray:self.activeTransfers.allValues];
  [records addObjectsFromArray:self.pendingTransfers.allValues];
  [self.activeTransfers removeAllObjects];
  [self.pendingTransfers removeAllObjects];

  NSArray<id<LynxTransferListener>>* listeners = self.transferListeners.array;
  for (LynxTransferRecord* record in records) {
    if (record.listener != nil && [listeners containsObject:record.listener]) {
      [record.listener onRemove:record.transferId view:record.view];
    }
    [self removeFromParent:record.view];
  }
  [self.transferListeners removeAllObjects];
}

- (NSString*)transferKeyForView:(UIView*)view {
  return [NSString stringWithFormat:@"%p", view];
}

- (void)removeFromParent:(UIView*)view {
  [view removeFromSuperview];
}

@end
