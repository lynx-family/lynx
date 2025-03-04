// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { getBridge } from '@/jsbridge';
import type { IViewsInfoState } from '@/models/viewsInfo';
import { toggleURLDisplay } from '@/models/viewsInfo';
import styles from './index.less';
import React, { useState } from 'react';
import vConsole from '@/utils/vconsole';
import { CloseOutlined, DeleteOutlined, InfoCircleOutlined } from '@ant-design/icons';
import { clearErrors, clearErrorCache } from '@/models/errorReducer';
import { useDispatch, useSelector } from 'react-redux';

export interface IDisplayViewsInfoProps {
  viewsInfo: IViewsInfoState;
}

const _navButtonStyle = {
  border: 'none',
  borderRadius: '4px',
  padding: '3px 6px',
  cursor: 'pointer',
};

const leftButtonStyle = (type) => ({
  ..._navButtonStyle,
  backgroundColor: type === 'redbox' ? '#CD5C5C' : '#F0E68C',
  color: 'black',
  borderTopRightRadius: '0px',
  borderBottomRightRadius: '0px',
  marginRight: '1px',
  flex: 1,
});

const rightButtonStyle = (type) => ({
  ..._navButtonStyle,
  backgroundColor: type === 'redbox' ? '#CD5C5C' : '#F0E68C',
  color: 'black',
  borderTopLeftRadius: '0px',
  borderBottomLeftRadius: '0px',
  right: 0,
  flex: 1,
});

function HeaderImpl(props: IDisplayViewsInfoProps): JSX.Element {
  const { currentView, viewsCount, type, templateUrl } = useSelector((state) => state.viewsInfo);
  const [clickedTimes, updateClickedTimes] = useState(0);
  const dispatch = useDispatch();

  if (clickedTimes > 5) {
    vConsole.showSwitch();
  }

  const clickCallback = () => {
    updateClickedTimes(clickedTimes + 1);
  };

  const clearErrorAndEntry = () => {
    dispatch(clearErrors());
    dispatch(clearErrorCache());
  };

  const previous = () => {
    const number = currentView - 1 < 1 ? viewsCount : currentView - 1;
    clearErrorAndEntry();
    getBridge().changeView(number);
  };

  const next = () => {
    const number = currentView + 1 <= viewsCount ? currentView + 1 : 1;
    clearErrorAndEntry();
    getBridge().changeView(number);
  };
  const urlParams = new URLSearchParams(window.location.search);
  const url = templateUrl ?? urlParams.get('url');

  return (
    <div className={styles.container} style={{ height: viewsCount > 1 ? '90px' : '50px' }}>
      <div
        className={styles.container}
        style={{ height: '40px', display: viewsCount > 1 ? 'flex' : 'none', backgroundColor: type === 'redbox' ? '#CD5C5C' : '#F0E68C' }}
      >
        <button onClick={previous} style={leftButtonStyle(type)}>
          PREVIOUS
        </button>
        <span>{`${currentView} / ${viewsCount} `}</span>
        <button onClick={next} style={rightButtonStyle(type)}>
          NEXT
        </button>
      </div>
      <div className={styles.container} style={{ height: '50px', top: viewsCount > 1 ? 40 : 0 }}>
        <img
          className={styles.icon}
          src="./LynxIcon.svg"
          onClick={() => {
            clickCallback();
          }}
        ></img>
        <span className={styles.title}>Lynx </span>
        <span className={styles.expand}></span>
        {!!url && (
          <InfoCircleOutlined
            className={styles['button-icon']}
            onClick={() => {
              dispatch(toggleURLDisplay());
            }}
          />
        )}
        <span className={styles.gap}></span>
        <DeleteOutlined
          className={styles['button-icon']}
          onClick={() => {
            clearErrorAndEntry();
            getBridge().deleteCurrentView(currentView);
          }}
        />
        <span className={styles.gap}></span>
        <CloseOutlined
          className={styles['button-icon']}
          style={{
            marginRight: '8px',
          }}
          onClick={() => {
            clearErrorAndEntry();
            getBridge().dismissError();
          }}
        />
      </div>
    </div>
  );
}

export const Header = HeaderImpl;
