// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxLog.h>
#import "LynxTransferredView+Internal.h"

@interface LynxTransferredView ()
@property(nonatomic, copy) dispatch_block_t unregisterCard;
@property(nonatomic, copy) NSString* transferId;
@property(nonatomic, copy) void (^dispatchCommand)(NSString*, NSDictionary*);
@property(nonatomic, strong) id<LynxTransferClient> client;
@property(nonatomic, strong) UIView* content;
@property(nonatomic, copy) NSDictionary* pendingProps;
@property(nonatomic) BOOL created;
@property(nonatomic) BOOL boundOnce;
@property(nonatomic) BOOL drewOnce;
@property(nonatomic) BOOL firstScreenScheduled;
@end

@implementation LynxTransferredView
- (instancetype)initWithId:(NSString*)transferId
                    client:(id<LynxTransferClient>)client
                  dispatch:(void (^)(NSString*, NSDictionary*))dispatch
                unregister:(dispatch_block_t)unregisterCard {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    _transferId = [transferId copy];
    _client = client;
    _dispatchCommand = [dispatch copy];
    _unregisterCard = [unregisterCard copy];
  }
  return self;
}
- (void)startComponent:(NSString*)componentName initialProps:(NSDictionary*)props {
  @try {
    if ([_client respondsToSelector:@selector(onLoadStart:)]) [_client onLoadStart:self];
    if (self.dispatchCommand) {
      self.created = YES;
      _dispatchCommand(@"__CreateTransfer", @{
        @"transferId" : self.transferId,
        @"componentName" : componentName,
        @"initialProps" : self.pendingProps ?: props
      });
      self.pendingProps = nil;
    }
  } @catch (NSException* exception) {
    [self dispose:LynxTransferReleaseReasonLoadFailed];
    @throw;
  }
}
- (void)detachContent:(UIView*)content {
  if (_content == content) [self bindContent:nil];
}
- (void)updateProps:(NSDictionary*)props {
  [self dispatchProps:props type:@"__UpdateTransfer"];
}
- (void)reload:(NSDictionary*)props {
  [self dispatchProps:props type:@"__ReloadTransfer"];
}
- (void)dispatchProps:(NSDictionary*)props type:(NSString*)type {
  NSAssert(NSThread.isMainThread, @"Transfer requires the main thread");
  if (!_dispatchCommand) {
    _LogW(@"Ignoring %@ for released transfer %@", type, _transferId);
  } else if (!_created) {
    self.pendingProps = props;
  } else {
    _dispatchCommand(type, @{@"transferId" : _transferId, @"props" : props});
  }
}
- (void)clearForDestroy {
  NSAssert(NSThread.isMainThread, @"Transfer requires the main thread");
  [self dispose:LynxTransferReleaseReasonExplicitRelease];
}
- (void)dispose:(LynxTransferReleaseReason)reason {
  if (!_dispatchCommand) return;
  void (^dispatch)(NSString*, NSDictionary*) = _dispatchCommand;
  _dispatchCommand = nil;
  _unregisterCard();
  _unregisterCard = nil;
  self.pendingProps = nil;
  [self bindContent:nil];
  id<LynxTransferClient> client = _client;
  _client = nil;
  if ([client respondsToSelector:@selector(onReleased:reason:)])
    [client onReleased:self reason:reason];
  if (_created && reason == LynxTransferReleaseReasonExplicitRelease) {
    dispatch(@"__DestroyTransfer", @{@"transferId" : _transferId});
  }
}
- (void)bindContent:(UIView*)content {
  [_content removeFromSuperview];
  _content = content;
  [self invalidateIntrinsicContentSize];
  if (!content) return;
  content.frame = self.bounds;
  content.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [self addSubview:content];
  if (!_boundOnce) {
    _boundOnce = YES;
    if ([_client respondsToSelector:@selector(onLoadSuccess:)]) [_client onLoadSuccess:self];
  }
  [self scheduleFirstScreenIfNeeded];
}
- (CGSize)intrinsicContentSize {
  return _content ? _content.intrinsicContentSize : [super intrinsicContentSize];
}
- (void)didMoveToWindow {
  [super didMoveToWindow];
  [self scheduleFirstScreenIfNeeded];
}
- (void)layoutSubviews {
  [super layoutSubviews];
  [self scheduleFirstScreenIfNeeded];
}
- (void)scheduleFirstScreenIfNeeded {
  if (_drewOnce || _firstScreenScheduled || !self.window || _content.superview != self ||
      CGRectIsEmpty(self.bounds)) {
    return;
  }
  _firstScreenScheduled = YES;
  __weak typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) return;
    strongSelf->_firstScreenScheduled = NO;
    if (strongSelf->_drewOnce || !strongSelf.window ||
        strongSelf->_content.superview != strongSelf || CGRectIsEmpty(strongSelf.bounds)) {
      return;
    }
    strongSelf->_drewOnce = YES;
    if ([strongSelf->_client respondsToSelector:@selector(onFirstScreen:)]) {
      [strongSelf->_client onFirstScreen:strongSelf];
    }
  });
}
@end
