// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LayoutEvent, TextLineInfo, UIMethods } from '../../types';
import type { SelectionChangeEvent } from '../../types/common/element/text';

// Props Types Check
{
  <text text-maxline={'1'} />;
  <text text-maxlength={'1'} />;
  <text enable-font-scaling={true} />;
  <text text-vertical-align={'top'} />;
  <text text-vertical-align={'center'} />;
  <text text-vertical-align={'bottom'} />;
  <text tail-color-convert={false} />;
  <text text-single-line-vertical-align={'normal'} />;
  <text text-single-line-vertical-align={'bottom'} />;
  <text text-single-line-vertical-align={'center'} />;
  <text text-single-line-vertical-align={'top'} />;
  <text include-font-padding={false} />;
  <text android-emoji-compat={false} />;
  <text text-fake-bold={false} />;
  <text text-selection={true} />;
  <text custom-context-menu={true} />;
  <text custom-text-selection={true} />;
}

// Events types check
function noop() {}
{
  <text bindtap={noop}></text>;
  <text
    bindlayout={(e: LayoutEvent) => {
      e.detail.lineCount = 1;
      e.detail.lines = [
        {
          start: 1,
          end: 1,
          ellipsisCount: 1,
        } as TextLineInfo,
      ];
      e.detail.size.width = 1;
      e.detail.size.height = 1;
    }}
  />;
  <text
    bindselectionchange={(e: SelectionChangeEvent) => {
      e.detail.start = 1;
      e.detail.end = 1;
      e.detail.direction = 'forward';
      e.detail.direction = 'backward';
    }}
  />;
}

// UIMethods types check
function invoke<T extends keyof UIMethods>(_param: UIMethods[T]) {}

{
  invoke<'text'>({
    method: 'setTextSelection',
    params: {
      startX: 1,
      startY: 1,
      endX: 1,
      endY: 1,
      showStartHandle: true,
      showEndHandle: true,
    },
    success: (res) => {
      res.boundingRect.left = 1;
      res.boundingRect.right = 1;
      res.boundingRect.top = 1;
      res.boundingRect.bottom = 1;
      res.boundingRect.width = 1;
      res.boundingRect.height = 1;

      res.boxes[1].left = 1;
      res.boxes[1].right = 1;
      res.boxes[1].top = 1;
      res.boxes[1].bottom = 1;
      res.boxes[1].width = 1;
      res.boxes[1].height = 1;

      res.handles[1].x = 1;
      res.handles[1].y = 1;
      res.handles[1].radius = 1;
    },
  });

  invoke<'text'>({
    method: 'getTextBoundingRect',
    params: {
      start: 1,
      end: 1,
    },
    success: (res) => {
      res.boundingRect.left = 1;
      res.boundingRect.right = 1;
      res.boundingRect.top = 1;
      res.boundingRect.bottom = 1;
      res.boundingRect.width = 1;
      res.boundingRect.height = 1;

      res.boxes[1].left = 1;
      res.boxes[1].right = 1;
      res.boxes[1].top = 1;
      res.boxes[1].bottom = 1;
      res.boxes[1].width = 1;
      res.boxes[1].height = 1;
    },
  });

  invoke<'text'>({
    method: 'getSelectedText',
    success: (res) => {
      res.selectedText = '';
    },
  });
}
