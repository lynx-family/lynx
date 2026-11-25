// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTransferManager.h"
#import "LynxTransferredView+Internal.h"

#import <Lynx/LynxTransferListener.h>
#import <Lynx/LynxTransferWrapperView.h>

static NSString* const kLynxManagedTransferIdPrefix = @"__lynx_sdk_transfer_";

@interface LynxTransferRecord : NSObject

@property(nonatomic, copy) NSString* transferId;
@property(nonatomic, strong) UIView* view;
@property(nonatomic, copy) NSDictionary* dataset;
@property(nonatomic, weak, nullable) id<LynxTransferListener> listener;

@end

@implementation LynxTransferRecord
@end

@interface LynxTransferManager ()

@property(nonatomic, strong) NSMutableDictionary<NSString*, LynxTransferredView*>* managed;

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
    _managed = [NSMutableDictionary dictionary];
  }
  return self;
}

- (LynxTransferredView*)loadTransfer:(NSString*)componentName
                        initialProps:(NSDictionary*)props
                              client:(id<LynxTransferClient>)client
                            dispatch:(void (^)(NSString*, NSDictionary*))dispatch {
  NSString* transferId =
      [kLynxManagedTransferIdPrefix stringByAppendingString:NSUUID.UUID.UUIDString];
  __weak LynxTransferManager* weakSelf = self;
  LynxTransferredView* card =
      [[LynxTransferredView alloc] initWithId:transferId
                                       client:client
                                     dispatch:dispatch
                                   unregister:^{
                                     [weakSelf.managed removeObjectForKey:transferId];
                                   }];
  self.managed[transferId] = card;
  [card startComponent:componentName initialProps:props];
  return card;
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
    [record.view removeFromSuperview];
    BOOL accepted = [listener onCreate:record.transferId view:record.view dataset:record.dataset];
    if (self.pendingTransfers[key] != record) {
      continue;
    }
    if (accepted) {
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
  [self.activeTransfers removeObjectForKey:key];
  [self.pendingTransfers removeObjectForKey:key];
  [view removeFromSuperview];
  if ([transferId hasPrefix:kLynxManagedTransferIdPrefix]) {
    [self.managed[transferId] bindContent:view];
    return;
  }
  LynxTransferRecord* record = [LynxTransferRecord new];
  record.transferId = transferId;
  record.view = view;
  record.dataset = dataset;

  self.pendingTransfers[key] = record;

  NSArray<id<LynxTransferListener>>* listeners = self.transferListeners.array;
  for (id<LynxTransferListener> listener in listeners) {
    BOOL accepted = [listener onCreate:transferId view:view dataset:dataset];
    if (self.pendingTransfers[key] != record) {
      return;
    }
    if (accepted) {
      record.listener = listener;
      self.activeTransfers[key] = record;
      [self.pendingTransfers removeObjectForKey:key];
      return;
    }
  }
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

- (void)dispatchTransferIntrinsicContentSizeChange:(LynxTransferWrapperView*)view {
  if ([view.superview isKindOfClass:LynxTransferredView.class]) {
    [view.superview invalidateIntrinsicContentSize];
    return;
  }
  LynxTransferRecord* record = self.activeTransfers[[self transferKeyForView:view]];
  if (record.listener != nil && [self.transferListeners containsObject:record.listener] &&
      [record.listener respondsToSelector:@selector(transferViewDidChangeIntrinsicContentSize:)]) {
    [record.listener transferViewDidChangeIntrinsicContentSize:view];
  }
}

- (void)dispatchTransferRemove:(NSString*)transferId view:(UIView*)view {
  if ([transferId hasPrefix:kLynxManagedTransferIdPrefix]) {
    LynxTransferredView* card = self.managed[transferId];
    [card detachContent:view];
    return;
  }
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
  [view removeFromSuperview];
}

- (void)clearTransfers {
  for (LynxTransferredView* card in self.managed.allValues) {
    [card dispose:LynxTransferReleaseReasonOwnerDestroy];
  }
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
    [record.view removeFromSuperview];
  }
  [self.transferListeners removeAllObjects];
}

- (NSString*)transferKeyForView:(UIView*)view {
  return [NSString stringWithFormat:@"%p", view];
}

@end
