// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextView.h"
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceSystemInvokeProtocol.h>
#import "LynxComponentRegistry.h"
#import "LynxLayer.h"
#import "LynxTextRenderer.h"
#import "LynxUIText.h"
#import "LynxWeakProxy.h"

#pragma mark - LynxTextLayerRender
@interface LynxTextLayerRender : NSObject <CALayerDelegate>
@property(nonatomic, weak) LynxTextRenderer *textRenderer;
@property(nonatomic, assign) UIEdgeInsets border;
@property(nonatomic, assign) UIEdgeInsets padding;
@property(nonatomic, assign) CGPoint overflowOffset;

@end

@implementation LynxTextLayerRender

- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)ctx {
  LynxTextRenderer *strongRender = self.textRenderer;

  CGRect frame = CGRectMake(_overflowOffset.x, _overflowOffset.y, layer.frame.size.width,
                            layer.frame.size.height);

  UIGraphicsPushContext(ctx);
  [strongRender drawRect:frame padding:self.padding border:self.border];
  UIGraphicsPopContext();
}

@end

#pragma mark - LynxTextView
static __weak LynxTextView *sWeakSelectingTextView = nil;
static const int kDotSize = 10;
static const int kHandleWidth = 2;
static const float kResponseTouchRadius = 20.f;

@interface LynxTextView () <UIGestureRecognizerDelegate>
/// selection drawing
@property(nonatomic, strong) LynxLayer *selectionLayer;
@property(nonatomic, strong) LynxLayer *startDot;
@property(nonatomic, strong) LynxLayer *endDot;
@property(nonatomic, strong) UIColor *selectColor;
@property(nonatomic, strong) UIColor *handleColor;
@property(nonatomic, assign) CGFloat handleSize;
/// selection info
@property(nonatomic, assign) NSInteger selectionStart;
@property(nonatomic, assign) NSInteger selectionEnd;
@property(nonatomic, assign) NSInteger lastSelectionStart;
@property(nonatomic, assign) NSInteger lastSelectionEnd;
@property(nonatomic, assign) CGPoint selectStartPoint;
@property(nonatomic, assign) CGPoint selectEndPoint;
@property(nonatomic, assign) CGPoint handleStartPoint;
@property(nonatomic, assign) CGPoint handleEndPoint;

/// selection state
@property(nonatomic, assign) BOOL trackingLongPress;
@property(nonatomic, assign) BOOL menuShowing;
@property(nonatomic, assign) BOOL isInSelection;
@property(nonatomic, assign) BOOL isAdjustStartPoint;
@property(nonatomic, assign) BOOL isAdjustEndPoint;
@property(nonatomic, strong) UILongPressGestureRecognizer *longPressGesture;
@property(nonatomic, strong) UIPanGestureRecognizer *hoverGesture;
@property(nonatomic, strong) UITapGestureRecognizer *tapGesture;
@property(nonatomic, assign) BOOL isSelectionForward;

@end

@implementation LynxTextView {
  LynxTextLayerRender *_layerRender;
}

+ (Class)layerClass {
  return [LynxLayer class];
}

- (instancetype)init {
  self = [super init];
  if (self) {
    self.contentLayer = [LynxLayer new];
    self.contentLayer.contentsScale = [[UIScreen mainScreen] scale];
    self.layer.contentsScale = [[UIScreen mainScreen] scale];
    // https://developer.apple.com/documentation/quartzcore/calayer/1410974-drawsasynchronously?language=objc
    // make drawInContext method queued draw command and execute in background thread
    self.contentLayer.drawsAsynchronously = YES;

    [self.layer addSublayer:self.contentLayer];

    _layerRender = [LynxTextLayerRender new];
    self.contentLayer.delegate = _layerRender;

    self.enableTextSelection = self.enableCustomContextMenu = self.enableCustomTextSelection = NO;
    self.selectionStart = self.selectionEnd = self.lastSelectionStart = self.lastSelectionEnd = -1;
    self.trackingLongPress = self.isInSelection = self.menuShowing = self.isAdjustStartPoint =
        self.isAdjustEndPoint = NO;
    self.selectColor = [UIColor.systemBlueColor colorWithAlphaComponent:0.5f];
    self.handleColor = UIColor.systemBlueColor;
    self.handleSize = kDotSize;

    self.userInteractionEnabled = YES;
  }
  return self;
}

