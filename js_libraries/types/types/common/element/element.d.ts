// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';
import { NoProps } from '../props';
import { ComponentProps } from './component';
import { FilterImageProps } from './filter-image';
import { ImageProps, ImageUIMethods } from './image';
import { ListProps, ListUIMethods } from './list';
import { ListItemProps } from './list-item';
import { PageProps } from './page';
import { ScrollViewProps, ScrollViewUIMethods } from './scroll-view';
import { TextProps, TextUIMethods } from './text';
import { ViewProps } from './view';
import { InputProps, InputUIMethods } from './input';
import { TextAreaProps, TextAreaUIMethods } from './textarea';
import { FrameProps } from './frame';
import { OverlayProps} from './overlay';
import {
  ScrollCoordinatorHeaderProps,
  ScrollCoordinatorProps,
  ScrollCoordinatorSlotDragProps,
  ScrollCoordinatorSlotProps,
  ScrollCoordinatorToolbarProps,
  ScrollCoordinatorUIMethods,
} from './scroll-coordinator';
import { SVGProps } from './svg';
import { TitleBarViewProps } from './title-bar-view';
import { RefreshProps, RefreshUIMethods } from './refresh';
import { ViewPagerItemProps, ViewPagerProps, ViewPagerUIMethods } from './viewpager';
import { BlurViewProps } from './blur-view';
import { WebviewProps, WebviewUIMethods } from './webview';
import { MarkdownProps, MarkdownUIMethods } from './markdown';
import { VideoProps, VideoUIMethods } from './video';


export interface UIMethods {
  'list': ListUIMethods;
  'scroll-view': ScrollViewUIMethods;
  'image': ImageUIMethods;
  'input': InputUIMethods;
  'textarea': TextAreaUIMethods;
  'text': TextUIMethods;
  'refresh': RefreshUIMethods;
  'scroll-coordinator': ScrollCoordinatorUIMethods;
  'viewpager': ViewPagerUIMethods;
  'webview': WebviewUIMethods;
  'markdown': MarkdownUIMethods;
  'video': VideoUIMethods;
}

type LynxComponentProps = ComponentProps;

