// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { useState } from '@lynx-js/react';
import './index.scss';

import ExplorerIconDark from '@assets/images/explorer-dark.png?inline';
import ExplorerIcon from '@assets/images/explorer.png?inline';
import ForwardIconDark from '@assets/images/forward-dark.png?inline';
import ForwardIcon from '@assets/images/forward.png?inline';
import ScanIconDark from '@assets/images/scan-dark.png?inline';
import ScanIcon from '@assets/images/scan.png?inline';
import ShowcaseIcon from '@assets/images/showcase.png?inline';
import type { InputEvent } from '../../typing';

interface HomePageProps {
  showPage: boolean;
  currentTheme: string;
  withTheme: (className: string) => string;
  withNotchScreen: (className: string) => string;
}

export default function HomePage(props: HomePageProps) {
  const [inputValue, setInputValue] = useState('');

  const icons = {
    Scan: {
      Dark: ScanIconDark,
      Light: ScanIcon,
    },
    Forward: {
      Dark: ForwardIconDark,
      Light: ForwardIcon,
    },
    Explorer: {
      Dark: ExplorerIconDark,
      Light: ExplorerIcon,
    },
  } as const;

  type Theme = 'Dark' | 'Light';

  const openScan = () => {
    'background only';
    NativeModules.ExplorerModule.openScan();
  };

  const openSchema = () => {
    'background only';
    if (inputValue && inputValue.length > 0) {
      NativeModules.ExplorerModule.openSchema(inputValue);
    }
  };

  const openShowcasePage = () => {
    'background only';
    const theme =
      props.currentTheme === 'Auto'
        ? lynx.__globalProps.theme
        : props.currentTheme;
    const titleColor = theme === 'Dark' ? 'FFFFFF' : '000000';
    const barColor = theme === 'Dark' ? '181D25' : 'F0F2F5';
    const backButtonStyle = theme.toLowerCase();

    const query = `title=Showcase&title_color=${titleColor}&bar_color=${barColor}&back_button_style=${backButtonStyle}`;
    NativeModules.ExplorerModule.openSchema(
      `file://lynx?local://showcase/menu/main.lynx.bundle?${query}`
    );
  };

  const handleInput = (event: InputEvent) => {
    'background only';
    const currentValue = event.detail.value.trim();
    setInputValue(currentValue);
  };

  const getIcon = (name: keyof typeof icons) => {
    const { currentTheme } = props;
    if (currentTheme !== 'Auto') {
      return icons[name][currentTheme as Theme];
    }
    return icons[name][lynx.__globalProps.theme as Theme];
  };

  const getTextColor = () => {
    const { currentTheme } = props;
    if (currentTheme !== 'Auto') {
      return currentTheme === 'Dark' ? '#FFFFFF' : '#000000';
    }
    return lynx.__globalProps.theme === 'Dark' ? '#FFFFFF' : '#000000';
  };

  const { showPage, withTheme, withNotchScreen } = props;
  if (!showPage) {
    return <></>;
  }

  return (
    <view clip-radius="true" className={withTheme('page')}>
      <view className={withNotchScreen('page-header')}>
        <image src={getIcon('Explorer')} className="logo" mode="aspectFit" />
        <text className={withTheme('home-title')}>Lynx Explorer</text>
        <view className="scan">
          {(() => {
            if (SystemInfo.platform === 'iOS') {
              return <></>;
            }
            return (
              <image
                src={getIcon('Scan')}
                className="scan-icon"
                bindtap={openScan}
                accessibility-element={true}
                accessibility-label="Open Scan"
                accessibility-traits="button"
              />
            );
          })()}
        </view>
      </view>

      <view
        className={withTheme('input-card-url')}
        style={{
          height:
            lynx.__globalProps.platform === 'macos' ||
            lynx.__globalProps.platform === 'windows'
              ? '40%'
              : '28%',
        }}
      >
        <text className={withTheme('bold-text')}>Card URL</text>
        <input
          className="input-box"
          bindinput={handleInput}
          placeholder="Enter Card URL"
          text-color={getTextColor()}
        />
        <view
          className={withTheme('connect-button')}
          bindtap={openSchema}
          accessibility-element={true}
          accessibility-label="Open Schema"
          accessibility-traits="button"
        >
          <text
            style="line-height: 22px; color: #ffffff; font-size: 16px"
            accessibility-element={false}
          >
            Go
          </text>
        </view>
      </view>

      <view
        className={withTheme('showcase')}
        bindtap={openShowcasePage}
        accessibility-element={true}
        accessibility-label="Open Show Cases"
        accessibility-traits="button"
      >
        <image src={ShowcaseIcon} className="showcase-icon" />
        <text className={withTheme('text')} accessibility-element={false}>
          Showcase
        </text>
        <view style="margin: auto 5% auto auto; justify-content: center">
          <image src={getIcon('Forward')} className="forward-icon" />
        </view>
      </view>
    </view>
  );
}
