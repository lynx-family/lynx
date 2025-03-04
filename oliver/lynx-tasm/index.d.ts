// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/* eslint-disable */
export function encode(code: any): Promise<EncodeResult>;
export function loadModule(): any;
export function lepusCheck(code: string): any;
export interface EncodeResult {
  status: number;
  buffer: Buffer;
  lepus_code: string;
  lepus_debug: string;
  error_msg: string;
  section_size: string;
}
export function supportNapi(binary?: string): boolean;
export function getEncodeMode(binary?: string): (options: any) => Promise<EncodeResult>;
export function encrypt(token: string): string;
export function decrypt(token: string): string;
export function encrypt_wasm(token: string): string;
export function decrypt_wasm(token: string): string;