// add also to global.JSX.IntrinsicElements
export interface IntrinsicElements {
  /**
   * @compatOverride method boundingClientRect Android=3.5 iOS=3.8 ClayAndroid=4.0 ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus Android=3.5 iOS=3.8
   * @compatOverride method scrollIntoView Android=3.5 iOS=3.8 ClayAndroid=4.0
   * @compatOverride method takeScreenshot Android=3.5 iOS=3.8
   */
  'component': LynxComponentProps;
  /**
   * @Web
   * @compatOverride method setFocus ClayAndroid=2.14
   */
  'filter-image': FilterImageProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   */
  'image': ImageProps;
  /**
   * @Web
   * @compatOverride attribute auto-size Android=false Harmony=2.17 Web=false ClayIOS=2.18 ClayHarmony=2.18
   * @compatOverride attribute autoplay Android=false Harmony=4.0 ClayAndroid=4.0 ClayIOS=2.18 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.1
   * @compatOverride attribute blur-radius Android=false Harmony=2.17 Web=false ClayIOS=2.18
   * @compatOverride attribute cap-insets Android=false Harmony=2.17 ClayAndroid=2.14 ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14
   * @compatOverride attribute cap-insets-scale Android=false Harmony=2.17 ClayAndroid=2.14 ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14
   * @compatOverride attribute defer-src-invalidation Android=false Harmony=2.17 ClayAndroid=4.0 ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride attribute image-config Android=false ClayAndroid=3.1 ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0
   * @compatOverride attribute loop-count Android=4.2 Harmony=4.0 ClayAndroid=2.18 ClayIOS=2.18 ClayMacOS=2.18 ClayWindows=2.18
   * @compatOverride attribute mode Harmony=2.16 Web=false ClayAndroid=2.14 ClayIOS=2.10 ClayHarmony=4.0
   * @compatOverride attribute placeholder Android=false Harmony=2.17 Web=false ClayIOS=2.16
   * @compatOverride attribute prefetch-height Android=false
   * @compatOverride attribute prefetch-width Android=false
   * @compatOverride attribute src Harmony=2.16 ClayAndroid=3.1 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride attribute tint-color Android=false Harmony=3.1 ClayAndroid=4.0 ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride event currentloopcomplete Android=false Harmony=4.1 ClayAndroid=4.0 ClayIOS=2.18 ClayHarmony=4.1
   * @compatOverride event error Android=2.0 Harmony=2.17 Web=false ClayAndroid=2.14
   * @compatOverride event finalloopcomplete Android=false ClayAndroid=2.18 ClayIOS=2.18 ClayHarmony=4.1
   * @compatOverride event load Android=2.0 Harmony=2.17 Web=false
   * @compatOverride event startplay Android=false Harmony=4.1 ClayAndroid=4.0 ClayIOS=2.18 ClayHarmony=4.1
   * @compatOverride method requestAccessibilityFocus Android=false
   * @compatOverride method scrollIntoView Android=false ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   * @compatOverride method setFocus ClayAndroid=2.14 ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   * @compatOverride method takeScreenshot Android=false
   */
  'inline-image': ImageProps;
  /**
   * @Web
   * @compatOverride attribute custom-context-menu Android=false iOS=false Harmony=false ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride attribute custom-text-selection Android=false iOS=false Harmony=false ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride attribute include-font-padding Android=1.3
   * @compatOverride attribute tail-color-convert Android=false iOS=false Web=false
   * @compatOverride attribute text-maxlength iOS=false Web=false
   * @compatOverride attribute text-maxline Android=false iOS=false Harmony=false Web=false ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride attribute text-selection Android=false iOS=false Harmony=false Web=false ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride attribute text-single-line-vertical-align Android=false iOS=false ClayAndroid=3.1 ClayIOS=2.12
   * @compatOverride attribute text-vertical-align iOS=false
   * @compatOverride event layout Android=false iOS=false Harmony=false Web=false ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride event selectionchange Android=false iOS=false Harmony=false ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method boundingClientRect Android=false iOS=false Harmony=false ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus Android=false iOS=false
   * @compatOverride method scrollIntoView Android=false iOS=false Harmony=false ClayIOS=2.16
   * @compatOverride method setFocus ClayIOS=2.16
   * @compatOverride method takeScreenshot Android=false iOS=false Harmony=false ClayIOS=2.18
   */
  'inline-text': TextProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect Android=false iOS=false Harmony=false ClayIOS=2.14 ClayHarmony=2.17
   * @compatOverride method requestAccessibilityFocus Android=false iOS=false
   * @compatOverride method scrollIntoView Android=false iOS=false Harmony=false ClayIOS=2.14 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.17
   * @compatOverride method setFocus ClayAndroid=2.14 ClayIOS=2.14 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.17
   * @compatOverride method takeScreenshot Android=false iOS=false Harmony=false
   */
  'inline-truncation': NoProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect iOS=3.8 ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus iOS=3.8
   * @compatOverride method scrollIntoView iOS=3.8
   * @compatOverride method takeScreenshot Android=3.5 iOS=3.8
   */
  'list': ListProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect iOS=3.5 ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus iOS=3.5
   * @compatOverride method scrollIntoView iOS=3.5
   * @compatOverride method takeScreenshot Android=3.5 iOS=3.5
   */
  'list-item': ListItemProps;
  /**
   * @compatOverride method boundingClientRect iOS=3.5 ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus Android=3.5 iOS=3.5
   * @compatOverride method scrollIntoView Android=3.5 iOS=3.5
   * @compatOverride method takeScreenshot Android=3.5 iOS=3.5
   */
  'list-row': StandardProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect iOS=4.1 Harmony=4.1 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus iOS=4.1
   * @compatOverride method scrollIntoView iOS=4.1 Harmony=4.1 ClayIOS=2.16
   * @compatOverride method setFocus ClayIOS=2.16
   * @compatOverride method takeScreenshot Android=4.1 iOS=4.1 Harmony=4.1 ClayIOS=2.18
   */
  'page': PageProps;
  /**
   * @Web
   * @compatOverride attribute enable-scrollbar Web=true
   * @compatOverride method boundingClientRect iOS=4.1 Harmony=4.1 ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus iOS=4.1
   * @compatOverride method scrollIntoView iOS=4.1 Harmony=3.5 ClayHarmony=2.14
   * @compatOverride method takeScreenshot iOS=4.1 Harmony=4.1
   */
  'scroll-view': ScrollViewProps;
  /**
   * @Web
   * @compatOverride method scrollIntoView ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   * @compatOverride method setFocus ClayAndroid=2.14 ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   */
  'text': TextProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect Android=1.0 iOS=1.0 Harmony=3.4 ClayAndroid=1.5 ClayIOS=1.5 ClayMacOS=1.5 ClayWindows=1.5 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus Android=1.0 iOS=1.0
   * @compatOverride method scrollIntoView Android=2.16 iOS=2.16 Harmony=4.0 ClayAndroid=4.0 ClayIOS=2.18
   * @compatOverride method setFocus ClayIOS=2.18
   * @compatOverride method takeScreenshot Android=1.0 iOS=1.0 Harmony=3.4 ClayAndroid=3.3 ClayIOS=3.1 ClayMacOS=3.3 ClayWindows=3.3
   */
  'view': ViewProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect Android=false iOS=false Harmony=false
   * @compatOverride method requestAccessibilityFocus Android=false iOS=false
   * @compatOverride method scrollIntoView Android=false iOS=false Harmony=false ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   * @compatOverride method setFocus ClayAndroid=2.14 ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   * @compatOverride method takeScreenshot Android=false iOS=false Harmony=false ClayIOS=2.18
   */
  'raw-text': StandardProps & {
    /**
     * @Android 0.1
     * @iOS 2.18
     * @Harmony 2.8
     * @Web
     * @ClayAndroid 1.5
     * @ClayIOS 2.8
     * @ClayMacOS 2.8
     * @ClayWindows 2.8
     * @ClayHarmony 2.8
     */
    text: number | string };
  /**
   * @Web
   * @compatOverride method boundingClientRect Web=true
   * @compatOverride method scrollIntoView Web=true ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   * @compatOverride method setFocus ClayAndroid=1.5 ClayIOS=2.8 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   */
  'input': InputProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect Web=true ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus iOS=4.1 ClayIOS=3.0
   * @compatOverride method scrollIntoView Web=true
   * @compatOverride method setFocus ClayAndroid=1.5 ClayIOS=2.8 ClayWindows=2.14
   * @compatOverride method takeScreenshot Android=4.1 iOS=4.1 Harmony=4.1
   */
  'textarea': TextAreaProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect Android=3.4 iOS=3.3 Web=true ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus Android=3.4 iOS=3.3
   * @compatOverride method scrollIntoView Android=3.4 iOS=3.3 Web=true ClayIOS=2.16
   * @compatOverride method setFocus ClayIOS=2.16
   * @compatOverride method takeScreenshot Android=3.4 iOS=3.3
   */
  'frame': FrameProps;
  /**
   * @compatOverride method boundingClientRect Android=1.5 iOS=4.0 Harmony=4.0 Web=false ClayMacOS=false ClayWindows=false ClayHarmony=3.6
   * @compatOverride method requestAccessibilityFocus ClayAndroid=3.1 ClayIOS=2.8
   * @compatOverride method scrollIntoView Web=false ClayIOS=2.16 ClayMacOS=false ClayWindows=false ClayHarmony=3.6
   * @compatOverride method setFocus ClayAndroid=2.14 ClayIOS=2.16 ClayMacOS=false ClayWindows=false ClayHarmony=3.6
   * @compatOverride method takeScreenshot ClayMacOS=false ClayWindows=false
   */
  'overlay': OverlayProps;
  /**
   * @Web
   * @compatOverride method scrollIntoView ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   * @compatOverride method setFocus ClayAndroid=2.14 ClayIOS=2.16 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=2.14
   */
  'svg': SVGProps;
  /**
   * @compatOverride method boundingClientRect Android=false iOS=false Harmony=4.1 ClayIOS=3.7 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus Android=false iOS=false
   * @compatOverride method scrollIntoView Android=false iOS=false Harmony=4.1 ClayAndroid=3.7 ClayIOS=3.8
   * @compatOverride method setFocus ClayAndroid=3.7 ClayIOS=3.8
   * @compatOverride method takeScreenshot Android=false iOS=false Harmony=4.1 ClayAndroid=3.7 ClayIOS=3.8
   */
  'title-bar-view': TitleBarViewProps;
  /**
   * @compatOverride method boundingClientRect Android=1.5 iOS=3.4 Harmony=4.1 ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus iOS=3.4
   * @compatOverride method scrollIntoView Android=1.5 iOS=3.4 Harmony=4.1 ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot Android=3.5 iOS=3.4 Harmony=4.1 ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'refresh': RefreshProps;
  /**
   * @compatOverride method boundingClientRect Android=1.5 iOS=4.0 Harmony=4.1 ClayAndroid=false ClayIOS=4.0 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus iOS=4.0
   * @compatOverride method scrollIntoView Android=1.5 iOS=4.0 Harmony=4.1 ClayAndroid=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot Android=4.0 iOS=4.0 Harmony=4.1 ClayAndroid=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'refresh-header': StandardProps;
  /**
   * @compatOverride method boundingClientRect Android=1.5 iOS=4.0 Harmony=4.0 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus iOS=4.0 ClayAndroid=3.1 ClayIOS=2.18
   * @compatOverride method scrollIntoView Android=4.0 iOS=4.0 Harmony=4.0 ClayAndroid=4.0 ClayIOS=2.18 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayIOS=2.16 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot Android=4.0 iOS=4.0 Harmony=4.0 ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'scroll-coordinator': ScrollCoordinatorProps;
  /**
   * @compatOverride method boundingClientRect Android=1.5 iOS=4.0 Harmony=4.0 ClayIOS=3.7 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus iOS=4.0 ClayAndroid=3.1 ClayIOS=3.8
   * @compatOverride method scrollIntoView Android=1.5 iOS=4.0 Harmony=4.0 ClayIOS=3.7 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayIOS=3.7 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot Android=4.0 iOS=4.0 Harmony=4.0 ClayIOS=3.7 ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'scroll-coordinator-header': ScrollCoordinatorHeaderProps;
  /**
   * @compatOverride method boundingClientRect ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus ClayAndroid=3.1 ClayIOS=2.8
   * @compatOverride method scrollIntoView ClayIOS=2.16 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayAndroid=2.14 ClayIOS=2.16 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'scroll-coordinator-slot': ScrollCoordinatorSlotProps;
  /**
   * @compatOverride method boundingClientRect Harmony=4.1 ClayAndroid=false ClayIOS=4.0 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus ClayIOS=3.0
   * @compatOverride method scrollIntoView Harmony=4.1 ClayAndroid=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayAndroid=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot Harmony=4.1 ClayAndroid=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'scroll-coordinator-slot-drag': ScrollCoordinatorSlotDragProps;
  /**
   * @compatOverride method boundingClientRect Android=1.5 iOS=4.0 Harmony=4.0 ClayIOS=3.7 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus iOS=4.0 ClayIOS=3.7
   * @compatOverride method scrollIntoView Android=1.5 iOS=4.0 Harmony=4.0 ClayIOS=3.7 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayIOS=3.7 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot Android=4.0 iOS=4.0 Harmony=4.0 ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'scroll-coordinator-toolbar': ScrollCoordinatorToolbarProps;
  /**
   * @compatOverride method boundingClientRect ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus ClayAndroid=3.1 ClayIOS=2.8
   * @compatOverride method scrollIntoView ClayIOS=2.16 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayAndroid=2.14 ClayIOS=2.16 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'viewpager': ViewPagerProps;
  /**
   * @compatOverride method boundingClientRect ClayIOS=4.0 ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus ClayAndroid=3.1 ClayIOS=3.0
   * @compatOverride method scrollIntoView ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'viewpager-item': ViewPagerItemProps;
  /**
   * @compatOverride method boundingClientRect ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=4.0
   * @compatOverride method scrollIntoView ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false
   * @compatOverride method setFocus ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false
   * @compatOverride method takeScreenshot ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false
   */
  'blur-view': BlurViewProps;
  /**
   * @compatOverride method boundingClientRect Web=false ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method scrollIntoView Web=false ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method setFocus ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   * @compatOverride method takeScreenshot ClayAndroid=false ClayIOS=false ClayMacOS=false ClayWindows=false ClayHarmony=false
   */
  'webview': WebviewProps;
  /**
   * @compatOverride method boundingClientRect Android=2.15 iOS=1.0 Web=false ClayAndroid=2.15 ClayIOS=4.0 ClayMacOS=4.0 ClayWindows=4.0 ClayHarmony=4.0
   * @compatOverride method requestAccessibilityFocus Android=2.15
   * @compatOverride method scrollIntoView Android=2.15 Web=false ClayAndroid=2.15
   */
  'markdown': MarkdownProps;
  /**
   * @Web
   * @compatOverride method boundingClientRect Android=4.1 iOS=4.1 Harmony=4.1 Web=true ClayAndroid=4.1 ClayIOS=4.0 ClayHarmony=false
   * @compatOverride method requestAccessibilityFocus Android=4.1 iOS=4.1
   * @compatOverride method scrollIntoView Android=4.1 iOS=4.1 Harmony=4.1 Web=true ClayAndroid=4.1 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=false
   * @compatOverride method setFocus ClayAndroid=4.1 ClayMacOS=2.14 ClayWindows=2.14 ClayHarmony=false
   * @compatOverride method takeScreenshot Android=4.1 iOS=4.1 Harmony=4.1 ClayAndroid=false ClayHarmony=false
   */
  'video': VideoProps;
}

