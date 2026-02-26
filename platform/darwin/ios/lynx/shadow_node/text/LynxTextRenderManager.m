// Copyright 2025 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEvent.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxTextLayoutSpec.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxTextRenderer.h>
#import <Lynx/LynxTextShadowNode.h>
#import <Lynx/LynxTextUtils.h>

@implementation LynxAttributedTextBundle
@end

@implementation LynxTextRenderManager {
  NSMutableDictionary<NSNumber *, LynxTextRenderer *> *_textRenderDic;
  NSMutableDictionary<NSNumber *, LynxAttributedTextBundle *> *_attributedTextBundleDic;
}

- (instancetype)init {
  if ((self = [super init])) {
    _textRenderDic = [NSMutableDictionary new];
    _attributedTextBundleDic = [NSMutableDictionary new];
  }
  return self;
}

- (MeasureResult)measureTextWithSign:(int)sign
                               width:(float)width
                           widthMode:(LynxMeasureMode)widthMode
                              height:(float)height
                          heightMode:(LynxMeasureMode)heightMode
                     childrenSizeDic:(NSDictionary *)childrenSizeDic {
  LynxAttributedTextBundle *textBundle = [_attributedTextBundleDic objectForKey:@(sign)];

  LynxLayoutSpec *spec = [[LynxLayoutSpec alloc] initWithWidth:width
                                                        height:height
                                                     widthMode:widthMode
                                                    heightMode:heightMode
                                                  textOverflow:textBundle.textOverflow
                                                      overflow:textBundle.overflow
                                                    whiteSpace:textBundle.whiteSpace
                                                    maxLineNum:textBundle.maxLineNum
                                                 maxTextLength:-1
                                                     textStyle:textBundle.textStyle
                                        enableTailColorConvert:NO];
  spec.enableTextRefactor = YES;
  spec.enableTextNonContiguousLayout = YES;
  spec.enableNewClipMode = YES;

  LineSpacingAdaptation *layoutDelegate = [LineSpacingAdaptation new];
  layoutDelegate.enableLayoutRefactor = YES;
  layoutDelegate.attributedString = textBundle.attributedString;
  if (!isnan(textBundle.textStyle.lineHeight)) {
    layoutDelegate.lineHeight = textBundle.textStyle.lineHeight;
  }
  spec.layoutManagerDelegate = layoutDelegate;

  if (childrenSizeDic && childrenSizeDic.count != 0) {
    [self updateAttachmentSize:childrenSizeDic
              attributedString:textBundle.attributedString
                    lineHeight:textBundle.textStyle.lineHeight];
  }

  LynxTextRenderer *renderer =
      [[LynxTextRenderer alloc] initWithAttributedString:textBundle.attributedString
                                              layoutSpec:spec];
  [renderer ensureTextRenderLayout];
  [_textRenderDic setObject:renderer forKey:@(sign)];

  CGSize size = renderer.size;
  if (!isnan(textBundle.textStyle.letterSpacing) && textBundle.textStyle.letterSpacing < 0) {
    size.width -= textBundle.textStyle.letterSpacing;
  }

  MeasureResult result = {.size = size, .baseline = renderer.baseline};
  return result;
}

