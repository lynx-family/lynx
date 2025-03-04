import { useState } from '@lynx-js/react';
import { root } from '@lynx-js/react';
import './index.css';

export function Event() {
  const [count, setCount] = useState(0);
  const [inlineBg, setInlineBg] = useState('transparent');

  const handleTap = () => {
    console.log('handleTap');
    setCount(count + 1);
  };

  const handleInlineTap = () => {
    setInlineBg('red');
  };

  return (
    <scroll-view className="root" lynx-test-tag="scroll-view" scroll-y>
      <view className="count">
        <text
          style={{ textAlign: 'center', fontSize: '18px' }}
          lynx-test-tag="count"
        >
          {count}
        </text>
      </view>
      <text className="title">View Click</text>
      <view lynx-test-tag="button0" className="button" bindtap={handleTap}>
        Click to add one
      </view>

      <text className="title">Text Click</text>
      <view lynx-test-tag="button1" className="button" bindtap={handleTap}>
        <text lynx-test-tag="button-text1" className="button-text">
          Click text to add one
        </text>
      </view>

      <text className="title">Click to change CSS </text>
      <view className="button">
        <text
          lynx-test-tag="button2"
          style={{
            textAlign: 'right',
            width: '100%',
            height: 'max-content',
            backgroundColor: inlineBg,
          }}
        >
          Test inline-text:
          <text lynx-test-tag="inline-text" bindtap={handleInlineTap}>
            Click Me
          </text>
        </text>
      </view>
    </scroll-view>
  );
}

root.render(<Event />);
