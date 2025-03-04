// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { CSSProperties } from '../csstype';

/**
 * IntrinsicAttributes specifies extra properties used by the JSX framework
 * which are not generally used by the components’ props or arguments.
 * @see https://www.typescriptlang.org/docs/handbook/jsx.html#attribute-type-checking
 *
 * But in Lynx, we use this interface to specify the extra attributes
 * just for Lynx components.
 * @see Lynx/tasm/component_attributes.cc
 */
export interface IntrinsicAttributes {
  /**
   * A type helper for Lynx, DO NOT SET THIS.
   */
  __empty?: false;

  /**
   * The name of the component
   */
  name?: string;

  /**
   * The style set to the view of component.
   *
   * Has no effect when `removeComponentElement` enabled.
   */
  style?: string | CSSProperties;

  // /**
  //  * The CSS classes set to the view of component.
  //  *
  //  * Has no effect when `removeComponentElement` enabled.
  //  */
  // className?: string;
  class?: string;

  /**
   * Set to `true` to enable overlapping rendering
   */
  overlap?: boolean;
}

type LynxIntrinsicAttributes = IntrinsicAttributes;

declare global {
  namespace JSX {
    interface IntrinsicAttributes extends LynxIntrinsicAttributes {
      /**
       * A type helper for Lynx, DO NOT SET THIS.
       */
      __empty?: never;
    }
  }
}
