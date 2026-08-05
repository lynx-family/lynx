// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root, useEffect, useState } from '@lynx-js/react';
import {
  close as sparklingClose,
  open as sparklingOpen,
} from 'sparkling-navigation';

import './index.css';

const CHILD_SCHEME =
  'hybrid://lynxview_page?bundle=automation%2Fnav-basic%2Fmain.lynx.bundle&nav_role=child';
const MALFORMED_CANONICAL_SCHEME = 'hybrid://lynxview_page?bundle=';

type ContainerRequest = 'automatic' | 'legacy' | 'sparkling';

interface NativeRouteResult {
  success: boolean;
  code: string;
  message: string;
}

interface ExplorerNativeModule {
  openRoute(
    url: string,
    container: ContainerRequest,
    callback: (result: NativeRouteResult) => void
  ): void;
}

function displayProp(props: Record<string, unknown>, name: string): string {
  const value = props[name];
  if (value === undefined || value === null || value === '') {
    return 'absent';
  }
  return String(value);
}

function explorerModule(): ExplorerNativeModule {
  'background only';
  return NativeModules.ExplorerModule as ExplorerNativeModule;
}

export function NavBasic() {
  const props = (lynx.__globalProps as unknown) as Record<string, unknown>;
  const role = displayProp(props, 'navRole') === 'child' ? 'child' : 'parent';
  const [pipeState, setPipeState] = useState('checking');
  const [routeResult, setRouteResult] = useState('idle');

  useEffect(() => {
    'background only';
    try {
      const modules = (NativeModules as unknown) as Record<string, unknown>;
      const pipe = modules.spkPipe as { call?: unknown } | undefined;
      setPipeState(typeof pipe?.call === 'function' ? 'available' : 'absent');
    } catch {
      setPipeState('absent');
    }
  }, []);

  const openChild = () => {
    'background only';
    setRouteResult('router.open:pending');
    sparklingOpen({ scheme: CHILD_SCHEME }, (result) => {
      setRouteResult(
        result.code === 1
          ? 'router.open:ok'
          : `router.open:${result.code}:${result.msg}`
      );
    });
  };

  const closeChild = () => {
    'background only';
    setRouteResult('router.close:pending');
    sparklingClose({ animated: false }, (result) => {
      setRouteResult(
        result.code === 1
          ? 'router.close:ok'
          : `router.close:${result.code}:${result.msg}`
      );
    });
  };

  const openMalformedCanonical = () => {
    'background only';
    setRouteResult('malformed:pending');
    explorerModule().openRoute(
      MALFORMED_CANONICAL_SCHEME,
      'automatic',
      (result) => {
        setRouteResult(
          result.success
            ? 'malformed:unexpected-success'
            : `malformed:${result.code}`
        );
      }
    );
  };

  return (
    <scroll-view className="page" scroll-y lynx-test-tag="nav-basic-root">
      <text
        className="heading"
        lynx-test-tag="nav-page-marker"
        ios-platform-accessibility-id="nav-page-marker"
        accessibility-element={true}
        accessibility-label={`Sparkling nav-basic ${role}`}
      >
        {`Sparkling nav-basic ${role}`}
      </text>

      <input
        className="xelement-probe"
        value="XElement ready"
        readonly={true}
        show-soft-input-on-focus={false}
        lynx-test-tag="nav-xelement-input"
        ios-platform-accessibility-id="nav-xelement-input"
        accessibility-element={true}
        accessibility-label="XElement ready"
      />

      {role === 'parent' ? (
        <>
          <view
            className="button"
            bindtap={openChild}
            lynx-test-tag="nav-open-child"
            ios-platform-accessibility-id="nav-open-child"
            accessibility-element={true}
            accessibility-label="Open nav-basic child"
            accessibility-traits="button"
          >
            <text className="button-text" accessibility-element={false}>
              Open child with router.open
            </text>
          </view>
          <view
            className="button secondary"
            bindtap={openMalformedCanonical}
            lynx-test-tag="nav-open-malformed"
            ios-platform-accessibility-id="nav-open-malformed"
            accessibility-element={true}
            accessibility-label="Open malformed canonical"
            accessibility-traits="button"
          >
            <text className="button-text" accessibility-element={false}>
              Open malformed canonical
            </text>
          </view>
        </>
      ) : (
        <view
          className="button"
          bindtap={closeChild}
          lynx-test-tag="nav-close-child"
          ios-platform-accessibility-id="nav-close-child"
          accessibility-element={true}
          accessibility-label="Close nav-basic child"
          accessibility-traits="button"
        >
          <text className="button-text" accessibility-element={false}>
            Close child with router.close
          </text>
        </view>
      )}

      <view className="panel">
        <text className="label">Role</text>
        <text className="value" lynx-test-tag="nav-role">
          {role}
        </text>
        <text className="label">containerType</text>
        <text className="value" lynx-test-tag="nav-container-type">
          {displayProp(props, 'containerType')}
        </text>
        <text className="label">containerID</text>
        <text className="value" lynx-test-tag="nav-container-id">
          {displayProp(props, 'containerID')}
        </text>
        <text className="label">sparklingNavigation</text>
        <text className="value" lynx-test-tag="nav-sparkling-navigation">
          {displayProp(props, 'sparklingNavigation')}
        </text>
        <text className="label">spkPipe</text>
        <text className="value" lynx-test-tag="nav-spk-pipe">
          {pipeState}
        </text>
        <text className="label">theme</text>
        <text className="value" lynx-test-tag="nav-theme">
          {displayProp(props, 'theme')}
        </text>
        <text className="label">isNotchScreen</text>
        <text className="value" lynx-test-tag="nav-is-notch-screen">
          {displayProp(props, 'isNotchScreen')}
        </text>
        <text className="label">lynxSdkVersion</text>
        <text className="value" lynx-test-tag="nav-lynx-sdk-version">
          {displayProp(props, 'lynxSdkVersion')}
        </text>
      </view>

      <view className="panel">
        <text className="label">title</text>
        <text className="value" lynx-test-tag="nav-title">
          {displayProp(props, 'title')}
        </text>
        <text className="label">hiddenNav</text>
        <text className="value" lynx-test-tag="nav-hidden-nav">
          {displayProp(props, 'hiddenNav')}
        </text>
        <text className="label">fullscreen</text>
        <text className="value" lynx-test-tag="nav-fullscreen">
          {displayProp(props, 'fullscreen')}
        </text>
        <text className="label">backButtonStyle</text>
        <text className="value" lynx-test-tag="nav-back-button-style">
          {displayProp(props, 'backButtonStyle')}
        </text>
        <text className="label">initialPage</text>
        <text className="value" lynx-test-tag="nav-initial-page">
          {displayProp(props, 'initialPage')}
        </text>
        <text className="label">customFlag</text>
        <text className="value" lynx-test-tag="nav-custom-flag">
          {displayProp(props, 'customFlag')}
        </text>
      </view>

      <view className="panel">
        <text className="label">Route result</text>
        <text className="value" lynx-test-tag="nav-route-result">
          {routeResult}
        </text>
        <text className="value" lynx-test-tag="nav-return-success">
          {role === 'parent' && routeResult === 'router.open:ok'
            ? 'returned:ok'
            : 'returned:pending'}
        </text>
      </view>
    </scroll-view>
  );
}

root.render(<NavBasic />);
