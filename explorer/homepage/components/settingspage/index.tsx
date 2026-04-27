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

  return (
    <view
      clip-radius="true"
      className={withTheme('page')}
      style={{ height: `calc(100% - ${navigatorHeight}px)` }}
    >
      <view
        className="page-header"
        style={{ marginTop: `${Math.max(safeArea.top, 10)}px` }}
      >
        <text className={withTheme('title')}>Settings</text>
      </view>

      <view style="margin: 0px 5% 0px 5%; height: 5%">
        <text className={withTheme('sub-title')}>Theme</text>
      </view>
      <view className={withTheme('theme')}>
        {THEMES.map((theme) => {
          return (
            <view
              key={theme}
              className="option-item"
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

      <view style="margin: 3% 5% 0px 5%; height: 5%">
        <text className={withTheme('sub-title')}>DevTool</text>
      </view>
      <view
        className={withTheme('devtool')}
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

      <view style="margin: 3% 5% 0px 5%; height: 5%">
        <text className={withTheme('sub-title')}>Render Strategy</text>
      </view>
      <view
        className={withTheme('theme')}
        style="height: 8%;justify-content:center"
      >
        <view
          className="option-item"
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

      <view style="margin: 3% 5% 0px 5%; height: 5%">
        <text className={withTheme('sub-title')}>System Info</text>
      </view>
      <view className={withTheme('info-section')}>
        <view className="info-row">
          <text className={withTheme('text')}>Lynx Engine</text>
          <text className={withTheme('info-value')}>
            {SystemInfo.engineVersion}
          </text>
        </view>
        <view className="info-row">
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
    </view>
  );
}
