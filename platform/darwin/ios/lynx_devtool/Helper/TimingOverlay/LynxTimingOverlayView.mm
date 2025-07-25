// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFSPTracer+Internal.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxUIOwner+Private.h>
#import <Lynx/LynxView.h>
#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>
#import <LynxDevtool/LynxDevToolEnv.h>
#import <LynxDevtool/LynxTimingOverlayHelper.h>
#import <LynxDevtool/LynxTimingOverlayView.h>
#import <UIKit/UIKit.h>

#pragma mark - LynxTimingOverlayView

@interface LynxTimingOverlayView ()

@property(nonatomic, strong) UILabel *overlayInfo;
@property(nonatomic, strong) UIView *highlightBorder;

@property(nonatomic, assign) BOOL needsUpdate;
@property(nonatomic, weak) id<LynxTimingOverlayViewDataSource> dataSource;

@property(nonatomic, copy) dispatch_block_t singleTapCallback;
@property(nonatomic, copy) dispatch_block_t doubleTapCallback;

@end

@implementation LynxTimingOverlayView

- (instancetype)initWithFrame:(CGRect)frame
                   dataSource:(nonnull id<LynxTimingOverlayViewDataSource>)dataSource {
  self = [super initWithFrame:frame];
  if (self) {
    _needsUpdate = NO;
    _dataSource = dataSource;

    self.translatesAutoresizingMaskIntoConstraints = NO;
    self.userInteractionEnabled = NO;
    self.hidden = YES;

    _overlayInfo = [[UILabel alloc] initWithFrame:CGRectZero];
    _overlayInfo.translatesAutoresizingMaskIntoConstraints = NO;
    _overlayInfo.userInteractionEnabled = NO;
    _overlayInfo.textAlignment = NSTextAlignmentLeft;
    _overlayInfo.textColor = [UIColor whiteColor];
    _overlayInfo.font = [UIFont systemFontOfSize:12];
    _overlayInfo.numberOfLines = 0;

    UIView *infoBG = [[UIView alloc] init];
    infoBG.backgroundColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.25];
    infoBG.translatesAutoresizingMaskIntoConstraints = NO;
    infoBG.userInteractionEnabled = NO;

    [self addSubview:infoBG];
    [self addSubview:_overlayInfo];

    // Info label: align to top-left corner
    [NSLayoutConstraint activateConstraints:@[
      [_overlayInfo.topAnchor constraintEqualToAnchor:self.topAnchor constant:24.0],
      [_overlayInfo.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:24.0],
      [_overlayInfo.trailingAnchor constraintLessThanOrEqualToAnchor:self.trailingAnchor
                                                            constant:-24.0],
      [_overlayInfo.bottomAnchor constraintLessThanOrEqualToAnchor:self.bottomAnchor constant:-24.0]
    ]];

    // Info background: align to info label plus padding 12.0
    [NSLayoutConstraint activateConstraints:@[
      [infoBG.topAnchor constraintEqualToAnchor:_overlayInfo.topAnchor constant:-12.0],
      [infoBG.leadingAnchor constraintEqualToAnchor:_overlayInfo.leadingAnchor constant:-12.0],
      [infoBG.trailingAnchor constraintEqualToAnchor:_overlayInfo.trailingAnchor constant:12.0],
      [infoBG.bottomAnchor constraintEqualToAnchor:_overlayInfo.bottomAnchor constant:12.0]
    ]];
  }
  return self;
}

- (void)setNeedsUpdate {
  if (pthread_main_np()) {
    _needsUpdate = YES;
    [self setNeedsLayout];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      self.needsUpdate = YES;
      [self setNeedsLayout];
    });
  }
}

- (void)layoutSubviews {
  [super layoutSubviews];

  if (_needsUpdate) {
    [self updateData];
    _needsUpdate = NO;
  }
}

- (void)updateData {
  // Update visibility based on FSP state
  if (self.dataSource.overlayViewIsHidden) {
    self.hidden = YES;
  } else {
    self.hidden = NO;
  }

  // Update background color based on FSP state
  self.backgroundColor =
      [self.dataSource overlayViewBackgroundColor] ?: [UIColor colorWithWhite:1.0 alpha:0.25];

  // Update label text
  self.overlayInfo.text =
      [[self.dataSource overlayViewDescriptionLines] componentsJoinedByString:@"\n"];

  // Update highlight
  [self setHighlightRect:[self.dataSource overlayViewHighlightRect]];
}

- (void)setHighlightRect:(CGRect)rect {
  if (CGRectIsNull(rect)) {
    _highlightBorder.hidden = YES;
    return;
  }

  if (!_highlightBorder) {
    _highlightBorder = [[UIView alloc] init];
    _highlightBorder.userInteractionEnabled = NO;
    _highlightBorder.layer.borderColor =
        [UIColor colorWithRed:0.5 green:0.0 blue:0.0 alpha:0.5].CGColor;
    _highlightBorder.layer.borderWidth = 2.0;
    [self addSubview:_highlightBorder];
  }

  _highlightBorder.frame = rect;
  _highlightBorder.hidden = NO;
  [self bringSubviewToFront:_highlightBorder];
}

// Replaces previous implementation
- (void)installTapGestureRecognizer:(dispatch_block_t)tapOnce twice:(dispatch_block_t)tapTwice {
  self.userInteractionEnabled = YES;

  // Store the blocks
  self.singleTapCallback = tapOnce;
  self.doubleTapCallback = tapTwice;

  if (self.gestureRecognizers.count > 0) {
    return;
  }

  // Create gesture recognizers
  UITapGestureRecognizer *singleTap =
      [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleSingleTap:)];
  UITapGestureRecognizer *doubleTap =
      [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleDoubleTap:)];
  doubleTap.numberOfTapsRequired = 2;

  // Ensure single‑tap waits for the possibility of a double‑tap
  [singleTap requireGestureRecognizerToFail:doubleTap];

  // Add to view
  [self addGestureRecognizer:singleTap];
  [self addGestureRecognizer:doubleTap];
}

- (void)handleSingleTap:(UITapGestureRecognizer *)recognizer {
  if (self.singleTapCallback) {
    self.singleTapCallback();
  }
}

- (void)handleDoubleTap:(UITapGestureRecognizer *)recognizer {
  if (self.doubleTapCallback) {
    self.doubleTapCallback();
  }
}

@end
