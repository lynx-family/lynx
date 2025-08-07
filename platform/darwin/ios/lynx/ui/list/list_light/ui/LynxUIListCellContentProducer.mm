// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListViewLight.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIComponent.h>
#import <Lynx/LynxUIListCellContentProducer.h>
#import <Lynx/LynxUIListLight.h>
#import <Lynx/LynxView+Internal.h>
#import "LynxContext+Internal.h"
#import "LynxTemplateRender+Internal.h"
#include "core/public/list_element_proxy.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/renderer/utils/diff_algorithm.h"
#include "core/shell/lynx_shell.h"

@interface LynxUIListCellContentProducer ()
@property(nonatomic, weak) LynxUIContext *UIContext;
@property(nonatomic, assign) NSUInteger operationIDCount;
@end

@implementation LynxUIListCellContentProducer

#pragma mark Init

- (void)setUIContext:(LynxUIContext *)UIContext {
  _UIContext = UIContext;
}

#pragma mark load

- (__kindof LynxListViewCellLight *)listView:(LynxListViewLight *)listView
                          cellForItemAtIndex:(NSInteger)index {
  LynxListViewCellLightLynxUI *cell =
      (LynxListViewCellLightLynxUI *)[listView dequeueReusableCellForIndex:index];
  if (listView.isAsync) {
    cell.updateToPath = index;
    cell.operationID = [self generateOperationId];
    [self asyncUIAtIndexPath:index operationID:cell.operationID];
  } else {
    LynxUI *ui = [self uiAtIndex:index];
    if (!ui) {
      return nil;
    }
    [cell addLynxUI:ui];
  }
  cell.updateToPath = index;
  [(UIView *)listView addSubview:cell];
  return cell;
}

#pragma mark update
- (id<LynxListCell>)listView:(LynxListViewLight *)listView
                  updateCell:(id<LynxListCell>)cell
               toItemAtIndex:(NSInteger)index {
  LynxListViewCellLightLynxUI *cellUI = (LynxListViewCellLightLynxUI *)cell;
  [(UIView *)listView addSubview:cellUI];
  cell.updateToPath = index;
  if (listView.isAsync) {
    cell.operationID = [self generateOperationId];
    [self asyncUIAtIndexPath:index operationID:cell.operationID];
  } else {
    LynxUI *ui = [self uiAtIndex:cellUI.updateToPath];
    if (!ui) {
      return nil;
    }
    [cellUI addLynxUI:ui];
  }
  return cell;
}

#pragma mark generate content

- (lynx::shell::ListElementProxy *)listElementProxy {
  if (!_UIContext.lynxContext) {
    return nullptr;
  }
  return _UIContext.lynxContext->list_element_proxy_.get();
}

- (LynxUI *)uiAtIndex:(NSInteger)index {
  auto *proxy = [self listElementProxy];
  if (!proxy) {
    return nil;
  }
  int32_t uiSign = proxy->GetLegacy()->ObtainListChild(
      static_cast<int32_t>(self.sign), static_cast<int32_t>(index), 0,
      self.needsInternalCellPrepareForReuseNotification);

  return [_UIContext.uiOwner findUIBySign:uiSign];
}

#pragma mark clear & enqueue
- (void)recycleLynxUI:(LynxUI *)ui {
  auto *proxy = [self listElementProxy];
  if (!proxy) {
    return;
  }
  proxy->GetLegacy()->RecycleListChild(static_cast<int32_t>(self.sign),
                                       static_cast<int32_t>(ui.sign));
}

- (void)recycleCell:(LynxListViewCellLight *)cell {
  LynxListViewCellLightLynxUI *UICell = (LynxListViewCellLightLynxUI *)cell;
  [self recycleLynxUI:(LynxUI *)UICell.ui];
}

- (void)clearCellContent:(LynxListViewCellLightLynxUI *)cell {
  if (cell.ui) {
    [self recycleCell:cell];
    cell.ui = nil;
  }
  [cell.contentView.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj,
                                                          NSUInteger idx, BOOL *_Nonnull stop) {
    [obj removeFromSuperview];
  }];
}

- (void)listView:(LynxListViewLight *)listView enqueueCell:(LynxListViewCellLightLynxUI *)cell {
  if (listView.isAsync) {
    [self asyncClearCellContent:cell];
  } else {
    [self clearCellContent:cell];
  }
}

#pragma mark Async
- (int64_t)generateOperationId {
  return (((int64_t)(self.sign)) << 32) + (int64_t)(self.operationIDCount++);
}

- (void)asyncUIAtIndexPath:(NSInteger)index operationID:(int64_t)operationID {
  auto *proxy = [self listElementProxy];
  if (!proxy) {
    return;
  }
  proxy->GetLegacy()->ObtainListChildAsync(static_cast<int32_t>(self.sign),
                                           static_cast<int32_t>(index), operationID,
                                           self.needsInternalCellPrepareForReuseNotification);
}

- (void)asyncClearCellContent:(LynxListViewCellLightLynxUI *)cell {
  if (cell.ui) {
    LynxUI *ui = [cell removeLynxUI];
    [self asyncRecycleLynxUI:ui];
  }
}

- (void)asyncRecycleLynxUI:(LynxUI *)ui {
  auto *proxy = [self listElementProxy];
  if (!proxy) {
    return;
  }
  proxy->GetLegacy()->RecycleListChildAsync(static_cast<int32_t>(self.sign),
                                            static_cast<int32_t>(ui.sign));
}

@end