- (void)initSelectionLayers {
  self.selectionLayer = [LynxLayer new];
  [self.layer insertSublayer:self.selectionLayer below:self.contentLayer];

  self.startDot = [LynxLayer new];
  self.endDot = [LynxLayer new];
  self.startDot.frame = self.endDot.frame = CGRectMake(0, 0, self.handleSize, self.handleSize);
  self.startDot.cornerRadius = self.endDot.cornerRadius = self.handleSize / 2.f;
  self.startDot.backgroundColor = self.endDot.backgroundColor = self.handleColor.CGColor;
}

- (void)setupSelectionGesture {
  self.longPressGesture =
      [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
  self.longPressGesture.delegate = self;

  self.hoverGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self
                                                              action:@selector(handleMove:)];
  self.hoverGesture.delegate = self;

  self.tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self
                                                            action:@selector(handleCancelTap:)];
  self.tapGesture.delegate = self;
}

- (void)updateSelectionColor:(UIColor *)color {
  self.selectColor = color == nil ? [UIColor.systemBlueColor colorWithAlphaComponent:0.5] : color;
}

- (void)updateHandleColor:(UIColor *)color {
  self.handleColor = color == nil ? UIColor.systemBlueColor : color;
}

- (void)updateHandleSize:(CGFloat)size {
  self.handleSize = size <= 0.f ? kDotSize : size;
}

- (void)setOverflowOffset:(CGPoint)offset {
  _layerRender.overflowOffset = offset;
}

- (NSString *)description {
  NSString *superDescription = super.description;
  NSRange semicolonRange = [superDescription rangeOfString:@";"];
  NSString *replacement =
      [NSString stringWithFormat:@"; text: %@", _ui.renderer.layoutManager.textStorage.string];
  return [superDescription stringByReplacingCharactersInRange:semicolonRange
                                                   withString:replacement];
}

- (NSString *)text {
  return _ui.renderer.attrStr.string;
}

- (void)setBorder:(UIEdgeInsets)border {
  _border = border;
  _layerRender.border = border;
}

- (void)setPadding:(UIEdgeInsets)padding {
  _padding = padding;
  _layerRender.padding = padding;
}

- (void)setTextRenderer:(LynxTextRenderer *)textRenderer {
  _textRenderer = textRenderer;
  _layerRender.textRenderer = textRenderer;

  if (_isInSelection) {
    [self clearSelectionHighlight];
  } else {
    [self resetSelectionState];
  }
}

- (void)resetSelectionState {
  self.selectionStart = self.selectionEnd = self.lastSelectionStart = self.lastSelectionEnd = -1;
  self.trackingLongPress = self.isInSelection = self.menuShowing = self.isAdjustStartPoint =
      self.isAdjustEndPoint = NO;
}

- (void)layoutSublayersOfLayer:(CALayer *)layer {
  if (layer != self.layer) {
    return;
  }
  [self updateSelectionHighlights];
}

- (void)initSelectionGesture {
  // init gestureRecognizer if need
  if (self.enableTextSelection && !self.enableCustomTextSelection) {
    if (!self.longPressGesture) {
      [self setupSelectionGesture];
    }
    [self installGestures];
  } else if (self.longPressGesture) {
    // It is necessary to uninstall gestures when to reuse node scenarios.
    [self unInstallGestures];
  }
}

- (NSArray *)getTextBoundingBoxes:(NSInteger)start withEnd:(NSInteger)end {
  NSMutableArray *boxes = [NSMutableArray new];
  if (!self.textRenderer.attrStr || start < 0 ||
      end > (NSInteger)self.textRenderer.attrStr.length || start > end) {
    return boxes;
  }

  NSUInteger glyphStartIndex =
      [self.textRenderer.layoutManager glyphIndexForCharacterAtIndex:start];
  if (start == end) {
    CGRect lineRect =
        [self.textRenderer.layoutManager lineFragmentRectForGlyphAtIndex:glyphStartIndex
                                                          effectiveRange:nil];
    [boxes addObject:@(CGRectMake(0.f, 0.f, 0.f, lineRect.size.height))];
  } else {
    NSUInteger glyphEndIndex =
        end == (NSInteger)self.textRenderer.attrStr.length
            ? [self getTextRenderGlyphCount]
            : [self.textRenderer.layoutManager glyphIndexForCharacterAtIndex:end];
    NSUInteger glyphLength = glyphEndIndex - glyphStartIndex;
    [self.textRenderer.layoutManager
        enumerateEnclosingRectsForGlyphRange:NSMakeRange(glyphStartIndex, glyphLength)
                    withinSelectedGlyphRange:NSMakeRange(glyphStartIndex, glyphLength)
                             inTextContainer:[self.textRenderer.layoutManager
                                                     .textContainers firstObject]
                                  usingBlock:^(CGRect rect, BOOL *_Nonnull stop) {
                                    [boxes addObject:@(rect)];
                                  }];
  }

  return boxes;
}

