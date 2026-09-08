// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import './index.scss';

import ForwardDarkIcon from '@assets/images/forward-dark.png?inline';
import ForwardIcon from '@assets/images/forward.png?inline';
import { useTheme } from '@explorer/lib';

interface ListRowProps {
  className?: string;
  size?: 'compact' | 'regular' | 'large';
  style?: Record<string, string | number>;
  leading?: JSX.Element;
  title: JSX.Element;
  subtitle?: JSX.Element;
  trailing?: JSX.Element;
  showChevron?: boolean;
  onTap?: () => void;
  accessibilityLabel: string;
  testTag?: string;
}

/**
 * Shared row primitive for Explorer's grouped lists and standalone cards.
 * The trailing slot owns a fixed, vertically-centered lane so chevrons,
 * values, and selection controls line up across every page.
 */
export default function ListRow(props: ListRowProps) {
  const { resolved } = useTheme();
  const size = props.size ?? 'regular';
  const className = `${props.className ?? ''} list-row list-row--${size}`;
  const chevron = resolved === 'dark' ? ForwardDarkIcon : ForwardIcon;

  return (
    <view
      className={className}
      style={props.style}
      bindtap={props.onTap}
      lynx-test-tag={props.testTag}
      ios-platform-accessibility-id={props.testTag}
      accessibility-element={true}
      accessibility-label={props.accessibilityLabel}
      accessibility-traits={props.onTap ? 'button' : undefined}
    >
      {props.leading ? (
        <view className="list-row__leading" accessibility-element={false}>
          {props.leading}
        </view>
      ) : null}
      <view className="list-row__content" accessibility-element={false}>
        {props.title}
        {props.subtitle ?? null}
      </view>
      {props.trailing || props.showChevron ? (
        <view className="list-row__trailing" accessibility-element={false}>
          {props.trailing ?? (
            <image src={chevron} className="list-row__chevron" />
          )}
        </view>
      ) : null}
    </view>
  );
}