- (void)updateAttachmentSize:(NSDictionary *)childrenSizeDic
            attributedString:(NSAttributedString *)attributedString
                  lineHeight:(CGFloat)lineHeight {
  __block CGFloat maxAscender = 0, maxDescender = 0, maxXHeight = 0;
  [attributedString enumerateAttribute:NSFontAttributeName
                               inRange:NSMakeRange(0, attributedString.length)
                               options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                            usingBlock:^(UIFont *font, NSRange range, BOOL *_Nonnull stop) {
                              if (font) {
                                maxAscender = MAX(maxAscender, font.ascender);
                                maxDescender = MIN(maxDescender, font.descender);
                                maxXHeight = MAX(maxXHeight, font.xHeight);
                              }
                            }];
  [attributedString
      enumerateAttribute:NSAttachmentAttributeName
                 inRange:NSMakeRange(0, attributedString.length)
                 options:0
              usingBlock:^(id _Nullable value, NSRange range, BOOL *_Nonnull stop) {
                if ([value isKindOfClass:[LynxTextAttachment class]]) {
                  LynxTextAttachment *attachment = value;
                  NSArray *size = [childrenSizeDic objectForKey:@(attachment.sign)];
                  if (size) {
                    CGFloat width = [size[0] floatValue], height = [size[1] floatValue],
                            baseline = [size[2] floatValue];
                    CGFloat baselineOffset = 0.f;
                    if (attachment.verticalAlign == LynxVerticalAlignBaseline) {
                      baselineOffset = baseline - height;
                    } else if (attachment.verticalAlign != LynxVerticalAlignDefault) {
                      baselineOffset = [LynxTextUtils
                          calcBaselineShiftOffset:attachment.verticalAlign
                               verticalAlignValue:attachment.verticalAlignLength
                                     withAscender:height
                                    withDescender:0.f
                                   withLineHeight:isnan(lineHeight) ? 0.f : lineHeight
                                  withMaxAscender:maxAscender
                                 withMaxDescender:maxDescender
                                   withMaxXHeight:maxXHeight];
                    }
                    attachment.bounds = CGRectMake(0, baselineOffset, width, [size[1] floatValue]);
                  }
                }
              }];
}

- (id)takeTextRender:(NSInteger)sign {
  return [_textRenderDic objectForKey:@(sign)];
}

- (void)releaseTextRender:(NSInteger)sign {
  [_textRenderDic removeObjectForKey:@(sign)];
  [_attributedTextBundleDic removeObjectForKey:@(sign)];
}

- (void)putAttributedTextBundle:(NSInteger)sign textBundle:(LynxAttributedTextBundle *)textBundle {
  if (textBundle) {
    [_attributedTextBundleDic setObject:textBundle forKey:@(sign)];
  }
}

- (NSDictionary *)getInlineElementOffsetDic:(NSInteger)sign {
  NSMutableDictionary *offsetDic = [NSMutableDictionary new];
  LynxTextRenderer *renderer = [_textRenderDic objectForKey:@(sign)];
  if (!renderer) {
    return offsetDic;
  }

  NSTextStorage *textStorage = renderer.textStorage;
  NSLayoutManager *layoutManager = textStorage.layoutManagers.firstObject;
  NSTextContainer *textContainer = layoutManager.textContainers.firstObject;

  NSRange glyphRange = [layoutManager glyphRangeForTextContainer:textContainer];
  NSRange characterRange = [layoutManager characterRangeForGlyphRange:glyphRange
                                                     actualGlyphRange:nil];
  NSMutableSet<NSNumber *> *visibleElementSigns = [NSMutableSet new];

  [self enumerateAttachmentsInTextStorage:textStorage
                                    range:characterRange
                            layoutManager:layoutManager
                            textContainer:textContainer
                                 renderer:renderer
                          visibleSignsSet:visibleElementSigns
                                offsetDic:offsetDic];

  [self handleHiddenInlineElementsWithSign:sign visibleSigns:visibleElementSigns renderer:renderer];

  return offsetDic;
}

- (void)enumerateAttachmentsInTextStorage:(NSTextStorage *)textStorage
                                    range:(NSRange)range
                            layoutManager:(NSLayoutManager *)layoutManager
                            textContainer:(NSTextContainer *)textContainer
                                 renderer:(LynxTextRenderer *)renderer
                          visibleSignsSet:(NSMutableSet<NSNumber *> *)visibleSigns
                                offsetDic:(NSMutableDictionary *)offsetDic {
  [textStorage
      enumerateAttribute:NSAttachmentAttributeName
                 inRange:range
                 options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
              usingBlock:^(id _Nullable value, NSRange range, BOOL *_Nonnull stop) {
                if (![value isKindOfClass:[LynxTextAttachment class]]) {
                  return;
                }

                LynxTextAttachment *attachment = (LynxTextAttachment *)value;
                NSRange attachmentGlyphRange = [layoutManager glyphRangeForCharacterRange:range
                                                                     actualCharacterRange:nil];

                NSRange truncatedGlyphRange = [layoutManager
                    truncatedGlyphRangeInLineFragmentForGlyphAtIndex:attachmentGlyphRange.location];
                if (truncatedGlyphRange.location != NSNotFound &&
                    truncatedGlyphRange.location <= attachmentGlyphRange.location) {
                  return;
                }

                CGPoint offset = [self calculateAttachmentOffset:attachment
                                                      glyphRange:attachmentGlyphRange
                                                   layoutManager:layoutManager
                                                   textContainer:textContainer
                                                        renderer:renderer];
                [offsetDic setObject:@(offset) forKey:@(attachment.sign)];
                [visibleSigns addObject:@(attachment.sign)];
              }];
}

