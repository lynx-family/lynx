// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListViewCellLight.h>
#import "LynxUI.h"
#import "LynxUIComponent.h"

@implementation LynxListViewCellLight
- (instancetype)init {
  if (self = [super init]) {
    self.contentView = [[UIView alloc] init];
  }
  return self;
}

- (void)applyLayoutModel:(LynxListLayoutModelLight *)model {
  self.frame = model.frame;
  self.columnIndex = model.columnIndex;
  self.layoutType = model.type;
  self.layer.zPosition = model.zIndex;
  self.contentView.layer.zPosition = model.zIndex;
}
@end

@implementation LynxListViewCellLightLynxUI

// The cell fetched from the reuse pool has no UI, as it has been removed in removeLynxUI.
- (void)addLynxUI:(LynxUI *)ui {
  self.ui = (LynxUIComponent *)ui;
  self.itemKey = self.ui.itemKey;
  [ui.view removeFromSuperview];
  [self.contentView addSubview:ui.view];
  [self addSubview:self.contentView];
  ((UIView *)self).frame = ui.view.frame;
  self.contentView.frame = CGRectMake(0, 0, ui.view.frame.size.width, ui.view.frame.size.height);
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
}

// Called in enqueue, every cell inside reusePool should not have a UI.
- (LynxUI *)removeLynxUI {
  LynxUI *ui = _ui;
  LynxBackgroundManager *mgr = ui.backgroundManager;
  if (mgr) {
    if (mgr.borderLayer) {
      [mgr.borderLayer removeFromSuperlayer];
    }
    if (mgr.backgroundLayer) {
      [mgr.backgroundLayer removeFromSuperlayer];
    }
  }
  _ui = nil;
  [ui.view removeFromSuperview];
  return ui;
}

- (void)adjustComponentFrame {
  self.ui.view.frame =
      CGRectMake(0, 0, self.ui.view.frame.size.width, self.ui.view.frame.size.height);
}

- (void)applyLayoutModel:(LynxListLayoutModelLight *)model {
  [super applyLayoutModel:model];
  [self adjustComponentFrame];
}

@end
