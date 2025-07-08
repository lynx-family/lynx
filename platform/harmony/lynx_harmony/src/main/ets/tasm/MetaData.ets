// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LLog } from '../base/LLog';

const TAG = 'TemplateData';

export class TemplateData {
  processor: string = '';
  private _data: string | Record<string, Object> | Object | undefined;
  readOnly: boolean = false;

  constructor(input?: string | Record<string, Object> | Object, processor: string = '') {
    this._data = input;
    this.processor = processor;
  }

  get data() {
    return this._data;
  }

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

export class MetaData {
  templateData?: TemplateData;
  globalProps?: TemplateData;

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
