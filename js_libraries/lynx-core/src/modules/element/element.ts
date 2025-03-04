import { Animation, AnimationOperation, KeyframeEffect } from '../animation';
import { Lynx } from '../../lynx';

/**
 * Native Element, Held by {@link Element} and interacting with native.
 */
export interface NativeElementProxy {
  animate(
    operation: AnimationOperation,
    id: string,
    keyframes?: Record<string, any>[],
    timingOptions?: Record<string, any>
  ): void;
  setProperty(propsName: string, propsValue: string): void;
  setProperty(props: Record<string, string>): void;
}

export default class Element {
  private readonly _root: string;
  private readonly _idSelector: string;
  private readonly _lynx: Lynx;
  private _element: NativeElementProxy;

  constructor(root: string, id: string, lynxProxy: Lynx) {
    this._root = root;
    this._idSelector = '#' + id;
    this._lynx = lynxProxy;
    this._element = undefined;
  }

  private ensureElement() {
    if (!this._element) {
      this._element = this._lynx.createElement(this._root, this._idSelector);
    }
  }

  // keyframes: see https://developer.mozilla.org/en-US/docs/Web/API/Web_Animations_API/Keyframe_Formats
  //  Either an array of keyframe objects, or a keyframe object whose property are arrays of values to iterate over. See Keyframe Formats for more details.
  //
  // timingOptions: see https://developer.mozilla.org/en-US/docs/Web/API/Element/animate
  //  id Optional: A property unique to animate(): a DOMString with which to reference the animation.
  //  delay Optional: The number of milliseconds to delay the start of the animation. Defaults to 0.
  //  direction Optional: Whether the animation runs forwards (normal), backwards (reverse), switches direction after each iteration (alternate), or runs backwards and switches direction after each iteration (alternate-reverse). Defaults to "normal".
  //  duration Optional: The number of milliseconds each iteration of the animation takes to complete. Defaults to 0. Although this is technically optional, keep in mind that your animation will not run if this value is 0.
  //  easing Optional: The rate of the animation's change over time. Accepts the pre-defined values "linear", "ease", "ease-in", "ease-out", and "ease-in-out", or a custom "cubic-bezier" value like "cubic-bezier(0.42, 0, 0.58, 1)". Defaults to "linear".
  //  endDelay Optional: The number of milliseconds to delay after the end of an animation. This is primarily of use when sequencing animations based on the end time of another animation. Defaults to 0.
  //  fill Optional: Dictates whether the animation's effects should be reflected by the element(s) prior to playing ("backwards"), retained after the animation has completed playing ("forwards"), or both. Defaults to "none".
  //  iterationStart Optional: Describes at what point in the iteration the animation should start. 0.5 would indicate starting halfway through the first iteration for example, and with this value set, an animation with 2 iterations would end halfway through a third iteration. Defaults to 0.0.
  // iterations Optional: The number of times the animation should repeat. Defaults to 1, and can also take a value of Infinity to make it repeat for as long as the element exists.
  animate(
    keyframes: Array<Record<string, any>>,
    timingOptions: Record<string, any>
  ): Animation {
    this.ensureElement();
    let ani = new Animation(new KeyframeEffect(this, keyframes, timingOptions));
    this._element.animate(0, ani.id, keyframes, timingOptions);
    return ani;
  }

  playAnimate(ani: Animation): void {
    this._element.animate(1, ani.id, undefined, undefined);
  }

  pauseAnimate(ani: Animation): void {
    this._element.animate(2, ani.id, undefined, undefined);
  }

  cancelAnimate(ani: Animation): void {
    this._element.animate(3, ani.id, undefined, undefined);
  }

  finishAnimate(ani: Animation): void {
    this._element.animate(4, ani.id, undefined, undefined);
  }

  setProperty(
    propsObj: string | Record<string, string>,
    propsVal?: string
  ): void {
    this.ensureElement();
    if (typeof propsObj === 'string' && typeof propsVal === 'string') {
      this._element.setProperty({
        [propsObj]: propsVal,
      });
    } else if (typeof propsObj === 'object') {
      this._element.setProperty(propsObj);
    } else {
      throw new Error(
        `setProperty's param must be string or object. While current type is ${typeof propsObj} and ${typeof propsVal}.`
      );
    }
  }
}
