// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import {
  BaseEvent,
  IntrinsicElements,
  MarkdownAnimationStepEvent,
  MarkdownLinkEvent,
  MarkdownOverflowEvent,
  MarkdownProps,
  MarkdownSelectionChangeEvent,
  MarkdownUIMethods,
  UIMethods,
} from '../../types';

let a: unknown;

{
  <markdown content="" />;
  assertType<string>(a as IntrinsicElements['markdown']['content']);

  <markdown content="" text-selection={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['markdown']['text-selection']);

  <markdown content="" enable-selection={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['markdown']['enable-selection']);

  <markdown content="" allow-break-around-punctuation={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['markdown']['allow-break-around-punctuation']);

  <markdown content="" enable-break-around-punctuation={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['markdown']['enable-break-around-punctuation']);

  <markdown animation-type="line-expand" content="" />;
  assertType<'none' | 'typewriter' | 'line-expand' | undefined>(a as IntrinsicElements['markdown']['animation-type']);

  assertType<MarkdownProps>(a as IntrinsicElements['markdown']);
}

{
  <markdown binddrawStart={(e: BaseEvent) => {}} content="" />;

  <markdown
    bindanimationStep={(e: BaseEvent<'bindanimationStep', MarkdownAnimationStepEvent>) => {
      assertType<number>(e.detail.animationStep);
      assertType<number>(e.detail.maxAnimationStep);
    }}
    content=""
  />;

  <markdown
    bindoverflow={(e: BaseEvent<'bindoverflow', MarkdownOverflowEvent>) => {
      assertType<'ellipsis' | 'clip'>(e.detail.type);
    }}
    content=""
  />;

  <markdown
    bindlink={(e: BaseEvent<'bindlink', MarkdownLinkEvent>) => {
      assertType<string>(e.detail.url);
      assertType<string>(e.detail.content);
    }}
    content=""
  />;

  <markdown
    bindselectionchange={(e: BaseEvent<'bindselectionchange', MarkdownSelectionChangeEvent>) => {
      assertType<number>(e.detail.start);
      assertType<number>(e.detail.end);
      assertType<'forward' | 'backward'>(e.detail.direction);
    }}
    content=""
  />;
}

function invoke<T extends keyof { markdown: MarkdownUIMethods }>(_param: { markdown: MarkdownUIMethods }[T]) {}

{
  invoke<'markdown'>({
    method: 'getContent',
    params: {},
    success: (res) => {
      assertType<string>(res.content);
    },
  });

  invoke<'markdown'>({
    method: 'setTextSelection',
    params: {
      startX: 0,
      startY: 0,
      endX: 1,
      endY: 1,
    },
    success: (res) => {
      assertType<{ x: number; y: number; radius: number }[]>(res.handles);
    },
  });

  invoke<'markdown'>({
    method: 'getSelectedText',
    success: (res) => {
      assertType<string>(res.selectedText);
    },
  });

  assertType<MarkdownUIMethods>(a as UIMethods['markdown']);
  assertType<'pauseAnimation' | 'resumeAnimation' | 'getContent' | 'getSelectedText' | 'setTextSelection' | 'getCharIndexByPoint' | 'getImages' | 'getParseResult'>(
    a as UIMethods['markdown']['method']
  );
}
