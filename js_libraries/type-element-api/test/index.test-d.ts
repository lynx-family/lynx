import { assertType, describe, expectTypeOf, it } from 'vitest';
import type {
  AnimationOperation,
  AnimationTimingOptions,
  Keyframe,
  ElementRef,
  ElementTemplateHandle,
  ComponentElementRef,
  PageElementRef,
  ListElementRef,
  ViewElementRef,
  ComposeElementKind,
  ComposeElementRef,
  TextElementMeasurement,
  SerializedCompiledTemplateInstance,
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
  StyleSheetRef,
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
  it('keeps stylesheet handles separate from element handles', () => {
    const sheet = {} as StyleSheetRef;
    expectTypeOf<typeof __LoadStyleSheet>().returns.toEqualTypeOf<StyleSheetRef | undefined>();
    expectTypeOf<typeof __AdoptStyleSheet>().toBeCallableWith(sheet);
    expectTypeOf<typeof __RemoveStyleSheet>().toBeCallableWith(sheet);
    expectTypeOf<typeof __RemoveStyleSheet>().returns.toBeBoolean();
    expectTypeOf<typeof __ReplaceStyleSheet>().toBeCallableWith(sheet, sheet);
    expectTypeOf<typeof __ReplaceStyleSheet>().returns.toBeBoolean();
    expectTypeOf<ElementRef>().not.toMatchTypeOf<StyleSheetRef>();
    expectTypeOf<typeof __RemoveStyleSheet>().parameters.toEqualTypeOf<[StyleSheetRef]>();
    expectTypeOf<typeof __ReplaceStyleSheet>().parameters.toEqualTypeOf<[StyleSheetRef, StyleSheetRef]>();
  });

  it('should have correct ElementRef types', () => {
    expectTypeOf<ElementRef>().toBeObject();
    expectTypeOf<ComponentElementRef>().toEqualTypeOf<ElementRef>();
    expectTypeOf<PageElementRef>().toEqualTypeOf<ComponentElementRef>();
    expectTypeOf<ListElementRef>().toEqualTypeOf<ElementRef>();
    expectTypeOf<ViewElementRef>().toEqualTypeOf<ElementRef>();
    expectTypeOf<ElementTemplateHandle>().not.toMatchTypeOf<ElementRef>();
    expectTypeOf<ElementRef>().not.toMatchTypeOf<ElementTemplateHandle>();
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

  it('should type detached text measurements without accepting ordinary Elements', () => {
    const composeNode = {} as ComposeElementRef;
    expectTypeOf<typeof __MeasureTextElement>().toBeCallableWith(composeNode, 100, 2, 0, 0);
    expectTypeOf<typeof __MeasureTextElement>().parameter(0).toEqualTypeOf<ComposeElementRef>();
    expectTypeOf<typeof __MeasureTextElement>().parameter(2).toEqualTypeOf<0 | 1 | 2>();
    expectTypeOf<typeof __MeasureTextElement>().parameter(4).toEqualTypeOf<0 | 1 | 2>();
    expectTypeOf<typeof __MeasureTextElement>().returns.toEqualTypeOf<TextElementMeasurement | undefined>();
    expectTypeOf<TextElementMeasurement>().toEqualTypeOf<{
      width: number;
      height: number;
      baseline: number;
    }>();
    // @ts-expect-error ordinary Element references do not carry a Compose handle brand.
    assertType<Parameters<typeof __MeasureTextElement>[0]>({} as ElementRef);
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
    const child = {} as ElementTemplateHandle;
    expectTypeOf<typeof __MarkTemplateElement>().parameter(0).toEqualTypeOf<ElementRef>();
    expectTypeOf<typeof __IsTemplateElement>().parameter(0).toEqualTypeOf<ElementRef>();
    expectTypeOf<typeof __CreateElementTemplate>().toBeCallableWith('todo_card', 'path/to/bundle.js', ['width: 320px;', { completed: false }], [[child]], 101, {
      cachedItems: [child],
      enabled: true,
    });
    const sparseChildSlots: Parameters<typeof __CreateElementTemplate>[3] = [[child], null, undefined];
    expectTypeOf<typeof __CreateElementTemplate>().toBeCallableWith('todo_card', null, null, sparseChildSlots, 102);
    const template = {} as ElementTemplateHandle;

    expectTypeOf<typeof __CreateElementTemplate>().returns.toEqualTypeOf<ElementTemplateHandle>();
    expectTypeOf<typeof __GetElementByAttrSlot>().toBeCallableWith(template, 0);
    expectTypeOf<typeof __GetElementByAttrSlot>().returns.toEqualTypeOf<ElementRef | null>();
    expectTypeOf<typeof __SetAttributeOfElementTemplate>().toBeCallableWith(template, 0, { completed: true });
    expectTypeOf<typeof __InsertNodeToElementTemplate>().toBeCallableWith(template, 1, child, null);
    expectTypeOf<typeof __RemoveNodeFromElementTemplate>().toBeCallableWith(template, 1, child);
    expectTypeOf<typeof __SerializeElementTemplate>().returns.toEqualTypeOf<SerializedTemplateInstance>();

    expectTypeOf<typeof __InsertNodeToElementTemplate>().parameter(2).toEqualTypeOf<ElementTemplateHandle>();
    expectTypeOf<ElementRef[][]>().not.toMatchTypeOf<Parameters<typeof __CreateElementTemplate>[3]>();
    expectTypeOf<ElementRef[][]>().not.toMatchTypeOf<Parameters<typeof __CreateTypedElementTemplate>[2]>();
    expectTypeOf<ElementTemplateHandle>().not.toMatchTypeOf<Parameters<typeof __GetElementUniqueID>[0]>();
    expectTypeOf<ElementTemplateHandle>().not.toMatchTypeOf<Parameters<typeof __AppendElement>[1]>();
    expectTypeOf<typeof __CreateElementTemplate>().parameter(4).toEqualTypeOf<number>();
    expectTypeOf<typeof __CreateTypedElementTemplate>().parameter(3).toEqualTypeOf<number>();

    expectTypeOf<SerializedCompiledTemplateInstance['childSlots']>().toEqualTypeOf<(SerializedTemplateInstance[] | null | undefined)[] | null | undefined>();
    expectTypeOf<SerializedCompiledTemplateInstance['options']>().toEqualTypeOf<Record<string, any> | null | undefined>();
    expectTypeOf<SerializedCompiledTemplateInstance['uid']>().toEqualTypeOf<number>();

    const typed = {} as ElementTemplateHandle;
    expectTypeOf<typeof __CreateTypedElementTemplate>().toBeCallableWith('raw-text', null, null, 104);
    expectTypeOf<typeof __SerializeElementTemplate>().toBeCallableWith(typed);
    expectTypeOf<typeof __CreateTypedElementTemplate>().parameter(0).toEqualTypeOf<string>();
    expectTypeOf<typeof __CreateTypedElementTemplate>().returns.toEqualTypeOf<ElementTemplateHandle>();

    expectTypeOf<SerializedTypedTemplateInstance['attributes']>().toEqualTypeOf<Record<string, SerializableValue> | null | undefined>();
    expectTypeOf<SerializedTypedTemplateInstance['childSlots']>().toEqualTypeOf<(SerializedTemplateInstance[] | null | undefined)[] | null | undefined>();
    expectTypeOf<SerializedTypedTemplateInstance['options']>().toEqualTypeOf<Record<string, any> | null | undefined>();
    expectTypeOf<SerializedTypedTemplateInstance['uid']>().toEqualTypeOf<number>();
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
