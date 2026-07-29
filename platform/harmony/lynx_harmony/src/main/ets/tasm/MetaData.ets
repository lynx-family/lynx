// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { LLog } from '../base/LLog';

const TAG = 'TemplateData';

/**
 * The mode used by `MetaData` update.
 * @apidoc
 */
export enum LynxUpdateMode {
  /**
   * UPDATE MODE identifies an incremental action, data will be incrementally modified with previous data.
   */
  UPDATE = 0,

  /**
   * RESET MODE identifies an reset action, data will be cleared before and the update with the in-coming data.
   */
  RESET = 1,

  /**
   * RELOAD MODE identifies an reload action, it mostly behaves the same as `RESET` and will trigger the destroy & recreate of js lifecycle.
   */
  RELOAD = 2,
}

/**
 * TemplateData is a data structure provided by Lynx.
 * It is used to store the data types accepted by Lynx at runtime
 * and can be constructed using either a string or a map type.
 * @apidoc
 */
export class TemplateData {
  processor: string = '';
  private _data: string | Record<string, Object> | Object | undefined;
  readOnly: boolean = false;
  private _nativeData: lynx.NativeTemplateData | undefined;

  /**
   * Creates a `TemplateData` for static-page loading without converting the supplied platform data
   * to Lepus. The caller must not mutate it after this call.
   * @apidoc
   * @param input Platform data used by the static page.
   * @return A `TemplateData` that retains the supplied platform data.
   */
  static createForStaticPage(input: Record<string, Object>): TemplateData {
    return new StaticPageTemplateData(input);
  }

  /**
   * Input data in string, key-value or json format and return the parsed `TemplateData` object.
   * @apidoc
   * @param input String, key-value or json type data.
   * @param processor Data processor.
   * @param enableNativeData Whether to synchronously convert the input to a reusable native
   *   snapshot. When `true`, the conversion happens once at construction time and the snapshot is
   *   what `getDataForNative()` hands to the engine. The caller must therefore treat `input` (and
   *   the object returned by `data`) as immutable afterwards: mutating them in place will NOT be
   *   reflected in the native snapshot. Use `merge()` to update the data, which rebuilds the
   *   snapshot; construct a new `TemplateData` if you need different data. When `false` (default),
   *   no snapshot is kept and `getDataForNative()` converts `data` on demand.
   */
  constructor(input?: string | Record<string, Object> | Object, processor: string = '',
    enableNativeData: boolean = false) {
    this._data = input;
    this.processor = processor;
    this._nativeData = enableNativeData ? new lynx.NativeTemplateData(input ?? '') : undefined;
  }

  /**
   * Get the data in `TemplateData` object.
   *
   * @apidoc
   * @note This getter returns the JS-side data reference which is not thread-safe. When native
   * data is enabled, the returned value may diverge from the native snapshot after in-place
   * mutation.
   * @deprecated This accessor will be removed in a future version. CRUD interfaces for
   * querying and mutating template data will be provided as replacements.
   */
  get data() {
    return this._data;
  }

  /**
   * Returns the value handed to the native engine. When a native snapshot was enabled at
   * construction time (`enableNativeData=true`), this returns the cached snapshot built at
   * construction or last refreshed by `merge()`; in-place mutations of `data` since then are not
   * reflected. Otherwise it falls back to `data`, which is converted on demand by native.
   */
  getDataForNative(): Object | string | undefined {
    return this._nativeData ?? this.data;
  }

  /**
   * Merge the incoming data with the current data. When a native snapshot is enabled, the snapshot
   * is rebuilt from the merged result so `getDataForNative()` stays coherent with the data.
   * @apidoc
   * @param data Incoming data in string, key-value or json format.
   */
  merge(data?: string | Record<string, Object> | Object) {
    if (data === undefined) {
      return;
    }
    if (this._data === undefined) {
      this._data = data;
      this.refreshNativeData();
      return;
    }
    try {
      if (typeof this._data === 'string') {
        this._data = JSON.parse(this._data);
      }
      if (typeof data === 'string') {
        data = JSON.parse(data);
      }
      for (let e of Object.entries(data as Record<string, Object>)) {
        (this._data as Record<string, Object>)[e[0]] = e[1];
      }
    } catch (e) {
      LLog.d(TAG, 'TemplateData merge error: ' + e);
    } finally {
      this.refreshNativeData();
    }
  }

  private refreshNativeData() {
    if (this._nativeData !== undefined) {
      this._nativeData = new lynx.NativeTemplateData(this._data);
    }
  }
}

class StaticPageTemplateData extends TemplateData {
  private readonly platformData: Record<string, Object>;

  constructor(input: Record<string, Object>) {
    super();
    this.platformData = input;
    this.readOnly = true;
  }

  override get data(): Record<string, Object> {
    return this.platformData;
  }

  override merge(data?: string | Record<string, Object> | Object): void {
    LLog.e(TAG, 'merge is not supported for static-page data');
  }
}

export function isForStaticPage(data?: TemplateData): boolean {
  return data instanceof StaticPageTemplateData;
}

/**
 * LynxUpdateMeta is the metadata structure used by LynxView to update template data.
 * It will serve as the input parameter for UpdateData. Users can specify various parameters
 * required for the update through LynxUpdateMeta, such as template data and globalProps, among others.
 * @apidoc
 */
export class MetaData {
  templateData?: TemplateData;
  globalProps?: TemplateData;

  /**
   * MetaData construction.
   * @param templateData The content of the data to update the template.
   * @param globalProps The globalProps content for updating the template.
   */
  constructor(templateData?: TemplateData, globalProps?: TemplateData) {
    this.templateData = templateData;
    this.globalProps = globalProps;
  }

  merge(data: MetaData) {
    if (this.globalProps) {
      this.globalProps.merge(data.globalProps?.data);
    } else {
      this.globalProps = data.globalProps;
    }

    if (!data.templateData) {
      return;
    }

    if (this.templateData) {
      this.templateData.processor = data.templateData.processor;
      this.templateData.readOnly = data.templateData.readOnly;
      this.templateData.merge(data.templateData.data);
    } else {
      this.templateData = data.templateData;
    }
  }
}

/**
 * LynxUpdateMeta is the metadata structure used by LynxView to update template data.
 * It extends MetaData with update mode, and should only be used by update APIs.
 * @apidoc
 */
export class LynxUpdateMeta extends MetaData {
  updateMode: LynxUpdateMode = LynxUpdateMode.UPDATE;

  constructor(templateData?: TemplateData, globalProps?: TemplateData,
    updateMode: LynxUpdateMode = LynxUpdateMode.UPDATE) {
    super(templateData, globalProps);
    this.updateMode = updateMode;
  }

  merge(data: LynxUpdateMeta) {
    super.merge(data);
    this.updateMode = data.updateMode;
  }
}
