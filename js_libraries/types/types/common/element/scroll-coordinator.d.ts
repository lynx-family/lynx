import { BaseEvent, BaseMethod } from '../events';
import { StandardProps } from '../props';

export interface ScrollCoordinatorOffset {
  /**
   * The absolute value of the header scroll offset, in px
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  offset: number;
  /**
   * The absolute value of the scrollable distance (header height - toolbar height), in px
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  height: number;
}

export interface ScrollCoordinatorProps extends StandardProps {
  /**
   * When the user taps the status bar, the scroll view beneath the touch which is closest to the status bar will be scrolled to top. iOS feature only.
   * @iOS 3.9
   * @ClayIOS 4.0
   * @defaultValue false
   */
  'ios-scrolls-to-top'?: boolean;
  /**
   * This property controls whether the header hierarchy higher than the slot, when header is overflow, it will be displayed on top of the slot. Otherwise, it will be displayed below the slot. It is better to set this property explicitly, rather than use the default value.
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @defaultValue false
   */
  'header-over-slot'?: boolean;
  /**
   * Set whether the coordinator can scroll vertically.
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @defaultValue true
   */
  'enable-scroll'?: boolean;
  /**
   * Enable the bounce effect when the coordinator scrolls past its boundary.
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayIOS 4.0
   * @defaultValue true
   */
  bounces?: boolean;
  /**
   * Set whether the scrollbar is visible during coordinator scrolling.
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayIOS 4.0
   * @defaultValue false
   */
  'enable-scroll-bar'?: boolean;
  /**
   * Android foldview is based on CoordinateLayout, but it only implements NestedScrollingParent, so it not support to nested scroll as child in other scrolling widget, set this property true to make it work.
   * @Android 3.9
   * @defaultValue false
   */
  'android-nested-scroll-as-child'?: boolean;
  /**
   * Whether to force the nested-vertical-scroll-behavior invalid of foldview on iOS, the setting is ineffective for other platforms
   * @iOS 3.9
   * @ClayIOS 4.0
   * @defaultValue false
   */
  'ios-force-scroll-detach'?: boolean;
  /**
   * The event response granularity of bindoffset, once the scroll distance exceeds the scrollable distance of granularity, it may trigger. It defaults to 0.01, but it doesn't necessarily trigger every 0.01.
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @defaultValue 0.01
   */
  granularity?: number;
  /**
   * The pull-to-refresh mode of foldview, none (none, default value), page (category pull-to-refresh), fold (overall pull-to-refresh)
   * @iOS 3.9
   * @ClayIOS 4.0
   * @defaultValue 'none'
   */
  'refresh-mode'?: 'none' | 'page' | 'fold';
  /**
   * Callback for folding progress
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   */
  bindoffset?: (e: ScrollCoordinatorOffsetEvent) => void;
}

export type ScrollCoordinatorOffsetEvent = BaseEvent<'bindoffset', ScrollCoordinatorOffset>;

export interface ScrollCoordinatorHeaderProps extends StandardProps {}

export interface ScrollCoordinatorSlotProps extends StandardProps {}

export interface ScrollCoordinatorSlotDragProps extends StandardProps {
  /**
   * Whether to allow drag-ng to respond to up and down dragging gestures
   * @Android 2.0
   * @iOS 2.5
   * @ClayAndroid 2.0
   * @ClayIOS 2.8
   * @defaultValue true
   */
  'enable-drag'?: boolean;
}

export interface ScrollCoordinatorToolbarProps extends StandardProps {}

/**
 * @Android 3.9
 * @iOS 3.9
 * @Harmony 3.9
 * @ClayAndroid 3.9
 * @ClayIOS 3.9
 */
export interface ScrollCoordinatorSetFoldExpandedMethod extends BaseMethod {
  method: 'setFoldExpanded';
  params: {
    /**
     * Offset, supports px and rpx
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    offset: string;
    /**
     * Whether folding requires animation
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    smooth: boolean;
  };
}

export type ScrollCoordinatorUIMethods = ScrollCoordinatorSetFoldExpandedMethod;
