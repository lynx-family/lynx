// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { useState } from '@lynx-js/react';
import './index.scss';

import AutoDarkIcon from '@assets/images/auto-dark.png?inline';
import AutoLightIcon from '@assets/images/auto.png?inline';
import DarkDarkIcon from '@assets/images/dark-dark.png?inline';
import DarkLightIcon from '@assets/images/dark.png?inline';
import ForwardDarkIcon from '@assets/images/forward-dark.png?inline';
import ForwardIcon from '@assets/images/forward.png?inline';
import LightDarkIcon from '@assets/images/light-dark.png?inline';
import LightLightIcon from '@assets/images/light.png?inline';
import {
  navigateTo,
  isSparklingAvailable,
  useTheme,
  useSafeArea,
} from '@explorer/lib';
import type { ThemePreference } from '@explorer/lib';

const THEMES: ThemePreference[] = ['Auto', 'Light', 'Dark'];

interface SettingsPageProps {
  showPage: boolean;
}

const DESKTOP_DEVTOOL_SWITCH_PARAMS = {
  width: 420,
  height: 720,
} as const;

export default function SettingsPage(props: SettingsPageProps) {
  const { preference, resolved, setPreference, withTheme } = useTheme();
  const safeArea = useSafeArea();
  const [listAsyncRender, setListAsyncRender] = useState(false);
  const platform = lynx.__globalProps.platform as string | undefined;

  const icons = {
    Auto: { dark: AutoDarkIcon, light: AutoLightIcon },
    Dark: { dark: DarkDarkIcon, light: DarkLightIcon },
    Light: { dark: LightDarkIcon, light: LightLightIcon },
    Forward: { dark: ForwardDarkIcon, light: ForwardIcon },
  } as const;

  const openDevtoolSwitchPage = () => {
    const isDesktop = platform === 'macos' || platform === 'windows';
    navigateTo(
      'switchPage/devtoolSwitch.lynx.bundle',
      isDesktop ? DESKTOP_DEVTOOL_SWITCH_PARAMS : undefined
    );
  };

  const getIcon = (name: keyof typeof icons) => icons[name][resolved];

  if (!props.showPage) {
    return <></>;
  }

  const navigatorHeight = 48 + safeArea.bottom;
  const horizontalSafeArea = safeArea.left + safeArea.right;
  const screenWidth = Number(lynx.__globalProps.screenWidth || 0);
  const screenHeight = Number(lynx.__globalProps.screenHeight || 0);
  const isLandscape =
    screenWidth > 0 && screenHeight > 0 && screenWidth > screenHeight;
  const labelStyle = 'margin: 0px 5% 0px 5%; height: 20px';
  const labelWithGapStyle = `margin: ${
    isLandscape ? 8 : 16
  }px 5% 0px 5%; height: ${isLandscape ? 20 : 24}px`;
  const optionItemStyle = { height: isLandscape ? '34px' : '44px' };
  const themeStyle = { height: isLandscape ? '102px' : '132px' };
  const rowCardStyle = { height: isLandscape ? '38px' : '48px' };
  const renderCardStyle = {
    height: isLandscape ? '38px' : '48px',
    justifyContent: 'center',
  };
  const infoSectionStyle = {
    marginBottom: isLandscape ? '0px' : '16px',
    padding: isLandscape ? '4px 0' : '8px 0',
  };
  const infoRowStyle = { height: isLandscape ? '32px' : '40px' };

  const renderThemeSection = () => (
    <>
      <view
        style={isLandscape ? labelStyle : 'margin: 0px 5% 0px 5%; height: 24px'}
      >
        <text className={withTheme('sub-title')}>Theme</text>
      </view>
      <view className={withTheme('theme')} style={themeStyle}>
        {THEMES.map((theme) => {
          return (
            <view
              key={theme}
              className="option-item"
              style={optionItemStyle}
              bindtap={() => setPreference(theme)}
              accessibility-element={true}
              accessibility-label={`Set Theme ${theme}`}
              accessibility-traits="button"
            >
              <image
                src={getIcon(theme as keyof typeof icons)}
                className="option-icon"
              />
              <text className={withTheme('text')}>{theme}</text>
              <view
                className={
                  preference === theme
                    ? withTheme('radio-button-container-active')
                    : withTheme('radio-button-container-inactive')
                }
              >
                {preference === theme ? (
                  <view className={withTheme('radio-button-active')} />
                ) : (
                  <view className={withTheme('radio-button')} />
                )}
              </view>
            </view>
          );
        })}
      </view>
    </>
  );

  const renderDevToolSection = () => (
    <>
      <view style={labelWithGapStyle}>
        <text className={withTheme('sub-title')}>DevTool</text>
      </view>
      <view
        className={withTheme('devtool')}
        style={rowCardStyle}
        bindtap={openDevtoolSwitchPage}
        accessibility-element={true}
        accessibility-label="Lynx DevTool Switches"
        accessibility-traits="button"
      >
        <text className={withTheme('text')} accessibility-element={false}>
          Lynx DevTool Switches
        </text>
        <view style="margin: auto 5% auto auto; justify-content: center">
          <image src={getIcon('Forward')} className="forward-icon" />
        </view>
      </view>
    </>
  );

  const renderStrategySection = () => (
    <>
      <view style={isLandscape ? labelStyle : labelWithGapStyle}>
        <text className={withTheme('sub-title')}>Render Strategy</text>
      </view>
      <view className={withTheme('theme')} style={renderCardStyle}>
        <view
          className="option-item"
          style={optionItemStyle}
          bindtap={() => {
            NativeModules.ExplorerModule.setThreadMode(
              !listAsyncRender ? 1 : 0
            );
            setListAsyncRender(!listAsyncRender);
          }}
          accessibility-element={true}
          accessibility-label={'List Async Render'}
          accessibility-traits="button"
        >
          <text className={withTheme('text')}>
            {'Enable List Async Render'}
          </text>
          <view
            className={
              listAsyncRender
                ? withTheme('radio-button-container-active')
                : withTheme('radio-button-container-inactive')
            }
          >
            {listAsyncRender ? (
              <view className={withTheme('radio-button-active')} />
            ) : (
              <view className={withTheme('radio-button')} />
            )}
          </view>
        </view>
      </view>
    </>
  );

  const renderSystemInfoSection = () => (
    <>
      <view style={labelWithGapStyle}>
        <text className={withTheme('sub-title')}>System Info</text>
      </view>
      <view className={withTheme('info-section')} style={infoSectionStyle}>
        <view className="info-row" style={infoRowStyle}>
          <text className={withTheme('text')}>Lynx Engine</text>
          <text className={withTheme('info-value')}>
            {SystemInfo.engineVersion}
          </text>
        </view>
        <view className="info-row" style={infoRowStyle}>
          <text className={withTheme('text')}>Sparkling</text>
          <text
            className={`${withTheme('info-value')} ${
              isSparklingAvailable() ? 'sparkling-active' : 'sparkling-inactive'
            }`}
          >
            {isSparklingAvailable() ? 'Active' : 'N/A'}
          </text>
        </view>
      </view>
    </>
  );

  return (
    <scroll-view
      scroll-y
      clip-radius="true"
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
          <text className={withTheme('title')}>Settings</text>
        </view>

        {isLandscape ? (
          <view className="settings-columns">
            <view className="settings-column">
              {renderThemeSection()}
              {renderDevToolSection()}
            </view>
            <view className="settings-column">
              {renderStrategySection()}
              {renderSystemInfoSection()}
            </view>
          </view>
        ) : (
          <>
            {renderThemeSection()}
            {renderDevToolSection()}
            {renderStrategySection()}
            {renderSystemInfoSection()}
          </>
        )}
        <view style={{ height: `${navigatorHeight}px` }} />
      </view>
    </scroll-view>
  );
}
