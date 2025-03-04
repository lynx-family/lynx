// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { IErrorState } from '@/models/errorReducer';
import type { IErrorRecord } from '@/common/interface';
import { LogLevels } from '@/common/interface';
import { ErrorContainer } from './error-container';
import styles from './displayError.less';
import React from 'react';
import copy from 'copy-to-clipboard';
import { CopyOutlined } from '@ant-design/icons';
import { getBridge } from '@/jsbridge';
import { popErrorCache } from '@/models/errorReducer';
import { add } from '@/models/errorEffects';
import { useSelector, useDispatch } from 'react-redux';
import { IViewsInfoState } from '@/models/viewsInfo';

export interface IDisplayErrorProps {
  error?: IErrorState;
  viewsInfo?: IViewsInfoState;
}

function getScrollTop() {
  let scrollTop = 0;
  if (document.documentElement && document.documentElement.scrollTop) {
    scrollTop = document.documentElement.scrollTop;
  } else if (document.body) {
    scrollTop = document.body.scrollTop;
  }
  return Math.ceil(scrollTop);
}

function getScrollBarHeight() {
  let scrollBarHeight = document.documentElement.clientHeight;
  return Math.ceil(scrollBarHeight);
}

function getPageHeight() {
  return Math.ceil(Math.max(document.body.clientHeight, document.documentElement.scrollHeight));
}

function createErrorItem(item: IErrorRecord, key: number, cachedErrors: string[]): JSX.Element {
  const dispatch = useDispatch();
  window.onscroll = function () {
    let top = getScrollTop();
    let ch = getScrollBarHeight();
    let sh = getPageHeight();
    if (top + ch + 50 >= sh) {
      if (cachedErrors?.length > 0) {
        const addErrorIntoLogbox = async (error: string) => {
          await add(error);
        };
        addErrorIntoLogbox(cachedErrors[0]);
        dispatch(popErrorCache());
      }
    }
  };

  let level = 'verbose';
  switch (item.errorProps.level ?? LogLevels.Error) {
    case LogLevels.Error:
    case LogLevels.Fatal:
      level = 'error';
      break;
    case LogLevels.Info:
      level = 'info';
      break;
    case LogLevels.Warning:
      level = 'warn';
      break;
    default:
      break;
  }

  return (
    <div key={key} className={styles['log-container'] + ' ' + styles['log-container--' + level]}>
      <div className={styles['log-header'] + ' ' + styles['log-header--' + level]}>
        <div style={{ flexGrow: 1 }}>{level.toLocaleUpperCase()}</div>
        <div style={{ padding: '2px 0px 0px 0px' }}>
          <CopyOutlined
            style={{ fontSize: '16px', color: 'white' }}
            onClick={() => {
              copy(JSON.stringify(item.rawErrorText));
              getBridge().toastMessage('The error message has been copied');
            }}
          />
        </div>
      </div>
      <div className={styles['log-content'] + ' ' + styles['log-content--' + level]}>
        <div className={styles['log-message']}>{item.stackFrames ? item.message : DisplayNativeError(item)}</div>
        {item.stackFrames && (
          <ErrorContainer
            currentRuntimeErrorRecords={[
              {
                error: { message: item.message, name: '', stack: item.errorProps.stack },
                stackFrames: item.stackFrames,
              },
            ]}
          />
        )}
      </div>
    </div>
  );
}

function DisplayNativeError(error: IErrorRecord): JSX.Element {
  const errProps = error.errorProps;
  return (
    <div>
      {errProps.code > 0 && DisplayFieldOfNativeError('code', errProps.code)}
      {DisplayFieldOfNativeError('message', error.message)}
      {errProps.fixSuggestion && DisplayFieldOfNativeError('suggestion', errProps.fixSuggestion)}
      {errProps.criticalInfo &&
        Object.entries(errProps.criticalInfo).map(([key, value]) => {
          return DisplayFieldOfNativeError(key, value);
        })}
    </div>
  );
}

function DisplayFieldOfNativeError(key: string, value: any): JSX.Element {
  return (
    <div key={key} style={{ marginBottom: '3px' }}>
      <strong>{key}</strong>: {value}
    </div>
  );
}

function DisplayErrorImpl(): JSX.Element {
  const errorsOnDisplay = useSelector((state) => state.error.errorsOnDisplay);
  const cachedErrors = useSelector((state) => state.error.cachedErrors);
  const viewInfo = useSelector((state) => state.viewsInfo);
  const showURL = viewInfo.showURL;

  const urlParams = new URLSearchParams(window.location.search);
  const url = urlParams.get('url');
  const URLInfo: IErrorRecord = {
    message: `Current debugging page: \n ${viewInfo?.templateUrl ? viewInfo?.templateUrl : url}`,
    contextSize: 0,
    errorProps: {
      code: 0,
      level: LogLevels.Verbose,
    },
  };

  const errors = showURL ? (errorsOnDisplay ? [URLInfo, ...errorsOnDisplay] : [URLInfo]) : errorsOnDisplay;
  const viewsCount = viewInfo?.viewsCount ?? 0;
  const hasMoreViews = viewsCount > 1 ? true : false;
  if (errorsOnDisplay && errorsOnDisplay.length > 0) {
    return (
      <div className={styles['container']} style={{ marginTop: hasMoreViews ? '90px' : '50px' }}>
        <div style={{ display: 'flex' }}>
          <div style={{ flexGrow: 1, textAlign: 'center', margin: 0, padding: 10 }} />
        </div>
        {errors ? errors.map((e, i) => createErrorItem(e, i, cachedErrors)) : undefined}
      </div>
    );
  } else {
    return (
      <div className={styles.container}>
        <div className={styles.empty}>
          <div style={{ width: '100%', textAlign: 'center' }}>
            Error data is loading, please wait
            <br />
          </div>
        </div>
      </div>
    );
  }
}

export const DisplayError = DisplayErrorImpl;