- (NSString *)getSelectedText {
  if (self.textRenderer.attrStr && self.selectionStart >= 0 &&
      self.selectionStart <= (NSInteger)[self getTextRenderGlyphCount] && self.selectionEnd >= 0 &&
      self.selectionEnd <= (NSInteger)[self getTextRenderGlyphCount]) {
    NSInteger start = MIN(self.selectionStart, self.selectionEnd);
    NSInteger end = MAX(self.selectionStart, self.selectionEnd);
    NSUInteger characterStart =
        [self.textRenderer.layoutManager characterIndexForGlyphAtIndex:start];
    NSUInteger characterEnd =
        end == (NSInteger)[self getTextRenderGlyphCount]
            ? self.textRenderer.attrStr.length
            : [self.textRenderer.layoutManager characterIndexForGlyphAtIndex:end];

    return [[self.textRenderer.attrStr string]
        substringWithRange:NSMakeRange(characterStart, characterEnd - characterStart)];
  }
  return @"";
}

- (NSArray *)setTextSelection:(CGFloat)startX
                       startY:(CGFloat)startY
                         endX:(CGFloat)endX
                         endY:(CGFloat)endY {
  NSMutableArray *ret = [NSMutableArray array];
  if (startX < 0 || startY < 0 || endX < 0 || endY < 0) {
    [self clearSelectionHighlight];
    return ret;
  }
  CGPoint startPoint = [self convertPointToLayout:CGPointMake(startX, startY)];
  CGPoint endPoint = [self convertPointToLayout:CGPointMake(endX, endY)];
  NSInteger startIndex = [self getGlyphOffsetByPoint:startPoint];
  NSInteger endIndex = [self getGlyphOffsetByPoint:endPoint];

  if (startIndex == endIndex) {
    if (startIndex == [self getTextRenderGlyphCount] ||
        (startIndex > 0 &&
         startPoint.x < [self.textRenderer.layoutManager locationForGlyphAtIndex:startIndex].x)) {
      startIndex--;
    } else {
      endIndex++;
    }
  }

  self.isInSelection = YES;
  [self updateSelectionRange:startIndex widthSelectEnd:endIndex];
  [self updateSelectStartEnd];

  NSUInteger characterStart =
      [self.textRenderer.layoutManager characterIndexForGlyphAtIndex:self.selectionStart];
  NSUInteger characterEnd =
      self.selectionEnd == (NSInteger)[self getTextRenderGlyphCount]
          ? self.textRenderer.attrStr.length
          : [self.textRenderer.layoutManager characterIndexForGlyphAtIndex:self.selectionEnd];

  return [self getTextBoundingBoxes:characterStart withEnd:characterEnd];
}

- (NSArray *)getHandlesInfo {
  NSMutableArray *ret = [NSMutableArray array];
  if (!self.isInSelection) {
    return ret;
  }

  NSMutableArray *startHandle = [NSMutableArray array];
  [startHandle addObject:@(self.handleStartPoint.x)];
  [startHandle addObject:@(self.handleStartPoint.y)];
  [startHandle addObject:@(kResponseTouchRadius)];
  [ret addObject:startHandle];
  NSMutableArray *endHandle = [NSMutableArray array];
  [endHandle addObject:@(self.handleEndPoint.x)];
  [endHandle addObject:@(self.handleEndPoint.y)];
  [endHandle addObject:@(kResponseTouchRadius)];
  [ret addObject:endHandle];

  return ret;
}