declare module 'react' {
  namespace JSX {
    // Should copy from above IntrinsicElements
    interface IntrinsicElements {
      'component': LynxComponentProps;
      'filter-image': FilterImageProps;
      'image': ImageProps;
      'inline-image': ImageProps;
      'inline-text': TextProps;
      'inline-truncation': NoProps;
      'list': ListProps;
      'list-item': ListItemProps;
      'list-row': StandardProps;
      'page': PageProps;
      'scroll-view': ScrollViewProps;
      'text': TextProps;
      'view': ViewProps;
      'raw-text': StandardProps & { text: number | string };
      'input': InputProps;
      'textarea': TextAreaProps;
      'frame': FrameProps;
      'overlay': OverlayProps;
      'svg': SVGProps;
      'title-bar-view': TitleBarViewProps;
      'refresh': RefreshProps;
      'refresh-header': StandardProps;
      'scroll-coordinator': ScrollCoordinatorProps;
      'scroll-coordinator-header': ScrollCoordinatorHeaderProps;
      'scroll-coordinator-slot': ScrollCoordinatorSlotProps;
      'scroll-coordinator-slot-drag': ScrollCoordinatorSlotDragProps;
      'scroll-coordinator-toolbar': ScrollCoordinatorToolbarProps;
      'viewpager': ViewPagerProps;
      'viewpager-item': ViewPagerItemProps;
      'blur-view': BlurViewProps;
      'webview': WebviewProps;
      'markdown': MarkdownProps;
      'video': VideoProps;
    }
  }
}
