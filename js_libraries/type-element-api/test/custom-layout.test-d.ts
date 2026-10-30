// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType, describe, expectTypeOf, it } from 'vitest';
import type { ComposeElementRef, CustomLayoutMeasureCallback, CustomLayoutPlaceCallback, CustomLayoutSize, ElementRef } from '../types/index';

describe('Custom layout API types', () => {
  it('uses four bounds and synchronous size results', () => {
    expectTypeOf<Parameters<CustomLayoutMeasureCallback>>().toEqualTypeOf<[number, number, number, number]>();
    expectTypeOf<ReturnType<CustomLayoutMeasureCallback>>().toEqualTypeOf<CustomLayoutSize>();
    expectTypeOf<CustomLayoutSize>().toEqualTypeOf<{ width: number; height: number }>();
    expectTypeOf<ReturnType<CustomLayoutPlaceCallback>>().toEqualTypeOf<boolean>();
    expectTypeOf<() => Promise<CustomLayoutSize>>().not.toMatchTypeOf<CustomLayoutMeasureCallback>();
    expectTypeOf<() => void>().not.toMatchTypeOf<CustomLayoutPlaceCallback>();
  });

  it('installs or clears both callbacks on a Compose handle', () => {
    const usePolicy = (owner: ComposeElementRef, raw: ElementRef) => {
      const measure: CustomLayoutMeasureCallback = () => ({ width: 30, height: 20 });
      const place: CustomLayoutPlaceCallback = () => true;
      assertType<void>(__SetCustomLayout(owner, measure, place));
      assertType<void>(__SetCustomLayout(owner, null, null));

      // @ts-expect-error A raw Element cannot own a Compose policy.
      __SetCustomLayout(raw, measure, place);
      // @ts-expect-error Callbacks must be installed or cleared together.
      __SetCustomLayout(owner, measure, null);
      // @ts-expect-error Callbacks must be installed or cleared together.
      __SetCustomLayout(owner, null, place);
      // @ts-expect-error Clearing requires null, not undefined.
      __SetCustomLayout(owner, undefined, undefined);
      // @ts-expect-error Native measurement callbacks must return a size synchronously.
      __SetCustomLayout(owner, async () => ({ width: 30, height: 20 }), place);
    };
    expectTypeOf(usePolicy).toBeFunction();
  });

  it('measures raw children and current Compose mount roots', () => {
    const measureChildren = (owner: ComposeElementRef, child: ComposeElementRef) => {
      const children = __GetChildren(owner);
      assertType<ElementRef[]>(children);
      assertType<CustomLayoutSize>(__MeasureElement(owner, children[0], 0, -1, 0, -1));
      assertType<CustomLayoutSize>(__MeasureElement(owner, child, 30, 30, 20, 20));
      // @ts-expect-error The owner must be a Compose handle.
      __MeasureElement(children[0], child, 30, 30, 20, 20);
      // @ts-expect-error Every width and height bound is required.
      __MeasureElement(owner, child, 30, 30, 20);
    };
    expectTypeOf(measureChildren).toBeFunction();
  });

  it('requires an owner, child, coordinates and zIndex for placement', () => {
    const owner = {} as ComposeElementRef;
    const raw = {} as ElementRef;
    expectTypeOf<typeof __PlaceElement>().toBeCallableWith(owner, raw, 5, 6, 0);
    expectTypeOf<typeof __PlaceElement>().toBeCallableWith(owner, owner, 5, 6, 0.5);
    expectTypeOf<typeof __PlaceElement>().returns.toBeVoid();
    expectTypeOf<ElementRef>().not.toMatchTypeOf<Parameters<typeof __PlaceElement>[0]>();
    expectTypeOf<Parameters<typeof __PlaceElement>>().toEqualTypeOf<[ComposeElementRef, ElementRef | ComposeElementRef, number, number, number]>();
  });

  it('invalidates only a Compose policy owner', () => {
    expectTypeOf<Parameters<typeof __InvalidateLayout>>().toEqualTypeOf<[ComposeElementRef]>();
    expectTypeOf<typeof __InvalidateLayout>().returns.toBeVoid();
    expectTypeOf<ElementRef>().not.toMatchTypeOf<Parameters<typeof __InvalidateLayout>[0]>();
  });
});