- (void)onSelectionChange {
  if (self.selectionChangeEventCallback) {
    NSDictionary *dic = @{
      @"start" : @(MIN(self.selectionStart, self.selectionEnd)),
      @"end" : @(MAX(self.selectionStart, self.selectionEnd)),
      @"direction" : self.isSelectionForward ? @"forward" : @"backward",
    };
    self.selectionChangeEventCallback(dic);
  }
}

#pragma mark - SelectionControl
- (CGPoint)convertPointToLayout:(CGPoint)point {
  point.x -= (self.padding.left + self.border.left);
  point.y -= (self.padding.top + self.border.left);

  return point;
}

- (NSInteger)getGlyphOffsetByPoint:(CGPoint)point {
  NSUInteger offset = [self.textRenderer.layoutManager
      glyphIndexForPoint:point
         inTextContainer:[self.textRenderer.layoutManager.textContainers firstObject]];
  CGPoint glyphPosition = [self.textRenderer.layoutManager locationForGlyphAtIndex:offset];
  CGRect lineRect = [self.textRenderer.layoutManager lineFragmentUsedRectForGlyphAtIndex:offset
                                                                          effectiveRange:nil];
  if (point.x > glyphPosition.x + (lineRect.origin.x + lineRect.size.width - glyphPosition.x) / 2) {
    offset++;
  }
  return offset;
}

- (NSInteger)getTextRenderGlyphCount {
  return [self.textRenderer.layoutManager numberOfGlyphs];
}

- (void)updateSelectionHighlights {
  if (!self.enableTextSelection && !self.enableCustomTextSelection) {
    return;
  }
  self.selectionLayer.sublayers = nil;

  if (!self.isInSelection || self.selectionStart == -1 || self.selectionEnd == -1 ||
      self.selectionStart == self.selectionEnd) {
    return;
  }

  [self.selectionLayer
      setFrame:CGRectMake(self.padding.left + self.border.left, self.padding.top + self.border.top,
                          self.selectionLayer.frame.size.width,
                          self.selectionLayer.frame.size.height)];

  NSInteger start = MIN(self.selectionStart, self.selectionEnd);
  NSInteger length = ABS(self.selectionEnd - self.selectionStart);
  [self.textRenderer.layoutManager
      enumerateEnclosingRectsForGlyphRange:NSMakeRange(start, length)
                  withinSelectedGlyphRange:NSMakeRange(start, length)
                           inTextContainer:[self.textRenderer.layoutManager
                                                   .textContainers firstObject]
                                usingBlock:^(CGRect rect, BOOL *_Nonnull stop) {
                                  CALayer *highlight = [LynxLayer new];
                                  if (rect.size.width > 0) {
                                    highlight.backgroundColor = self.selectColor.CGColor;
                                  } else {
                                    rect.size.width = 1;
                                    highlight.backgroundColor = self.handleColor.CGColor;
                                  }

                                  highlight.frame = rect;

                                  [self.selectionLayer addSublayer:highlight];
                                }];

  // start cursor
  CGRect startFrame = [self.textRenderer.layoutManager
      boundingRectForGlyphRange:NSMakeRange(start, 0)
                inTextContainer:self.textRenderer.layoutManager.textContainers.firstObject];
  self.startDot.frame =
      CGRectMake(self.handleStartPoint.x - self.handleSize / 2,
                 self.handleStartPoint.y - self.handleSize / 2, self.handleSize, self.handleSize);
  CALayer *startCursor = [LynxLayer new];
  startCursor.frame = CGRectMake(self.handleStartPoint.x - kHandleWidth / 2,
                                 self.handleStartPoint.y, kHandleWidth, startFrame.size.height);
  startCursor.backgroundColor = self.handleColor.CGColor;
  self.startDot.backgroundColor = self.handleColor.CGColor;
  [self.selectionLayer addSublayer:self.startDot];
  [self.selectionLayer addSublayer:startCursor];

  // end cursor
  CGRect endFrame = [self.textRenderer.layoutManager
      boundingRectForGlyphRange:NSMakeRange(start + length - 1, 1)
                inTextContainer:self.textRenderer.layoutManager.textContainers.firstObject];
  self.endDot.frame =
      CGRectMake(self.handleEndPoint.x - self.handleSize / 2,
                 self.handleEndPoint.y - self.handleSize / 2, self.handleSize, self.handleSize);

  CALayer *endCursor = [LynxLayer new];
  endCursor.frame =
      CGRectMake(self.handleEndPoint.x - kHandleWidth / 2,
                 self.handleEndPoint.y - endFrame.size.height, kHandleWidth, endFrame.size.height);
  endCursor.backgroundColor = self.handleColor.CGColor;
  self.endDot.backgroundColor = self.handleColor.CGColor;
  [self.selectionLayer addSublayer:self.endDot];
  [self.selectionLayer addSublayer:endCursor];
}

