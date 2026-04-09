import { assertType, describe, expect, expectTypeOf, it } from 'vitest';
import { StandardProps, Target, TouchEvent, MainThread, AnimationEvent, MouseEvent } from '../../types';

describe('Test Basic Event Binding', () => {
  it('Test event bind', () => {
    // bind and capture-bind
    expectTypeOf<StandardProps['bindtap']>().exclude(undefined).toExtend<(e: TouchEvent) => void>();
    expectTypeOf<StandardProps['catchtap']>().exclude(undefined).toExtend<(e: TouchEvent) => void>();
    expectTypeOf<StandardProps['capture-bindtap']>().exclude(undefined).toExtend<(e: TouchEvent) => void>();
  });

  it('Test event object', () => {
    expectTypeOf<StandardProps['bindtap']>().parameter(0).toEqualTypeOf<TouchEvent>();
    expectTypeOf<StandardProps['bindtap']>().parameter(0).toEqualTypeOf<TouchEvent>();

    expectTypeOf<StandardProps['bindtap']>().exclude(undefined).parameter(0).toHaveProperty('type').toEqualTypeOf<string>();
    expectTypeOf<StandardProps['bindtap']>().exclude(undefined).parameter(0).toHaveProperty('target').toEqualTypeOf<Target>();
    expectTypeOf<StandardProps['bindtap']>().exclude(undefined).parameter(0).toHaveProperty('detail').toEqualTypeOf<{ x: number; y: number }>();
  });

  it('Test MainThread Event bind', () => {
    // bind and capture-bind
    expectTypeOf<StandardProps['main-thread:bindtap']>().exclude(undefined).toExtend<(e: MainThread.TouchEvent) => void>();
    expectTypeOf<StandardProps['main-thread:catchtap']>().exclude(undefined).toExtend<(e: MainThread.TouchEvent) => void>();
    expectTypeOf<StandardProps['main-thread:capture-bindtap']>().exclude(undefined).toExtend<(e: MainThread.TouchEvent) => void>();
  });

  it('Test MainThread Event object', () => {
    expectTypeOf<StandardProps['main-thread:bindtap']>().parameter(0).toEqualTypeOf<MainThread.TouchEvent>();

    expectTypeOf<StandardProps['main-thread:bindtap']>().exclude(undefined).parameter(0).toHaveProperty('type').toEqualTypeOf<string>();
    expectTypeOf<StandardProps['main-thread:bindtap']>().exclude(undefined).parameter(0).toHaveProperty('target').toEqualTypeOf<MainThread.Element>();
    expectTypeOf<StandardProps['main-thread:bindtap']>().exclude(undefined).parameter(0).toHaveProperty('detail').toEqualTypeOf<{ x: number; y: number }>();
  });

  it('Test MainThread Exports', () => {
    // MainThread.TouchEvent
    expectTypeOf<StandardProps['main-thread:bindtap']>().parameter(0).toEqualTypeOf<MainThread.TouchEvent>();
    expectTypeOf<StandardProps['main-thread:bindanimationstart']>().parameter(0).toEqualTypeOf<MainThread.AnimationEvent>();
    expectTypeOf<StandardProps['main-thread:bindtransitionstart']>().parameter(0).toEqualTypeOf<MainThread.TransitionEvent>();
    expectTypeOf<StandardProps['main-thread:bindlayoutchange']>().parameter(0).toEqualTypeOf<MainThread.LayoutChangeEvent>();
    expectTypeOf<StandardProps['main-thread:binduiappear']>().parameter(0).toEqualTypeOf<MainThread.UIAppearanceEvent>();
  });
});

