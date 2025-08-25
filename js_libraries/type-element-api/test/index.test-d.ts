import { assertType, describe, expectTypeOf, it } from 'vitest';
import { AnimationOperation, AnimationTimingOptions, Keyframe, ElementRef, ComponentElementRef, PageElementRef, ListElementRef, ViewElementRef } from '../types/index';

describe('Test Animation Types', () => {
  it('should have correct AnimationOperation type', () => {
    expectTypeOf<AnimationOperation>().toBeNumber();
    expectTypeOf<AnimationOperation.START>().toBeNumber();
    expectTypeOf<AnimationOperation.PLAY>().toBeNumber();
    expectTypeOf<AnimationOperation.PAUSE>().toBeNumber();
    expectTypeOf<AnimationOperation.CANCEL>().toBeNumber();
  });

  it('should have correct AnimationTimingOptions type', () => {
    expectTypeOf<AnimationTimingOptions>().toBeObject();
    expectTypeOf<AnimationTimingOptions>().toEqualTypeOf<{
      name?: string;
      duration?: number | string;
      delay?: number | string;
      iterationCount?: number | string;
      fillMode?: string;
      timingFunction?: string;
      direction?: string;
    }>();
  });

  it('should have correct Keyframe type', () => {
    expectTypeOf<Keyframe>().toBeObject();
    expectTypeOf<Record<string, string | number>>().toEqualTypeOf<Keyframe>();
  });
});

describe('Test Element API Types', () => {
  it('should have correct ElementRef types', () => {
    expectTypeOf<ElementRef>().toBeObject();
    expectTypeOf<ComponentElementRef>().toEqualTypeOf<ElementRef>();
    expectTypeOf<PageElementRef>().toEqualTypeOf<ComponentElementRef>();
    expectTypeOf<ListElementRef>().toEqualTypeOf<ElementRef>();
    expectTypeOf<ViewElementRef>().toEqualTypeOf<ElementRef>();
  });

  it('should have correct global functions available', () => {
    expectTypeOf<typeof __CreatePage>().toBeFunction();
    expectTypeOf<typeof __CreateView>().toBeFunction();
    expectTypeOf<typeof __CreateText>().toBeFunction();
    expectTypeOf<typeof __ElementAnimate>().toBeFunction();
  });

  it('should test __ElementAnimate function signature', () => {
    const element = __CreateView(0);

    // Test that __ElementAnimate is a function
    expectTypeOf<typeof __ElementAnimate>().toBeFunction();

    // Test that it accepts ElementRef as first parameter
    expectTypeOf<typeof __ElementAnimate>().toBeCallableWith(element, [
      AnimationOperation.START,
      'test-animation',
      [{ opacity: 0 }, { opacity: 1 }],
      { duration: 1000, timingFunction: 'ease-in-out' },
    ]);

    // Test that it accepts pause operation overload
    expectTypeOf<typeof __ElementAnimate>().toBeCallableWith(element, [AnimationOperation.PAUSE, 'test-animation']);

    // Test that it accepts play operation overload
    expectTypeOf<typeof __ElementAnimate>().toBeCallableWith(element, [AnimationOperation.PLAY, 'test-animation']);

    // Test that it accepts cancel operation overload
    expectTypeOf<typeof __ElementAnimate>().toBeCallableWith(element, [AnimationOperation.CANCEL, 'test-animation']);
  });
});
