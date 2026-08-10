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
import {
  openSchema,
  navigateTo,
  useTheme,
  useSafeArea,
  getRecentUrls,
  clearRecentUrls,
} from '@explorer/lib';

interface HomePageProps {
  showPage: boolean;
}

export default function HomePage(props: HomePageProps) {
  const { resolved, withTheme } = useTheme();
  const safeArea = useSafeArea();
  const [inputValue, setInputValue] = useState('');
  const [recentUrls, setRecentUrls] = useState<string[]>(() => getRecentUrls());

  const icons = {
    Scan: { dark: ScanIconDark, light: ScanIcon },
    Forward: { dark: ForwardIconDark, light: ForwardIcon },
    Explorer: { dark: ExplorerIconDark, light: ExplorerIcon },
  } as const;

  const openScan = () => {
    'background only';
    NativeModules.ExplorerModule.openScan();
  };

  const openSchemaHandler = () => {
    'background only';
    if (inputValue && inputValue.length > 0) {
      openSchema(inputValue);
      setRecentUrls(getRecentUrls());
    }
  };

  const openShowcasePage = () => {
    'background only';
    navigateTo('showcase/menu/main.lynx.bundle', {
      title: 'Showcase',
      title_color: resolved === 'dark' ? 'FFFFFF' : '000000',
      bar_color: resolved === 'dark' ? '181D25' : 'F0F2F5',
      back_button_style: resolved,
    });
  };

  const handleInput = (event: InputEvent) => {
    'background only';
    const currentValue = event.detail.value.trim();
    setInputValue(currentValue);
  };

  const handleClearRecent = () => {
    'background only';
    clearRecentUrls();
    setRecentUrls([]);
  };

  const handleOpenRecent = (url: string) => {
    'background only';
    openSchema(url);
    setRecentUrls(getRecentUrls());
  };

  const getIcon = (name: keyof typeof icons) => icons[name][resolved];
  const getTextColor = () => (resolved === 'dark' ? '#FFFFFF' : '#000000');

  if (!props.showPage) {
    return <></>;
  }

  const navigatorHeight = 48 + safeArea.bottom;
  const horizontalSafeArea = safeArea.left + safeArea.right;
  const isDesktop =
    lynx.__globalProps.platform === 'macos' ||
    lynx.__globalProps.platform === 'windows';

  return (
    <scroll-view
      scroll-y
      className={withTheme('page')}
      style={{ height: `calc(100% - ${navigatorHeight}px)` }}
    >
      <view
        className="safe-area-content"
        style={{
          marginLeft: `${safeArea.left}px`,
          marginRight: `${safeArea.right}px`,
          width: `calc(100% - ${horizontalSafeArea}px)`,
        }}
      >
        <view
          className="page-header"
          style={{ marginTop: `${Math.max(safeArea.top, 10)}px` }}
        >
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
            height: isDesktop ? '40%' : '148px',
          }}
        >
          <text className={withTheme('bold-text')}>Bundle URL</text>
          <input
            className={withTheme('input-box')}
            bindinput={handleInput}
            placeholder="Enter Bundle URL"
            text-color={getTextColor()}
          />
          <view
            className={withTheme('connect-button')}
            bindtap={openSchemaHandler}
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

        {recentUrls.length > 0 && (
          <view className={withTheme('recent-panel')}>
            <view className="recent-header">
              <text className={withTheme('recent-title')}>Recently Opened</text>
              <text
                className={withTheme('recent-clear')}
                bindtap={handleClearRecent}
                accessibility-element={true}
                accessibility-label="Clear recent history"
                accessibility-traits="button"
              >
                Clear
              </text>
            </view>
            {recentUrls.map((url) => (
              <view
                key={url}
                className={withTheme('recent-item')}
                bindtap={() => handleOpenRecent(url)}
                accessibility-element={true}
                accessibility-label={`Open ${url}`}
                accessibility-traits="button"
              >
                <text
                  className={withTheme('recent-url')}
                  accessibility-element={false}
                >
                  {url}
                </text>
                <view style="margin: auto 5% auto auto; justify-content: center">
                  <image src={getIcon('Forward')} className="forward-icon" />
                </view>
              </view>
            ))}
          </view>
        )}
      </view>
    </scroll-view>
  );
}
