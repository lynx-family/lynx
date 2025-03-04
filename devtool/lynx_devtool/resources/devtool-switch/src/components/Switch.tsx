import switch_off_image from '../assets/switch_off.png';
import switch_on_image from '../assets/switch_on.png';

import './Switch.css';

interface Props {
  on: boolean;
  onChange: () => void;

  title: string;
  description: string;
}

export function Switch(props: Props) {
  return (
    <view className="item_wrap">
      <view className="label_wrap">
        <text className="label_text">{props.title}</text>
        <text className="desc_text">{props.description}</text>
      </view>
      <view className="switch_wrap" bindtap={props.onChange}>
        <image
          className="switch"
          mode="aspectFit"
          src={props.on ? switch_on_image : switch_off_image}
        />
      </view>
    </view>
  );
}