- (void)clearSelectionHighlight {
  self.selectionStart = -1;
  self.selectionEnd = -1;
  self.lastSelectionStart = -1;
  self.lastSelectionEnd = -1;

  if (self.isInSelection) {
    [self.layer setNeedsLayout];
    [self onSelectionChange];
  }
  self.isInSelection = NO;
  self.isAdjustEndPoint = NO;
  self.isAdjustStartPoint = NO;
  self.trackingLongPress = NO;

  [self hideMenu];

  [self.layer setNeedsLayout];
}

- (void)clearOtherSelection {
  if (sWeakSelectingTextView && sWeakSelectingTextView != self) {
    [sWeakSelectingTextView clearSelectionHighlight];
    [sWeakSelectingTextView.layer setNeedsLayout];
  }

  sWeakSelectingTextView = self;
}

- (void)adjustStartPosition:(CGPoint)point {
  self.isAdjustStartPoint = YES;
  NSInteger selectStart = [self getGlyphOffsetByPoint:point];
  if (selectStart == self.selectionEnd) {
    if (selectStart == (NSInteger)[self getTextRenderGlyphCount] ||
        (point.x < self.selectEndPoint.x && selectStart > 0)) {
      selectStart--;
    } else {
      selectStart++;
    }
  }
  [self updateSelectionRange:selectStart widthSelectEnd:self.selectionEnd];
}

- (void)adjustEndPosition:(CGPoint)point {
  self.isAdjustEndPoint = YES;
  NSInteger selectEnd = [self getGlyphOffsetByPoint:point];
  if (self.selectionStart == selectEnd) {
    if (selectEnd == (NSInteger)[self getTextRenderGlyphCount] ||
        (point.x < self.selectStartPoint.x && selectEnd > 0)) {
      selectEnd--;
    } else {
      selectEnd++;
    }
  }
  [self updateSelectionRange:self.selectionStart widthSelectEnd:selectEnd];
}

- (void)updateSelectionRange:(NSInteger)selectStart widthSelectEnd:(NSInteger)selectEnd {
  if (self.selectionStart == selectStart && self.selectionEnd == selectEnd) {
    return;
  }
  if (!self.selectionLayer) {
    [self initSelectionLayers];
  }

  self.isSelectionForward =
      self.lastSelectionStart == -1
          ? selectEnd > selectStart
          : (self.lastSelectionStart < selectStart || self.lastSelectionEnd < selectEnd);

  self.lastSelectionStart = self.selectionStart;
  self.lastSelectionEnd = self.selectionEnd;
  self.selectionStart = selectStart;
  self.selectionEnd = selectEnd;
  if (self.selectionStart >= 0 &&
      self.selectionStart <= (NSInteger)[self getTextRenderGlyphCount] && self.selectionEnd >= 0 &&
      self.selectionEnd <= (NSInteger)[self getTextRenderGlyphCount]) {
    // calculate start end position
    [self calcSelectionPosition];
    [self clearOtherSelection];
    [self.layer setNeedsLayout];
  }
}

