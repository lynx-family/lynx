import { root } from '@lynx-js/react';

export function renderGridLanesFixture() {
  root.render(
    <view className="page" lynx-test-tag="page">
      <view className="lanes" lynx-test-tag="lanes">
        <view className="item item1" lynx-test-tag="item1" />
        <view className="item item2" lynx-test-tag="item2" />
        <view className="item item3" lynx-test-tag="item3" />
        <view className="item item4" lynx-test-tag="item4" />
        <view className="item item5" lynx-test-tag="item5" />
        <view className="item item6" lynx-test-tag="item6" />
      </view>
    </view>
  );
}
