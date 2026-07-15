// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type * as CSS from 'csstype';
import type { LegacyCSSCompatibilityV4_2 } from './csstype-legacy-compat';
import type {
  Longhands,
  LynxCSSProperties,
  Shorthands,
} from './csstype.generated';

export type {
  Longhands,
  LynxCSSProperties,
  Shorthands,
} from './csstype.generated';

export type Modify<T, R> = Omit<T, keyof R> & R;

type StandardCSSProperties = CSS.Properties<string | number>;

type StylePropertyName =
  | keyof StandardCSSProperties
  | keyof LynxCSSProperties
  | keyof LegacyCSSCompatibilityV4_2;

type ValueAt<T, K extends PropertyKey> = K extends keyof T ? T[K] : never;

/**
 * The default inline style type preserves standard, generated, and previously
 * published Lynx values. Use LynxCSSProperties for metadata-only checking.
 */
export type CSSProperties = {
  [K in StylePropertyName]?:
    | ValueAt<StandardCSSProperties, K>
    | ValueAt<LynxCSSProperties, K>
    | ValueAt<LegacyCSSCompatibilityV4_2, K>;
};

/** The explicit name for the backward-compatible inline-style contract. */
export type CompatibleCSSProperties = CSSProperties;

/** The metadata-derived contract for opt-in strict inline-style checking. */
export type StrictCSSProperties = LynxCSSProperties;

/** A type-only contract for checking every value in a named style map. */
export type StrictStyleSheet = Readonly<
  Record<string, StrictCSSProperties>
>;

/** Declaration-merging hook used by the optional strict package entry. */
export interface InlineStyleTypeConfig {}

/** The style-object contract consumed by Lynx element props. */
export type InlineStyleProperties =
  InlineStyleTypeConfig extends { strict: true }
    ? StrictCSSProperties
    : CompatibleCSSProperties;

export type CSSPropertiesWithShorthands = Pick<
  CSSProperties,
  Shorthands & keyof CSSProperties
>;
export type CSSPropertiesWithLonghands = Pick<
  CSSProperties,
  Longhands & keyof CSSProperties
>;
