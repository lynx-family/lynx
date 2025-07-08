// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxErrorCodeLegacy } from './LynxErrorCodeLegacy';
import { LynxErrorConsumer, LynxErrorLevel, LynxErrorMetaData } from './gen/LynxSubErrorCode';

export class LynxError extends LynxErrorCodeLegacy {
  private static TAG = 'LynxError';
  // The prefix indicates that the field should be placed in the 'context' field.
  static LYNX_ERROR_KEY_PREFIX_CONTEXT = 'lynx_context_';
  // The field 'context' is mainly used to store custom infos that are of interest
  // to the user, while other custom infos are mainly used to assist client-side
  // debugging. LogBox will determine whether to display the information based on
  // whether a custom info is present in field 'context'.
  static LYNX_ERROR_KEY_CONTEXT = 'context';
  // Suggestion for errors with a complex cause that require a detailed explanation
  // on the official site.
  public static LYNX_ERROR_SUGGESTION_REF_OFFICIAL_SITE =
    'Please refer to the solution in Doc "LynxError FAQ" on the official website.';
  // Some commonly used keys of LynxError's customInfo
  public static LYNX_ERROR_KEY_RESOURCE_TYPE = 'type';
  public static LYNX_ERROR_KEY_RESOURCE_URL = 'src';
  public static LYNX_ERROR_KEY_IMAGE_CATEGORIZED_CODE = 'image_categorized_code';
  public static LYNX_THROWABLE = 'throwable';
  public static PLATFORM_ERROR = -3;
  public static JS_ERROR = -2;
  public static NATIVE_ERROR = -1;
  // error level, can take value LEVEL_ERROR or LEVEL_WARN
  private level: LynxErrorLevel;
  private subCode: number;
  private isNewErrorCode: boolean = false;
  private userDefineInfo?: Object;
  // indicates the source of the error, can take
  // value of PLATFORM_ERROR, NATIVE_ERROR
  private errorType: number;
  // be used to store json cache, avoid
  // regenerate json for same error content.
  private jsonStringCache?: string;
  // Error fields to organize structured error,
  // recommend use following fields to construct
  // lynx error over msg and errorObj.

  /** Required fields */
  // a summary message of the error
  private summaryMessage: string;
  // url of the template that reported the error
  private templateUrl?: string;
  // the consumers of the error
  private consumers: Array<LynxErrorConsumer>;
  /** Optional fields */
  // fix suggestion for the error
  private fixSuggestion: string;
  // the call stack when the error occurred
  private callStack?: string;
  private customInfo: Record<string, Object>;

  constructor(level: LynxErrorLevel, errorCode: number, errorMessage: string, fixSuggestion: string,
    errorType: number, customInfo: Record<string, Object>) {
    super();
    this.subCode = errorCode;
    this.summaryMessage = errorMessage;
    this.errorType = errorType;
    this.customInfo = customInfo;
    let metadata = LynxErrorMetaData.getMetaData(errorCode);
    if (metadata) {
      this.level = metadata.level != LynxErrorLevel.Undecided ? metadata.level : level;
      this.fixSuggestion = metadata.fixSuggestion.length > 0 ? metadata.fixSuggestion : fixSuggestion;
      this.consumers = metadata.consumer;
      this.isNewErrorCode = true;
    } else {
      this.level = level;
      this.fixSuggestion = fixSuggestion;
      this.consumers = [];
      this.isNewErrorCode = false;
    }
  }

  static createLynxError(code: number, message: string, customInfo?: Record<string, Object>): LynxError {
    return new LynxError(LynxErrorLevel.Error, code, message, '', LynxError.PLATFORM_ERROR,
      customInfo ? customInfo : {} as Record<string, Object>);
  }

  isFatal() {
    return this.level == LynxErrorLevel.Fatal;
  }

  setTemplateUrl(url?: string) {
    this.jsonStringCache = undefined;
    this.templateUrl = url;
  }

  setCallStack(stack: string) {
    this.jsonStringCache = undefined;
    this.callStack = stack;
  }

  setUserDefineInfo(object: Object) {
    this.jsonStringCache = undefined;
    this.userDefineInfo = object;
  }

  getSummaryMessage() {
    return this.summaryMessage || '';
  }

  getLevel() {
    return this.level;
  }

  getErrorCode() {
    if (this.isNewErrorCode) {
      return Math.floor(this.subCode / 100);
    }
    return this.subCode;
  }

  getSubCode() {
    return this.subCode;
  }

  getType() {
    return this.errorType;
  }

  getMsg() {
    if (this.jsonStringCache) {
      return this.jsonStringCache;
    }
    if (this.summaryMessage) {
      this.jsonStringCache = this.generateJsonString();
    }
    return this.jsonStringCache;
  }

  // covert fields to json string
  private generateJsonString() {
    let json: Record<string, Object> = {};
    // put required fields
    json.error_code = this.getErrorCode();
    json.sub_code = this.subCode;
    json.error = this.summaryMessage;
    json.level = this.level;
    json.consumers = this.consumers;
    // put optional fields
    if (this.templateUrl) {
      json.url = this.templateUrl;
    }
    if (this.fixSuggestion) {
      json.fix_suggestion = this.fixSuggestion;
    }
    if (this.callStack) {
      json.error_stack = this.callStack;
    }
    if (this.userDefineInfo) {
      json.user_define_info = JSON.stringify(this.userDefineInfo);
    }
    let context: Record<string, Object> = {};
    for (let e of Object.entries(this.customInfo)) {
      if (e[0].startsWith(LynxError.LYNX_ERROR_KEY_PREFIX_CONTEXT)) {
        context[e[0]] = e[1];
      } else {
        json[e[0]] = e[1];
      }
    }
    if (Object.entries(context).length !== 0) {
      json.context = context;
    }
    return JSON.stringify(json);
  }
}
