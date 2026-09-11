// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Types shared by every Element API stability tier.
 *
 * This file intentionally contains no global API declarations. Import a
 * stability tier instead: `stable`, `experimental`, or `internal`.
 */

export interface ElementRef extends Record<string, unknown> {}

declare const composeElementRefBrand: unique symbol;

export interface ComposeElementRef {
  readonly [composeElementRefBrand]: never;
}

export interface ComponentElementRef extends ElementRef {}

export interface PageElementRef extends ComponentElementRef {}

export interface ListElementRef extends ElementRef {}

export interface ViewElementRef extends ElementRef {}

export interface TextElementRef extends ElementRef {}

export interface RawTextElementRef extends ElementRef {}

export interface ImageElementRef extends ElementRef {}

export interface ScrollElementRef extends ElementRef {}

export interface WrapperElementRef extends ElementRef {}

export interface NoneElementRef extends ElementRef {}

export interface IfElementRef extends ElementRef {}

export interface ForElementRef extends ElementRef {}

export interface BlockElementRef extends ElementRef {}

export type ElementInfo = Record<string, any>;
