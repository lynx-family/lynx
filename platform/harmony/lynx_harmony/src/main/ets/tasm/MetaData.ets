// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

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

  /**
   * Input data in string, key-value or json format and return the parsed `TemplateData` object.
   * @apidoc
   * @param input String, key-value or json type data.
   * @param processor Data processor.
   */
  constructor(input?: string | Record<string, Object> | Object, processor: string = '') {
    this._data = input;
    this.processor = processor;
  }

  /**
   * Get the data in `TemplateData` object.
   * @apidoc
   */
  get data() {
    return this._data;
  }

  /**
   * Merge the incoming data with the current data.
   * @apidoc
   * @param data Incoming data in string, key-value or json format.
   */
  merge(data?: string | Record<string, Object> | Object) {
    if (data === undefined) {
      return;
    }
    if (this._data === undefined) {
      this._data = data;
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
    }
  }
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

  constructor(templateData?: TemplateData, globalProps?: TemplateData, updateMode: LynxUpdateMode = LynxUpdateMode.UPDATE) {
    super(templateData, globalProps);
    this.updateMode = updateMode;
  }

  merge(data: LynxUpdateMeta) {
    super.merge(data);
    this.updateMode = data.updateMode;
  }
}
