// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, it, expectTypeOf, assertType } from 'vitest';
import { MainThread } from '../../types';

describe('Animation Interface Type Tests', () => {
  it('should have correct AnimationOptions type', () => {
    const options: MainThread.AnimationOptions = {
      duration: 1000,
      delay: 100,
      iterations: 3,
      direction: 'alternate',
      easing: 'ease-in-out',
      fill: 'both',
      name: 'test-animation',
      'play-state': 'running',
    };

    expectTypeOf<MainThread.AnimationOptions>().toEqualTypeOf<typeof options>();
  });

  it('should allow optional AnimationOptions properties', () => {
    assertType<MainThread.AnimationOptions>({});
    assertType<MainThread.AnimationOptions>({
      duration: 500,
      iterations: Infinity,
    });
  });

  it('should have correct KeyframeEffect type', () => {
    const keyframes = [
      { opacity: 0, transform: 'translateX(0px)' },
      { opacity: 1, transform: 'translateX(100px)' },
    ];

    const options: MainThread.AnimationOptions = {
      duration: 1000,
      easing: 'linear',
    };

    // Mock element for testing
    const mockElement = {} as MainThread.Element;

    const animation = mockElement.animate(keyframes, options);

    assertType<MainThread.KeyframeEffect>(animation.effect);
  });

  it('should have correct Animation type', () => {
    // Mock effect for testing
    const mockEffect = {} as MainThread.KeyframeEffect;

    const animation: MainThread.Animation = {
      effect: mockEffect,
      id: 'animation-123',
      cancel: () => {},
      pause: () => {},
      play: () => {},
    };

    expectTypeOf<MainThread.Animation>().toEqualTypeOf<typeof animation>();
    expectTypeOf<MainThread.Animation['effect']>().toEqualTypeOf<MainThread.KeyframeEffect>();
    expectTypeOf<MainThread.Animation['id']>().toEqualTypeOf<string>();
    expectTypeOf<MainThread.Animation['cancel']>().toBeCallableWith();
    expectTypeOf<MainThread.Animation['cancel']>().returns.toBeVoid();
    expectTypeOf<MainThread.Animation['pause']>().toBeCallableWith();
    expectTypeOf<MainThread.Animation['pause']>().returns.toBeVoid();
    expectTypeOf<MainThread.Animation['play']>().toBeCallableWith();
    expectTypeOf<MainThread.Animation['play']>().returns.toBeVoid();
  });

  it('should have correct readonly property types on KeyframeEffect', () => {
    // Test that readonly properties have the correct types
    expectTypeOf<MainThread.KeyframeEffect['target']>().toEqualTypeOf<MainThread.Element>();
    expectTypeOf<MainThread.KeyframeEffect['keyframes']>().toEqualTypeOf<Record<string, string | number>[]>();
    expectTypeOf<MainThread.KeyframeEffect['options']>().toEqualTypeOf<MainThread.AnimationOptions>();
  });

  it('should have correct readonly property types on Animation', () => {
    // Test that readonly properties have the correct types
    expectTypeOf<MainThread.Animation['effect']>().toEqualTypeOf<MainThread.KeyframeEffect>();
    expectTypeOf<MainThread.Animation['id']>().toEqualTypeOf<string>();
  });
});
