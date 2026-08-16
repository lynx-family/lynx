import { root } from '@lynx-js/react';

import './index.css';

root.render(
  <view className="page" lynx-test-tag="page">
    <view className="lanes zero" lynx-test-tag="zero">
      <view className="item a1" lynx-test-tag="a1" />
      <view className="item a2" lynx-test-tag="a2" />
      <view className="item a3" lynx-test-tag="a3" />
    </view>
    <view className="lanes infinite" lynx-test-tag="infinite">
      <view className="item b1" lynx-test-tag="b1" />
      <view className="item b2" lynx-test-tag="b2" />
      <view className="item b3" lynx-test-tag="b3" />
    </view>
  </view>
);
