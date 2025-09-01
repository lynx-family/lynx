// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';

import {
  UIMethods,
  IntrinsicElements,
  InputFocusEvent,
  BaseEvent,
  InputBlurEvent,
  InputConfirmEvent,
  InputInputEvent,
  InputSelectionEvent,
  TextAreaSelectionChangeEvent,
  TextAreaInputEvent,
  TextAreaConfirmEvent,
  TextAreaBlurEvent,
  TextAreaFocusEvent,
  NodesRef,
  InputUIMethods,
} from '../../types';
import { expectError } from 'tsd';

// Props Types Check
let a;
{
  <input readonly={true} />;
  <textarea readonly={false} />;
  assertType<boolean | undefined>(a as IntrinsicElements['input']['readonly']);
  assertType<boolean | undefined>(a as IntrinsicElements['textarea']['readonly']);
  expectError(() => {
    // @ts-expect-error type error
    <input readonly={'text'} />;
    // @ts-expect-error type error
    <textarea readonly={'text'} />;
  });

  <input type={'text'} />;
  <textarea type={'number'} />;
  assertType<'text' | 'number' | 'digit' | 'password' | 'tel' | 'email' | undefined>(a as IntrinsicElements['input']['type']);
  assertType<'text' | 'number' | 'digit' | 'password' | 'tel' | 'email' | undefined>(a as IntrinsicElements['textarea']['type']);
  expectError(() => {
    // @ts-expect-error type error
    <input type={true} />;
    // @ts-expect-error type error
    <textarea type={true} />;
  });

  <input maxlength={10} />;
  <textarea maxlength={11} />;
  assertType<number | undefined>(a as IntrinsicElements['input']['maxlength']);
  assertType<number | undefined>(a as IntrinsicElements['textarea']['maxlength']);
  expectError(() => {
    // @ts-expect-error type error
    <input maxlength={true} />;
    // @ts-expect-error type error
    <textarea maxlength={true} />;
  });

  <textarea maxlines={11} />;
  assertType<number | undefined>(a as IntrinsicElements['textarea']['maxlines']);
  expectError(() => {
    // @ts-expect-error type error
    <textarea maxlines={true} />;
  });

  <input confirm-type="done" />;
  <textarea confirm-type="done" />;
  assertType<'done' | 'go' | 'next' | 'search' | 'send' | undefined>(a as IntrinsicElements['input']['confirm-type']);
  assertType<'done' | 'go' | 'next' | 'search' | 'send' | undefined>(a as IntrinsicElements['textarea']['confirm-type']);
  expectError(() => {
    // @ts-expect-error type error
    <input confirm-type={true} />;
    // @ts-expect-error type error
    <textarea confirm-type={'eles'} />;
  });

  <input show-soft-input-on-focus={true} />;
  <textarea show-soft-input-on-focus={false} />;
  assertType<boolean | undefined>(a as IntrinsicElements['input']['show-soft-input-on-focus']);
  assertType<boolean | undefined>(a as IntrinsicElements['textarea']['show-soft-input-on-focus']);
  expectError(() => {
    // @ts-expect-error type error
    <input show-soft-input-on-focus={1} />;
    // @ts-expect-error type error
    <textarea show-soft-input-on-focus={100} />;
  });

  <input input-filter={'[A-Za-z0-9]'} />;
  <textarea input-filter={'[A-Za-z0-9]'} />;
  assertType<string | undefined>(a as IntrinsicElements['input']['input-filter']);
  assertType<string | undefined>(a as IntrinsicElements['textarea']['input-filter']);
  expectError(() => {
    // @ts-expect-error type error
    <input input-filter={1} />;
    // @ts-expect-error type error
    <textarea input-filter={100} />;
  });

  <textarea line-spacing={'10px'} />;
  assertType<number | `${number}px` | `${number}rpx` | undefined>(a as IntrinsicElements['textarea']['line-spacing']);
  expectError(() => {
    // @ts-expect-error type error
    <textarea line-spacing={'100'} />;
  });

  <input placeholder="placeholder" />;
  <textarea placeholder="placeholder" />;
  assertType<string | undefined>(a as IntrinsicElements['input']['placeholder']);
  assertType<string | undefined>(a as IntrinsicElements['textarea']['placeholder']);
  expectError(() => {
    // @ts-expect-error type error
    <input placeholder={1} />;
    // @ts-expect-error type error
    <textarea placeholder={100} />;
  });
}