- (void)calcSelectionPosition {
  NSInteger start = MIN(self.selectionStart, self.selectionEnd);
  NSInteger length = ABS(self.selectionEnd - self.selectionStart);

  CGRect startFrame = [self.textRenderer.layoutManager
      boundingRectForGlyphRange:NSMakeRange(start, 0)
                inTextContainer:self.textRenderer.layoutManager.textContainers.firstObject];
  __block CGRect endFrame = startFrame;
  [self.textRenderer.layoutManager
      enumerateEnclosingRectsForGlyphRange:NSMakeRange(start + length - 1, 1)
                  withinSelectedGlyphRange:NSMakeRange(start + length - 1, 1)
                           inTextContainer:[self.textRenderer.layoutManager
                                                   .textContainers firstObject]
                                usingBlock:^(CGRect rect, BOOL *_Nonnull stop) {
                                  endFrame = rect;
                                }];

  self.handleStartPoint = CGPointMake(MAX(startFrame.origin.x, 0), MAX(startFrame.origin.y, 0));
  self.handleEndPoint =
      CGPointMake(MIN(endFrame.origin.x + endFrame.size.width, self.frame.size.width),
                  MIN(endFrame.origin.y + endFrame.size.height, self.frame.size.height));

  self.selectStartPoint =
      CGPointMake(self.handleStartPoint.x, self.handleStartPoint.y + startFrame.size.height / 2);
  self.selectEndPoint =
      CGPointMake(self.handleEndPoint.x, self.handleEndPoint.y - endFrame.size.height / 2);
}

- (void)updateSelectStartEnd {
  NSInteger min = MIN(self.selectionStart, self.selectionEnd);
  self.selectionEnd = MAX(self.selectionStart, self.selectionEnd);
  self.selectionStart = min;

  [self onSelectionChange];
}

#pragma mark - Menu control

- (void)hideMenu {
  if (self.enableCustomContextMenu || !self.menuShowing) {
    return;
  }

  UIMenuController *menu = [UIMenuController sharedMenuController];
  [menu setMenuVisible:NO animated:YES];
  self.menuShowing = NO;
}

- (void)showMenu {
  if (self.enableCustomContextMenu) {
    return;
  }
  if (self.selectionStart == -1 || self.selectionEnd == -1 ||
      self.selectionStart == self.selectionEnd) {
    return;
  }

  if (!self.isFirstResponder) {
    [self becomeFirstResponder];
  }

  if (!self.isFirstResponder) {
    return;
  }

  UIMenuController *menu = [UIMenuController sharedMenuController];
  NSInteger start = MIN(self.selectionStart, self.selectionEnd);
  NSInteger length = ABS(self.selectionEnd - self.selectionStart);
  CGRect rect = [self.textRenderer.layoutManager
      boundingRectForGlyphRange:NSMakeRange(start, length)
                inTextContainer:self.textRenderer.layoutManager.textContainers.firstObject];
  rect.origin.x += self.border.left + self.padding.left;
  rect.origin.y += self.border.top + self.padding.top;

  [menu setTargetRect:rect inView:self];
  [menu update];
  [menu setMenuVisible:YES animated:YES];

  self.menuShowing = YES;
}

#pragma mark - UIResponder
- (BOOL)canBecomeFirstResponder {
  if (!self.enableTextSelection) {
    return NO;
  }

  if (!self.isInSelection) {
    return NO;
  }

  return YES;
}

#pragma mark - Menu Action

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender {
  return action == @selector(selectAll:) || action == @selector(copy:);
}

- (void)copy:(id)sender {
  NSInteger start = MIN(self.selectionStart, self.selectionEnd);
  NSInteger length = ABS(self.selectionEnd - self.selectionStart);

  if (start == -1 || length == 0) {
    return;
  }
  NSRange characterRange =
      [self.textRenderer.layoutManager characterRangeForGlyphRange:NSMakeRange(start, length)
                                                  actualGlyphRange:nil];
  NSString *string = [self.textRenderer.attrStr attributedSubstringFromRange:characterRange].string;
  // write to pasteboard
  id<LynxServiceSystemInvokeProtocol> invoker = LynxService(LynxServiceSystemInvokeProtocol);
  if (invoker) {
    [invoker setString:string];
  } else {
    [UIPasteboard generalPasteboard].string = string;
  }

  // clear selection
  [self clearSelectionHighlight];
}

- (void)selectAll:(id)sender {
  [self updateSelectionRange:0 widthSelectEnd:[self getTextRenderGlyphCount]];
  [self updateSelectStartEnd];
  [self.layer setNeedsLayout];

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.01 * NSEC_PER_SEC)),
                 dispatch_get_main_queue(), ^{
                   [self showMenu];
                 });
}