- (CGPoint)calculateAttachmentOffset:(LynxTextAttachment *)attachment
                          glyphRange:(NSRange)glyphRange
                       layoutManager:(NSLayoutManager *)layoutManager
                       textContainer:(NSTextContainer *)textContainer
                            renderer:(LynxTextRenderer *)renderer {
  CGRect glyphRect = [layoutManager boundingRectForGlyphRange:glyphRange
                                              inTextContainer:textContainer];
  CGFloat leftOffset =
      glyphRect.origin.x + attachment.bounds.origin.x + renderer.textContentOffsetX;
  CGFloat baselineY = [layoutManager locationForGlyphAtIndex:glyphRange.location].y;
  CGFloat topOffset =
      glyphRect.origin.y + [LynxTextUtils alignInlineNodeInVertical:attachment.verticalAlign
                                                     withLineHeight:glyphRect.size.height
                                               withAttachmentHeight:attachment.bounds.size.height
                                            withAttachmentYPosition:baselineY];
  return CGPointMake(leftOffset, topOffset);
}

- (void)handleHiddenInlineElementsWithSign:(NSInteger)sign
                              visibleSigns:(NSSet<NSNumber *> *)visibleSigns
                                  renderer:(LynxTextRenderer *)renderer {
  LynxAttributedTextBundle *textBundle = [_attributedTextBundleDic objectForKey:@(sign)];
  if (!textBundle || !textBundle.inlineElementSigns.count) return;

  NSMutableSet<NSNumber *> *hiddenSigns = [textBundle.inlineElementSigns mutableCopy];
  [hiddenSigns minusSet:visibleSigns];

  NSMutableArray *hiddenAttachments = [NSMutableArray new];
  for (NSNumber *signNum in hiddenSigns) {
    LynxTextAttachmentInfo *info = [[LynxTextAttachmentInfo alloc] initWithSign:signNum.integerValue
                                                                       andFrame:CGRectZero];
    info.nativeAttachment = YES;
    [hiddenAttachments addObject:info];
  }
  renderer.attachments = hiddenAttachments;
}

- (void)dispatchLayoutEventWithRenderer:(LynxTextRenderer *)renderer
                                   sign:(NSInteger)sign
                           eventEmitter:(LynxEventEmitter *)eventEmitter {
  if (!renderer || !eventEmitter) {
    return;
  }

  LynxAttributedTextBundle *textBundle = [_attributedTextBundleDic objectForKey:@(sign)];
  if (!textBundle || !textBundle.attributedString) {
    return;
  }

  [self dispatchLayoutEventWithRenderer:renderer
                             textBundle:textBundle
                                   sign:sign
                           eventEmitter:eventEmitter];
}

- (void)dispatchLayoutEventWithRenderer:(LynxTextRenderer *)renderer
                             textBundle:(LynxAttributedTextBundle *)textBundle
                                   sign:(NSInteger)sign
                           eventEmitter:(LynxEventEmitter *)eventEmitter {
  if (!renderer || !textBundle || !textBundle.attributedString || !eventEmitter) {
    return;
  }

  NSDictionary *layoutInfo =
      [LynxTextUtils computeLayoutEventInfoWithRenderer:renderer
                                       attributedString:textBundle.attributedString
                                             maxLineNum:textBundle.maxLineNum];
  if (!layoutInfo) {
    return;
  }

  LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:@"layout"
                                                      targetSign:sign
                                                          detail:layoutInfo];
  dispatch_async(dispatch_get_main_queue(), ^{
    [eventEmitter sendCustomEvent:event];
  });
}

@end
