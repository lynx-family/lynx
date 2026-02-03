import './Status.css';
import './Basic.css';

interface Props {
  status: string;

  title: string;
  description?: string;

  labelStyle?: object;
  statusStyle?: object;
}

export function Status(props: Props) {
  return (
    <view className="item_wrap">
      <view className="label_wrap" style={props.labelStyle}>
        <text className="label_text">{props.title}</text>
        {props.description && (
          <text className="desc_text">{props.description}</text>
        )}
      </view>
      <view className="status_wrap" style={props.statusStyle}>
        <text className="status_text">{props.status}</text>
      </view>
    </view>
  );
}
