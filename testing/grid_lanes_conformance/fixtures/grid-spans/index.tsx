import { root } from '@lynx-js/react';

import './index.css';

function App() {
  return (
    <view className="page" lynx-test-tag="page">
      <view className="grid" lynx-test-tag="grid">
        <view className="wide" lynx-test-tag="wide" />
        <view className="tall" lynx-test-tag="tall" />
        <view className="small" lynx-test-tag="small" />
        <view className="last" lynx-test-tag="last" />
      </view>
    </view>
  );
}

root.render(<App />);
