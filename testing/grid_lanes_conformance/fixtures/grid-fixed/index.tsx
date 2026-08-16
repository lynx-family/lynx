import { root } from '@lynx-js/react';

import './index.css';

function App() {
  return (
    <view className="page" lynx-test-tag="page">
      <view className="grid" lynx-test-tag="grid">
        <view className="item a" lynx-test-tag="a" />
        <view className="item b" lynx-test-tag="b" />
        <view className="item c" lynx-test-tag="c" />
        <view className="item d" lynx-test-tag="d" />
      </view>
    </view>
  );
}

root.render(<App />);