#pragma mark - UIGestureRecognizer
- (void)handleLongPress:(UIGestureRecognizer *)sender {
  CGPoint point = [sender locationInView:self];
  point = [self convertPointToLayout:point];
  if (sender.state == UIGestureRecognizerStateBegan) {
    [self clearSelectionHighlight];
    self.isInSelection = YES;
    self.trackingLongPress = YES;

    self.selectStartPoint = self.selectEndPoint = point;
    self.selectionStart = self.selectionEnd = [self getGlyphOffsetByPoint:point];
    self.isAdjustEndPoint = YES;
    [self adjustEndPosition:point];

    [self.layer setNeedsLayout];

  } else if (sender.state == UIGestureRecognizerStateEnded) {
    if (self.trackingLongPress) {
      [self performEndSelection:point];
      [self.layer setNeedsLayout];
      self.trackingLongPress = NO;
    }
  }
}

- (void)handleMove:(UIPanGestureRecognizer *)sender {
  if (!self.isInSelection) {
    return;
  }

  self.trackingLongPress = NO;
  CGPoint point = [sender locationInView:self];
  point = [self convertPointToLayout:point];

  if (sender.state == UIGestureRecognizerStateBegan) {
    [self hideMenu];
    if ([self distanceBetweenPoints:point
                        withAnother:self.handleStartPoint] < kResponseTouchRadius) {
      [self adjustStartPosition:point];
    } else if ([self distanceBetweenPoints:point
                               withAnother:self.handleEndPoint] < kResponseTouchRadius) {
      [self adjustEndPosition:point];
    }

  } else if (sender.state == UIGestureRecognizerStateChanged) {
    if (self.isAdjustStartPoint) {
      [self adjustStartPosition:point];
    } else if (self.isAdjustEndPoint) {
      [self adjustEndPosition:point];
    }
  } else if (sender.state == UIGestureRecognizerStateEnded) {
    [self performEndSelection:point];
  }

  [self.layer setNeedsLayout];
}

- (CGFloat)distanceBetweenPoints:(CGPoint)point1 withAnother:(CGPoint)point2 {
  return sqrt(powf(point1.x - point2.x, 2) + powf(point1.y - point2.y, 2));
}

- (void)performEndSelection:(CGPoint)point {
  if (!self.isInSelection) {
    [self clearSelectionHighlight];
    return;
  }

  if (self.isAdjustStartPoint) {
    [self adjustStartPosition:point];
    [self updateSelectStartEnd];
  } else if (self.isAdjustEndPoint) {
    [self adjustEndPosition:point];
    [self updateSelectStartEnd];
  }
  [self showMenu];
  self.isAdjustEndPoint = self.isAdjustStartPoint = NO;
}

- (void)handleCancelTap:(UITapGestureRecognizer *)sender {
  [self clearSelectionHighlight];
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer {
  if (gestureRecognizer == self.hoverGesture &&
      [self shouldRecognizeIndividuallyWithGestureRecognizer:gestureRecognizer]) {
    return NO;
  }
  return YES;
}

- (BOOL)shouldRecognizeIndividuallyWithGestureRecognizer:(UIGestureRecognizer *)gestureRecognizer {
  if (self.isInSelection) {
    CGPoint point = [gestureRecognizer locationInView:self];
    point = [self convertPointToLayout:point];
    return [self distanceBetweenPoints:point
                           withAnother:self.handleStartPoint] < kResponseTouchRadius ||
           [self distanceBetweenPoints:point
                           withAnother:self.handleEndPoint] < kResponseTouchRadius;
  }

  return NO;
}

// return YES, otherGestureRecognizer recognize failure, gestureRecognizer will start
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRequireFailureOfGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
  return NO;
}

// return YES, gestureRecognizer recognize failure, otherGestureRecognizer will start
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
  if (gestureRecognizer == self.longPressGesture && otherGestureRecognizer != self.hoverGesture) {
    return YES;
  }
  return NO;
}

- (void)installGestures {
  [self addGestureRecognizer:self.longPressGesture];
  [self addGestureRecognizer:self.hoverGesture];
  [self addGestureRecognizer:self.tapGesture];
}

- (void)unInstallGestures {
  [self removeGestureRecognizer:self.longPressGesture];
  [self removeGestureRecognizer:self.hoverGesture];
  [self removeGestureRecognizer:self.tapGesture];
}

@end
