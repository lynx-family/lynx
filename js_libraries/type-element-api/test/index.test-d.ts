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
  ComposeElementKind,
  ComposeElementRef,
  SerializedTemplateInstance,
  SerializableValue,
  SerializedTypedTemplateInstance,
  ElementEvent,
  ElementEventBindType,
  ElementEventCallback,
  ElementEventClosureType,
  ElementEventOptions,
  ElementEventRef,
  ElementEventType,
  ElementEventListenerOptions,
} from '../types/internal';

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
    const composeNode = {} as ComposeElementRef;
    const pageNode = {} as PageElementRef;
    const rawNode = {} as ElementRef;
    expectTypeOf<typeof __CreateCompose>().toBeCallableWith(1, 1 as ComposeElementKind);
    expectTypeOf<typeof __CreateCompose>().returns.toEqualTypeOf<ComposeElementRef>();
    expectTypeOf<ComposeElementRef>().not.toEqualTypeOf<ElementRef>();
    expectTypeOf<typeof __SetComposeModifier>().toBeCallableWith(composeNode, null);
    expectTypeOf<typeof __SetComposeModifier>().returns.toBeVoid();
    expectTypeOf<typeof __InsertElementAt>().toBeCallableWith(pageNode, composeNode, 0);
    expectTypeOf<typeof __InsertElementAt>().toBeCallableWith(composeNode, composeNode, 0);
    expectTypeOf<typeof __InsertElementAt>().toBeCallableWith(composeNode, rawNode, 0);
    expectTypeOf<typeof __SetAttribute>().toBeCallableWith(composeNode, 'text', 'hello');
    expectTypeOf<typeof __SetAttribute>().toBeCallableWith(composeNode, 'accessibility-label', null);
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

  it('should test event api signatures', () => {
    const element = {} as ElementRef;
    const callback = ((event: ElementEvent) => {
      assertType<ElementEventRef | undefined>(event.ref);
    }) as ElementEventCallback;
    const listenerOptions: ElementEventListenerOptions = {
      capture: true,
      once: true,
      passive: true,
      signal: false,
      closure_type: 0 as ElementEventClosureType.NONE,
      bind_type: 2 as ElementEventBindType.CAPTURE,
    };
    const eventOptions: ElementEventOptions = {
      capture: false,
      bubbles: true,
      cancelable: true,
      composed: true,
    };

    expectTypeOf<typeof __AddEventListener>().toBeCallableWith(element, 'tap', callback, listenerOptions);
    expectTypeOf<typeof __AddEventListener>().toBeCallableWith(element, 'tap', 'onTap', {
      closure_type: 3 as ElementEventClosureType.CLIENT,
      bind_type: 1 as ElementEventBindType.BUBBLE,
    });
    expectTypeOf<typeof __RemoveEventListener>().toBeCallableWith(element, 'tap', callback, listenerOptions);
    expectTypeOf<typeof __RemoveEventListeners>().toBeCallableWith(element);
    expectTypeOf<typeof __CreateEvent>().toBeCallableWith(8 as ElementEventType.CUSTOM, 'ready', eventOptions, { value: 1 });
    expectTypeOf<typeof __CreateEvent>().returns.toEqualTypeOf<ElementEventRef>();

    const event = {} as ElementEventRef;
    expectTypeOf<typeof __DispatchEvent>().toBeCallableWith(element, event);
    expectTypeOf<typeof __DispatchEvent>().returns.toBeBoolean();
    expectTypeOf<typeof __StopPropagation>().toBeCallableWith(event);
    expectTypeOf<typeof __StopImmediatePropagation>().toBeCallableWith(event);
  });
});
