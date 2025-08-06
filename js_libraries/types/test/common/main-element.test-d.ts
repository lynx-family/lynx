// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, it, expectTypeOf } from 'vitest';
import { MainThread } from '../../types';

describe('Main Thread Element Interface Type Tests', () => {
  it('should have correct getAttribute type', () => {
    expectTypeOf<MainThread.Element['getAttribute']>().toBeCallableWith('class');
    expectTypeOf<MainThread.Element['getAttribute']>().returns.toBeAny();
  });

  it('should have correct getAttributeNames type', () => {
    expectTypeOf<MainThread.Element['getAttributeNames']>().toBeCallableWith();
    expectTypeOf<MainThread.Element['getAttributeNames']>().returns.toEqualTypeOf<string[]>();
  });

  it('should have correct setAttribute type', () => {
    expectTypeOf<MainThread.Element['setAttribute']>().toBeCallableWith('id', 'test');
    expectTypeOf<MainThread.Element['setAttribute']>().toBeCallableWith('disabled', true);
    expectTypeOf<MainThread.Element['setAttribute']>().returns.toBeVoid();
  });

  it('should have correct setStyleProperty type', () => {
    expectTypeOf<MainThread.Element['setStyleProperty']>().toBeCallableWith('font-size', '16px');
    expectTypeOf<MainThread.Element['setStyleProperty']>().returns.toBeVoid();
  });

  it('should have correct setStyleProperties type', () => {
    const styles = { 'background-color': 'red', 'font-weight': 'bold' };
    expectTypeOf<MainThread.Element['setStyleProperties']>().toBeCallableWith(styles);
    expectTypeOf<MainThread.Element['setStyleProperties']>().returns.toBeVoid();
  });

  it('should have correct querySelector type', () => {
    expectTypeOf<MainThread.Element['querySelector']>().toBeCallableWith('.container');
    expectTypeOf<MainThread.Element['querySelector']>().returns.toEqualTypeOf<MainThread.Element | null>();
  });

  it('should have correct querySelectorAll type', () => {
    expectTypeOf<MainThread.Element['querySelectorAll']>().toBeCallableWith('view');
    expectTypeOf<MainThread.Element['querySelectorAll']>().returns.toEqualTypeOf<MainThread.Element[]>();
  });

  it('should have correct invoke type', () => {
    expectTypeOf<MainThread.Element['invoke']>().toBeCallableWith('refresh');
    expectTypeOf<MainThread.Element['invoke']>().toBeCallableWith('update', { data: 123 });
    expectTypeOf<MainThread.Element['invoke']>().returns.toEqualTypeOf<Promise<any>>();
  });
});
