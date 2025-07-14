// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

@class LynxView;
@protocol LUIBodyView;

/// can be used for key in Dictionary, but must be clear before LynxView's dealloc, which means it's
/// lifecycle is smaller than LynxView's Lifecycle.
@interface LynxScrollInfo : NSObject <NSCopying>

@property(nonatomic, weak) UIView<LUIBodyView> *_Nullable lynxView;

/// Tag name of LynxUI.
@property(nonatomic, copy) NSString *_Nullable tagName;
/// Tag name  specified by web.
@property(nonatomic, copy) NSString *_Nullable scrollMonitorTagName;

// lynxView is weak, so we need presistent URL to judge whether two ScrollInfo generate from same
// LynxView.
@property(nonatomic, readonly) NSString *_Nullable lynxViewUrl;

@property(nonatomic, weak) UIScrollView *_Nullable scrollView;

@property(nonatomic, assign) SEL _Nullable selector;

// used only for selector `scrollerDidEndDragging:willDecelerate:`
@property(nonatomic, assign) BOOL decelerate;

+ (instancetype _Nullable)infoWithScrollView:(UIScrollView *_Nullable)scrollView
                                     tagName:(NSString *_Nullable)tagName
                        scrollMonitorTagName:(NSString *_Nullable)scrollMonitorTagName;

@end

@protocol LynxScrollListener <NSObject>

@optional
- (void)scrollerDidScroll:(nullable LynxScrollInfo *)info;
- (void)scrollerWillBeginDragging:(nullable LynxScrollInfo *)info;
- (void)scrollerDidEndDragging:(nullable LynxScrollInfo *)info willDecelerate:(BOOL)decelerate;
- (void)scrollerDidEndDecelerating:(nullable LynxScrollInfo *)info;
- (void)scrollerDidEndScrollingAnimation:(nullable LynxScrollInfo *)info;

@end
