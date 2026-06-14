import { assertType, describe, expectTypeOf, it } from 'vitest';
import type {
  AnimationOperation,
  AnimationTimingOptions,
  Keyframe,
  ElementRef,
  ComponentElementRef,
  PageElementRef,
  ListElementRef,
  ViewElementRef,
  SerializedTemplateInstance,
  SerializableValue,
  SerializedTypedTemplateInstance,
} from '../types/index';

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
    expectTypeOf<typeof __CreateComponent>().toBeFunction();
    expectTypeOf<typeof __CreateComponent>().toBeCallableWith(1, 'component-id', 2, '', 'component-name', 'component/path');
    expectTypeOf<typeof __CreateComponent>().toBeCallableWith(1, 'component-id', 2, '', 'component-name', 'component/path', {}, { nodeIndex: 42 });
    expectTypeOf<typeof __CreateView>().toBeFunction();
    expectTypeOf<typeof __CreateText>().toBeFunction();
    expectTypeOf<typeof __ElementAnimate>().toBeFunction();
    expectTypeOf<typeof __CreateElementTemplate>().toBeFunction();
    expectTypeOf<typeof __SetAttributeOfElementTemplate>().toBeFunction();
    expectTypeOf<typeof __InsertNodeToElementTemplate>().toBeFunction();
    expectTypeOf<typeof __RemoveNodeFromElementTemplate>().toBeFunction();
    expectTypeOf<typeof __SerializeElementTemplate>().toBeFunction();
  });

  it('should test __ElementAnimate function signature', () => {
    const element = {} as ElementRef;

    // Test that __ElementAnimate is a function
    expectTypeOf<typeof __ElementAnimate>().toBeFunction();

    // Test that it accepts ElementRef as first parameter
    expectTypeOf<typeof __ElementAnimate>().toBeCallableWith(element, [
      0 as AnimationOperation.START,
      'test-animation',
      [{ opacity: 0 }, { opacity: 1 }],
      { duration: 1000, timingFunction: 'ease-in-out' },
    ]);

    // Test that it accepts pause operation overload
    expectTypeOf<typeof __ElementAnimate>().toBeCallableWith(element, [2 as AnimationOperation.PAUSE, 'test-animation']);

    // Test that it accepts play operation overload
    expectTypeOf<typeof __ElementAnimate>().toBeCallableWith(element, [1 as AnimationOperation.PLAY, 'test-animation']);

    // Test that it accepts cancel operation overload
    expectTypeOf<typeof __ElementAnimate>().toBeCallableWith(element, [3 as AnimationOperation.CANCEL, 'test-animation']);
  });

  it('should test element template api signatures', () => {
    const child = {} as ElementRef;
    expectTypeOf<typeof __CreateElementTemplate>().toBeCallableWith('todo_card', 'path/to/bundle.js', ['width: 320px;', { completed: false }], [[child]], 'template-uid', {
      cachedItems: [child],
      enabled: true,
    });
    const template = {} as ElementRef;

    expectTypeOf<typeof __CreateElementTemplate>().returns.toEqualTypeOf<ElementRef>();
    expectTypeOf<typeof __SetAttributeOfElementTemplate>().toBeCallableWith(template, 0, { completed: true });
    expectTypeOf<typeof __InsertNodeToElementTemplate>().toBeCallableWith(template, 1, child, null);
    expectTypeOf<typeof __RemoveNodeFromElementTemplate>().toBeCallableWith(template, 1, child);
    expectTypeOf<typeof __SerializeElementTemplate>().returns.toEqualTypeOf<SerializedTemplateInstance>();

    const serialized = {} as SerializedTemplateInstance;
    assertType<SerializedTemplateInstance>(serialized);
    assertType<SerializedTemplateInstance[][] | null | undefined>(serialized.elementSlots);
    assertType<Record<string, any> | null | undefined>(serialized.options);
    assertType<number | string>(serialized.uid);

    expectTypeOf<typeof __CreateTypedElementTemplate>().toBeCallableWith('list', { 'enable-layout': true }, [[child]], 1001, { recycled: [child] });
    const typed = {} as ElementRef;
    expectTypeOf<typeof __CreateTypedElementTemplate>().returns.toEqualTypeOf<ElementRef>();
    expectTypeOf<typeof __CreateTypedElementTemplate>().toBeCallableWith('raw-text', null, null, 'typed-uid');
    expectTypeOf<typeof __SerializeElementTemplate>().toBeCallableWith(typed);

    const serializedTyped = {} as SerializedTemplateInstance;
    assertType<SerializedTemplateInstance>(serializedTyped);
    expectTypeOf<SerializedTypedTemplateInstance['attributes']>().toEqualTypeOf<Record<string, SerializableValue> | null | undefined>();
    assertType<SerializedTemplateInstance[][] | null | undefined>(serializedTyped.elementSlots);
    assertType<Record<string, any> | null | undefined>(serializedTyped.options);
    assertType<number | string>(serializedTyped.uid);
  });
});
