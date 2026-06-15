// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { expectError } from 'tsd';
import {
  ContentEditable,
  IntrinsicElements,
  LayoutEvent,
  TextLineInfo,
  UIMethods,
} from '../../types';
import type { SelectionChangeEvent } from '../../types/common/element/text';

// Props Types Check
let a;
{
  <text contenteditable />;
  <text contenteditable={true} />;
  <text contenteditable={false} />;
  <text contenteditable="" />;
  <text contenteditable="true" />;
  <text contenteditable="false" />;
  <x-markdown contenteditable />;
  <x-markdown contenteditable="" />;
  <inline-text contenteditable />;
  <inline-text contenteditable="true" />;
  <inline-text text-maxline={'1'} />;
  assertType<ContentEditable | undefined>(
    a as IntrinsicElements['text']['contenteditable']
  );
  assertType<ContentEditable | undefined>(
    a as IntrinsicElements['x-markdown']['contenteditable']
  );
  assertType<ContentEditable | undefined>(
    a as IntrinsicElements['inline-text']['contenteditable']
  );
  expectError(() => {
    // @ts-expect-error type error
    <text contenteditable="inherit" />;
    // @ts-expect-error type error
    <text contenteditable="plaintext-only" />;
    // @ts-expect-error type error
    <x-markdown contenteditable="inherit" />;
    // @ts-expect-error type error
    <x-markdown contenteditable={1} />;
  });

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
    method: 'setEditableSelectionRange',
    params: {
      start: 1,
      end: 2,
    },
    success: (res) => {
      assertType<{
        start: number;
        end: number;
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

  invoke<'x-markdown'>({
    method: 'applySourcePatch',
    params: {
      start: 1,
      end: 2,
      replacement: '**',
    },
    success: (res) => {
      assertType<{
        value: string;
        selectionStart: number;
        selectionEnd: number;
        applied: boolean;
        fallback: boolean;
        error?: string;
      }>(res);
    },
  });
}
