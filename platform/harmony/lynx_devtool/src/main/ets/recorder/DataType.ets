// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface ActionData {
  'Function Name': string,
  RecordMillisecond: number,
  Params: object
}

export interface SetThreadStrategyParams {
  threadStrategy: number,
  enableJSRuntime: boolean
}

export interface UpdateViewPortParams {
  layoutHeightMode: number,
  layoutWidthMode: number,
  preferredLayoutHeight: number,
  preferredLayoutWidth: number,
  preferredMaxLayoutHeight: number,
  preferredMaxLayoutWidth: number
}

export interface SetGlobalPropsParams {
  global_props: object
}

export interface RecordedTemplateDataParams {
  value?: object | string | null,
  preprocessorName?: string | null,
  readOnly?: boolean
}

export interface LoadTemplateParams {
  url?: string | null,
  source: string,
  isCSR: boolean,
  templateData: object
}

export interface LoadComponentWithCallbackParams {
  url: string,
  source: string,
  sync_tag?: boolean,
  callback_id?: number
}

export interface UpdateMetaDataParams {
  templateData?: RecordedTemplateDataParams | null,
  global_props?: object | null
}

export interface UpdatePageOptionParams {
  reset_page_data?: boolean
}

export interface UpdateDataByPreParsedDataParams {
  value?: object | string | null,
  preprocessorName?: string | null,
  readOnly?: boolean,
  updatePageOption?: UpdatePageOptionParams
}

export interface SendGlobalEventParams {
  module_id: string,
  method_id: string,
  arguments: Object[]
}

export interface JSBInvokerParams {
  argc: number,
  args: Object[],
  returnValue: string,
  callback: string[]
}

export interface JSBCallbackParams {
  returnValue: object
}