// Events types check
function noop() {}
{
  <input bindtap={noop} />;
  <input
    bindfocus={(e: BaseEvent<'bindfocus', InputFocusEvent>) => {
      assertType<string>(e.detail.value);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
      });
    }}
  />;
  <textarea
    bindfocus={(e: BaseEvent<'bindfocus', TextAreaFocusEvent>) => {
      assertType<string>(e.detail.value);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
      });
    }}
  />;
  <input
    bindblur={(e: BaseEvent<'bindblur', InputBlurEvent>) => {
      assertType<string>(e.detail.value);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
      });
    }}
  />;
  <textarea
    bindblur={(e: BaseEvent<'bindblur', TextAreaBlurEvent>) => {
      assertType<string>(e.detail.value);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
      });
    }}
  />;
  <input
    bindconfirm={(e: BaseEvent<'bindconfirm', InputConfirmEvent>) => {
      assertType<string>(e.detail.value);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
      });
    }}
  />;
  <textarea
    bindconfirm={(e: BaseEvent<'bindconfirm', TextAreaConfirmEvent>) => {
      assertType<string>(e.detail.value);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
      });
    }}
  />;
  <input
    bindinput={(e: BaseEvent<'bindinput', InputInputEvent>) => {
      assertType<string>(e.detail.value);
      assertType<number>(e.detail.selectionStart);
      assertType<number>(e.detail.selectionEnd);
      assertType<boolean | undefined>(e.detail.isComposing);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
        // @ts-expect-error type error
        assertType<string>(e.detail.selectionStart);
        // @ts-expect-error type error
        assertType<string>(e.detail.selectionEnd);
        // @ts-expect-error type error
        assertType<number>(e.detail.isComposing);
      });
    }}
  />;
  <textarea
    bindinput={(e: BaseEvent<'bindinput', TextAreaInputEvent>) => {
      assertType<string>(e.detail.value);
      assertType<number>(e.detail.selectionStart);
      assertType<number>(e.detail.selectionEnd);
      assertType<boolean | undefined>(e.detail.isComposing);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
        // @ts-expect-error type error
        assertType<string>(e.detail.selectionStart);
        // @ts-expect-error type error
        assertType<string>(e.detail.selectionEnd);
        // @ts-expect-error type error
        assertType<number>(e.detail.isComposing);
      });
    }}
  />;
  <input
    bindselection={(e: BaseEvent<'bindselection', InputSelectionEvent>) => {
      assertType<number>(e.detail.selectionStart);
      assertType<number>(e.detail.selectionEnd);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
        // @ts-expect-error type error
        assertType<string>(e.detail.selectionStart);
        // @ts-expect-error type error
        assertType<string>(e.detail.selectionEnd);
        // @ts-expect-error type error
        assertType<number>(e.detail.isComposing);
      });
    }}
  />;
  <textarea
    bindselection={(e: BaseEvent<'bindselection', TextAreaSelectionChangeEvent>) => {
      assertType<number>(e.detail.selectionStart);
      assertType<number>(e.detail.selectionEnd);
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e.detail.value);
        // @ts-expect-error type error
        assertType<string>(e.detail.selectionStart);
        // @ts-expect-error type error
        assertType<string>(e.detail.selectionEnd);
        // @ts-expect-error type error
        assertType<number>(e.detail.isComposing);
      });
    }}
  />;
}

// UIMethods types check
function invoke<T extends keyof UIMethods>(_param: UIMethods[T]) {}

{
  invoke<'input'>({
    method: 'focus',
  });
  invoke<'textarea'>({
    method: 'focus',
  });
  invoke<'input'>({
    method: 'blur',
  });
  invoke<'textarea'>({
    method: 'blur',
  });
  invoke<'input'>({
    method: 'setValue',
    params: {
      value: '123',
    },
  });
  invoke<'textarea'>({
    method: 'setValue',
    params: {
      value: '123',
    },
  });
  invoke<'input'>({
    method: 'setSelectionRange',
    params: {
      selectionStart: 0,
      selectionEnd: 0,
    },
  });
  invoke<'textarea'>({
    method: 'setSelectionRange',
    params: {
      selectionStart: 0,
      selectionEnd: 0,
    },
  });

  let a: unknown;
  assertType<'focus' | 'blur' | 'getValue' | 'setValue' | 'setSelectionRange'>(a as UIMethods['input']['method']);
  assertType<'focus' | 'blur' | 'getValue' | 'setValue' | 'setSelectionRange'>(a as UIMethods['textarea']['method']);
}
