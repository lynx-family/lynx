import radio_off_image from '../assets/radio_off.png';
import radio_on_image from '../assets/radio_on.png';

import './Radio.css';

interface Props {
  on: number;
  onChange: (index: number) => void;

  values: string[];

  title: string;
  description: string;
}

export function Radio(props: Props) {
  return (
    <view className="item_wrap">
      <view className="label_wrap">
        <text className="label_text">{props.title}</text>
        <text className="desc_text">{props.description}</text>
        {props.values.map((label, index) => (
          <view className="radio_wrap" bindtap={() => props.onChange(index)}>
            <text className="sub_label_text">{label}</text>
            <image
              className="radio"
              mode="aspectFit"
              src={props.on == index ? radio_on_image : radio_off_image}
            />
          </view>
        ))}
      </view>
      <view></view>
    </view>
  );
}
