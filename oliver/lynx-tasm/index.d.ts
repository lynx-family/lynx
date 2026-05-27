// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/* eslint-disable */
export function encode(code: any): Promise<EncodeResult>;
export function lepusCheck(
  code: string,
  targetSdkVersion?: string,
  execute?: number
): Promise<any>;
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
export function encrypt_wasm(token: string): Promise<string>;
export function decrypt_wasm(token: string): Promise<string>;
export function decode_napi(template: Uint8Array): any;
export function decode_wasm(template: Uint8Array): Promise<any>;

export type TemplateDecodeBackend = "auto" | "napi" | "wasm";

export type TemplateDecodeDetailResult =
  | TemplateDecodeDetailSuccess
  | TemplateDecodeDetailError;

export type TemplateDecodeStatus = "ok" | "partial" | `error:${string}`;

export interface TemplateDecodeDetailSuccess {
  schemaVersion: 1;
  status: "ok" | "partial";
  backend: "napi" | "wasm";
  template: TemplateInfo;
  size: TemplateSizeInfo;
  content: TemplateContentInfo;
}

export interface TemplateDecodeDetailError {
  schemaVersion: 1;
  status: `error:${string}`;
  backend?: "napi" | "wasm";
}

export interface TemplateItemCollection<T> {
  totalCount: number;
  items: T[];
}

export interface TemplateSizeCollection<T> extends TemplateItemCollection<T> {
  totalBytes: number;
}

export interface TemplateContentCollection<T> extends TemplateItemCollection<T> {
  totalContentBytes: number;
}

export interface TemplateRawContentRef {
  id: string;
  contentBytes: number;
  encoding: "utf8" | "base64" | "json";
  label?: string;
}

export interface TemplateInfo {
  appType: string;
  arch: "radon" | "fiber" | "air" | "unknown";
  templateMode: "flexible" | "legacy";
  lepusType: "lepus" | "lepus-ng" | "unknown";
  engineVersion?: string;
  targetSdkVersion?: string;
  compileOptions: Record<string, unknown>;
  templateInfo: Record<string, unknown>;
  trialOptions: Record<string, unknown>;
}

export interface TemplateSizeInfo {
  totalBytes: number;
  sections: TemplateSizeCollection<TemplateBinarySection>;
  items: TemplateSizeCollection<TemplateContentSize>;
}

export interface TemplateBinarySection {
  name: string;
  bytes: number;
  range: ByteRange;
  known: boolean;
  order: number;
}

export interface TemplateContentSize {
  name: string;
  bytes: number;
  kind:
    | "script"
    | "lepus"
    | "css"
    | "string"
    | "custom-section"
    | "section"
    | "other";
  range?: ByteRange;
  sourceSection?: string;
  rawContentRef?: TemplateRawContentRef;
  details?: Record<string, unknown>;
}

export interface TemplateContentInfo {
  components: TemplateComponentsInfo;
  css: TemplateCssInfo;
  scripts: TemplateScriptInfo;
  lepus: TemplateLepusInfo;
  strings: TemplateStringInfo;
  customSections: TemplateContentCollection<TemplateCustomSectionInfo>;
  rawContents: TemplateContentCollection<TemplateRawContent>;
}

export interface TemplateComponentsInfo {
  source: "legacy-section" | "fiber-metadata" | "mixed" | "unavailable";
  staticComponents: TemplateItemCollection<TemplateComponentInfo>;
  dynamicComponents: TemplateItemCollection<TemplateComponentInfo>;
  dynamicDeclarations: TemplateItemCollection<TemplateDynamicComponentDeclaration>;
}

export interface TemplateComponentInfo {
  id?: number;
  name?: string;
  path?: string;
  cssId?: number;
  dependentComponentIds?: number[];
  properties?: Record<string, unknown>;
  externalClasses?: string[];
  config?: Record<string, unknown>;
}

export interface TemplateDynamicComponentDeclaration {
  name: string;
  path?: string;
}

export interface TemplateCssInfo {
  fragments: TemplateContentCollection<TemplateCssFragment>;
  selectors: TemplateContentCollection<TemplateStringRef>;
  urls: TemplateContentCollection<TemplateStringRef>;
  identifiers: TemplateContentCollection<TemplateStringRef>;
}

export interface TemplateCssFragment {
  id?: number;
  source?: string;
  range?: ByteRange;
  contentBytes?: number;
}

export interface TemplateStringInfo {
  count: number;
  totalContentBytes: number;
  emptyCount: number;
  entries: TemplateContentCollection<TemplateStringItem>;
  classified: {
    url: TemplateContentCollection<TemplateStringRef>;
    cssSelector: TemplateContentCollection<TemplateStringRef>;
    identifier: TemplateContentCollection<TemplateStringRef>;
    other: TemplateContentCollection<TemplateStringRef>;
  };
}

export interface TemplateStringItem {
  value?: string;
  contentBytes: number;
  index: number;
  source?: "css" | "js" | "string-table" | "heuristic";
  role?: "path" | "js-source" | "css-selector" | "identifier" | "url" | "other";
  rawContentRef?: TemplateRawContentRef;
}

export interface TemplateStringRef {
  index: number;
  contentBytes: number;
  role?: TemplateStringItem["role"];
  rawContentRef?: TemplateRawContentRef;
}

export interface TemplateScriptInfo {
  files: TemplateContentCollection<TemplateScriptFile>;
}

export interface TemplateScriptFile {
  path: string;
  pathStringIndex?: number;
  kind: "source" | "bytecode";
  contentBytes: number;
  rawContentRef: TemplateRawContentRef;
}

export interface TemplateLepusInfo {
  root?: {
    contentBytes: number;
    range?: ByteRange;
    rawContentRef?: TemplateRawContentRef;
  };
  chunks: TemplateContentCollection<TemplateLepusChunk>;
}

export interface TemplateLepusChunk {
  path: string;
  contentBytes: number;
  range?: ByteRange;
  rawContentRef?: TemplateRawContentRef;
}

export interface TemplateCustomSectionInfo {
  key: string;
  encoding?: string;
  contentBytes?: number;
  range?: ByteRange;
  rawContentRef?: TemplateRawContentRef;
}

export type TemplateRawContentData =
  | TemplateTextContent
  | TemplateBinaryContent
  | TemplateJsonContent;

export interface TemplateTextContent {
  contentBytes: number;
  encoding: "utf8";
  data: string;
}

export interface TemplateBinaryContent {
  contentBytes: number;
  encoding: "base64";
  data: string;
}

export interface TemplateJsonContent {
  contentBytes: number;
  encoding: "json";
  data: unknown;
}

export interface TemplateRawContent {
  id: string;
  role:
    | "string"
    | "js-source"
    | "js-bytecode"
    | "lepus-bytecode"
    | "custom-section"
    | "css"
    | "unknown";
  label?: string;
  contentBytes: number;
  range?: ByteRange;
  source?: {
    stringIndex?: number;
    section?: string;
    routeIndex?: number;
  };
  content: TemplateRawContentData;
}

export interface ByteRange {
  start: number;
  end: number;
  size: number;
}

export function decode_detail(
  template: Buffer | Uint8Array,
  backend?: TemplateDecodeBackend
): Promise<TemplateDecodeDetailResult>;
export function decode_detail_napi(
  template: Buffer | Uint8Array
): TemplateDecodeDetailResult;
export function decode_detail_wasm(
  template: Buffer | Uint8Array
): Promise<TemplateDecodeDetailResult>;
