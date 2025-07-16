// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
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
      assertType<number>(e.detail.lineCount);
      assertType<TextLineInfo[]>(e.detail.lines);
      assertType<{ width: number; height: number }>(e.detail.size);
    }}
  />;
  <text
    bindselectionchange={(e: SelectionChangeEvent) => {
      assertType<number>(e.detail.start);
      assertType<number>(e.detail.end);
      assertType<'forward' | 'backward'>(e.detail.direction);
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
      assertType<{
        boundingRect: {
          left: number;
          right: number;
          top: number;
          bottom: number;
          width: number;
          height: number;
        };
        boxes: {
          left: number;
          right: number;
          top: number;
          bottom: number;
          width: number;
          height: number;
        }[];
        handles: {
          x: number;
          y: number;
          radius: number;
        }[];
      }>(res);
    },
  });

  invoke<'text'>({
    method: 'getTextBoundingRect',
    params: {
      start: 1,
      end: 1,
    },
    success: (res) => {
      assertType<{
        boundingRect: {
          left: number;
          right: number;
          top: number;
          bottom: number;
          width: number;
          height: number;
        };
        boxes: {
          left: number;
          right: number;
          top: number;
          bottom: number;
          width: number;
          height: number;
        }[];
      }>(res);
    },
  });

  invoke<'text'>({
    method: 'getSelectedText',
    success: (res) => {
      assertType<{
        selectedText: string;
      }>(res);
    },
  });
}
