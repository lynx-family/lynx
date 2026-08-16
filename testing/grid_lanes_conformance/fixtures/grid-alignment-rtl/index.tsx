import { root } from '@lynx-js/react';

import './index.css';

function App() {
  return (
    <view className="page" lynx-test-tag="page">
      <view className="grid" lynx-test-tag="grid">
        <view className="item first" lynx-test-tag="first" />
        <view className="item second" lynx-test-tag="second" />
        <view className="item third" lynx-test-tag="third" />
      </view>
    </view>
  );
}

root.render(<App />);