describe('Test Animation Events', () => {
  it('animation event bind', () => {
    // bind and capture-bind
    expectTypeOf<StandardProps['bindanimationstart']>().exclude(undefined).toExtend<(e: AnimationEvent) => void>();
    expectTypeOf<StandardProps['bindanimationend']>().exclude(undefined).toExtend<(e: AnimationEvent) => void>();
    expectTypeOf<StandardProps['bindanimationcancel']>().exclude(undefined).toExtend<(e: AnimationEvent) => void>();
    expectTypeOf<StandardProps['bindanimationiteration']>().exclude(undefined).toExtend<(e: AnimationEvent) => void>();
  });

  it('Test event object', () => {
    expectTypeOf<StandardProps['bindanimationstart']>().parameter(0).toEqualTypeOf<AnimationEvent>();

    expectTypeOf<StandardProps['bindanimationstart']>().exclude(undefined).parameter(0).toHaveProperty('type').toEqualTypeOf<string>();
    expectTypeOf<StandardProps['bindanimationstart']>().exclude(undefined).parameter(0).toHaveProperty('target').toEqualTypeOf<Target>();
    expectTypeOf<StandardProps['bindanimationstart']>().exclude(undefined).parameter(0).toHaveProperty('params').toEqualTypeOf<{
      animation_type: 'keyframe-animation';
      animation_name: string;
      new_animator?: true;
    }>();
  });

  it('Test MainThread Event bind', () => {
    // bind and capture-bind
    expectTypeOf<StandardProps['main-thread:bindanimationstart']>().exclude(undefined).toExtend<(e: MainThread.AnimationEvent) => void>();
    expectTypeOf<StandardProps['main-thread:bindanimationend']>().exclude(undefined).toExtend<(e: MainThread.AnimationEvent) => void>();
    expectTypeOf<StandardProps['main-thread:bindanimationcancel']>().exclude(undefined).toExtend<(e: MainThread.AnimationEvent) => void>();
    expectTypeOf<StandardProps['main-thread:bindanimationiteration']>().exclude(undefined).toExtend<(e: MainThread.AnimationEvent) => void>();
  });

  it('Test MainThread Event object', () => {
    expectTypeOf<StandardProps['main-thread:bindanimationstart']>().parameter(0).toEqualTypeOf<MainThread.AnimationEvent>();

    expectTypeOf<StandardProps['main-thread:bindanimationstart']>().exclude(undefined).parameter(0).toHaveProperty('type').toEqualTypeOf<string>();
    expectTypeOf<StandardProps['main-thread:bindanimationstart']>().exclude(undefined).parameter(0).toHaveProperty('target').toEqualTypeOf<MainThread.Element>();
    expectTypeOf<StandardProps['main-thread:bindanimationstart']>().exclude(undefined).parameter(0).toHaveProperty('params').toEqualTypeOf<{
      animation_type: 'keyframe-animation';
      animation_name: string;
      new_animator?: true;
    }>();
  });

  it('Test MainThread Exports', () => {
    // MainThread.TouchEvent
    expectTypeOf<StandardProps['main-thread:bindtap']>().parameter(0).toEqualTypeOf<MainThread.TouchEvent>();
    expectTypeOf<StandardProps['main-thread:bindtouchstart']>().parameter(0).toEqualTypeOf<MainThread.TouchEvent>();
    expectTypeOf<StandardProps['main-thread:bindtouchmove']>().parameter(0).toEqualTypeOf<MainThread.TouchEvent>();
    expectTypeOf<StandardProps['main-thread:bindtouchend']>().parameter(0).toEqualTypeOf<MainThread.TouchEvent>();
  });
});

describe('Test Mouse Events', () => {
  it('mouse enter and leave bind', () => {
    expectTypeOf<StandardProps['bindmouseenter']>().exclude(undefined).toExtend<(e: MouseEvent) => void>();
    expectTypeOf<StandardProps['bindmouseleave']>().exclude(undefined).toExtend<(e: MouseEvent) => void>();
    expectTypeOf<StandardProps['main-thread:bindmouseenter']>().exclude(undefined).toExtend<(e: MainThread.MouseEvent) => void>();
    expectTypeOf<StandardProps['main-thread:bindmouseleave']>().exclude(undefined).toExtend<(e: MainThread.MouseEvent) => void>();
  });

  it('mouse enter and leave event object', () => {
    expectTypeOf<StandardProps['bindmouseenter']>().parameter(0).toEqualTypeOf<MouseEvent>();
    expectTypeOf<StandardProps['bindmouseleave']>().parameter(0).toEqualTypeOf<MouseEvent>();
    expectTypeOf<StandardProps['main-thread:bindmouseenter']>().parameter(0).toEqualTypeOf<MainThread.MouseEvent>();
    expectTypeOf<StandardProps['main-thread:bindmouseleave']>().parameter(0).toEqualTypeOf<MainThread.MouseEvent>();
  });
});
