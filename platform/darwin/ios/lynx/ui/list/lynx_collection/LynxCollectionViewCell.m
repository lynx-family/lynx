// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCollectionViewCell.h"
#import "LynxUIComponent.h"
#import "LynxUIImage.h"

@interface LynxCollectionViewCell ()
@end

@implementation LynxCollectionViewCell

void LynxCollectionResetAnimationRecursively(LynxUI *ui) {
  [ui.animationManager resetAnimation];
  [ui.children enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
    LynxCollectionResetAnimationRecursively(obj);
  }];
}

- (void)prepareForReuse {
  [super prepareForReuse];
  _loading = YES;
  LynxCollectionResetAnimationRecursively(_ui);
}

void LynxCollectionRestartAnimationRecursively(LynxUI *ui) {
  if ([ui isKindOfClass:[LynxUIImage class]]) {
    LynxUIImage *uiImage = (LynxUIImage *)ui;

    if (uiImage.isAnimated) {
      // to avoid GIF from being stopped after reuse, we manually call propsDidUpdate here.
      [uiImage startAnimating];
    }
  }
  [ui.animationManager restartAnimation];
  [ui.children enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
    LynxCollectionRestartAnimationRecursively(obj);
  }];
}

/**
 * All animations is removed when preparedForReuse.
 * We should restart the animation after data is set.
 */
- (void)restartAnimation {
  LynxCollectionRestartAnimationRecursively(_ui);
}

- (void)dealloc {
  // TODO (figure out why) this remove code not entered before, after refine, when actually remove
  // node will cause crash
  //  LynxUICollection *list = self.ui.parent;
  //  lynx::tasm::ListVirtualNode *listVirtualNode =
  //      reinterpret_cast<lynx::tasm::ListVirtualNode *>(list.listNode);
  //  if (listVirtualNode != nil) {
  //    listVirtualNode->RemoveComponent((uint32_t)self.ui.sign);
  //  }
}

- (void)adjustComponentFrame {
  CGRect frame = CGRectMake(0, 0, self.ui.view.frame.size.width, self.ui.view.frame.size.height);
  self.ui.view.frame = frame;
  // Adjust all related layers (background, border and mask).
  NSValue *value = [NSValue valueWithCGRect:frame];
  [self.ui setLayerValue:value forKeyPath:@"frame" forAllLayers:YES];
}

- (LynxUI *)removeLynxUI {
  LynxUI *ui = _ui;
  _ui = nil;
  [ui.view removeFromSuperview];
  return ui;
}

- (void)addLynxUI:(LynxUI *)ui {
  if (ui.view.superview) {
    LynxCollectionViewCell *originalCell = (LynxCollectionViewCell *)ui.view.superview.superview;
    originalCell.ui = nil;
    [ui.view removeFromSuperview];
  }
  self.ui = (LynxUIComponent *)ui;
  [self.contentView addSubview:ui.view];
  // Because using the contentView to wrap the component's view, so here we need to port the
  // borderLayer and backgroundLayer of the component's view to the contentView.
  LynxBackgroundManager *mgr = ui.backgroundManager;
  CALayer *contentLayer = self.contentView.layer;
  CALayer *componentLayer = ui.view.layer;
  if (mgr) {
    // if the borderLayer exists
    if (mgr.borderLayer) {
      [mgr.borderLayer removeFromSuperlayer];
      [contentLayer insertSublayer:mgr.borderLayer above:componentLayer];
    }
    // if the backgroundLayer exists
    if (mgr.backgroundLayer) {
      [mgr.backgroundLayer removeFromSuperlayer];
      [contentLayer insertSublayer:mgr.backgroundLayer below:componentLayer];
    }
  }
  [self adjustComponentFrame];
#if LYNX_LIST_DEBUG_LABEL
  if (self.label == nil) {
    self.label = [[UILabel alloc] initWithFrame:CGRectMake(4, 4, 100, 30)];
    self.label.backgroundColor = UIColor.yellowColor;
    self.label.textColor = UIColor.blackColor;
    [self.contentView addSubview:self.label];
    self.label.text = @"NO_INDEX";
  }
#endif
}

- (UICollectionViewLayoutAttributes *)preferredLayoutAttributesFittingAttributes:
    (UICollectionViewLayoutAttributes *)layoutAttributes {
  [super preferredLayoutAttributesFittingAttributes:layoutAttributes];
  CGRect rect = layoutAttributes.frame;

  BOOL frameDidSetUnderPartOnLayout = self.isPartOnLayout &&
                                      [self.ui isKindOfClass:LynxUIComponent.class] &&
                                      ((LynxUIComponent *)self.ui).frameDidSet;

  if ((!self.isPartOnLayout && self.ui) || frameDidSetUnderPartOnLayout) {
    rect.size = self.ui.view.frame.size;
  }
  layoutAttributes.frame = rect;
  return layoutAttributes;
}

- (void)applyLayoutAttributes:(UICollectionViewLayoutAttributes *)layoutAttributes {
  [super applyLayoutAttributes:layoutAttributes];
  [self adjustComponentFrame];
}

@end
