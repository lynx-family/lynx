// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { EventHandlerArray, IContext, LLog, NativeContent, UIBase, UIGroup } from '@lynx/lynx';

// Sample usage code.
/**
 export default function App() {
 return (
 <view style={{ flexDirection: 'column' }} className="container">
 <text>Hello, ReactLynx 3!</text>
 <click-view style="width: 300px; height: 300px;" >
 <view style="width: 200px; height: 200px; background: red;"/>
 </click-view>
 </view>
 );
 }
 */
@Component
struct ClickView {
  @Prop @Watch('update') count: number;
  @State children: NativeContent[] = [];
  @State extraDataString: string = '';
  ui?: UIClickView

  aboutToDisappear(): void {
    LLog.d('UIClickView', 'Disappearing');
  }

  update() {
    if (this.ui) {
      this.children = this.ui.children;
      this.extraDataString = this.ui.extraDataString;
    }
  }

  aboutToAppear(): void {
  }

  build() {
    Column() {
      Text(this.ui?.customData).fontSize("32vp")
      Text(this.extraDataString).fontSize("32vp")
      Scroll() {
        Row() {
          ForEach(this.children, (item: NativeContent, index: number) => {
            Flex({ direction: FlexDirection.Row }) {
              ContentSlot(item.content)
            }.width('50%')
          })
        }.alignItems(VerticalAlign.Top).width('200%')
      }.scrollable(ScrollDirection.Horizontal).align(Alignment.TopStart)
      .width('100%').height('100%')
    }
    .size({ width: '100%', height: '100%' }).position({ x: '0', y: '0' }).onClick(() => {
      this.count++;
    })
  }
}

@Builder
function buildClickView(ui: UIBase) {
  ClickView({
    ui: ui as UIClickView,
    count: (ui as UIClickView).updateCount,
    children: (ui as UIClickView).children
  }).size({
    width: '100%',
    height: '100%'
  }).position({
    x: '0',
    y: '0'
  }).onAppear(
    () => {
      ui.lynxContext?.sendGlobalEvent('onClickViewCreate', [true]);
    }
  )
}

export class UIClickView extends UIGroup {
  readonly builder: WrappedBuilder<[UIBase]> = wrapBuilder<[UIBase]>(buildClickView);
  customData: string | undefined;

  constructor(context: IContext, param?: Object) {
    super(context, param);
    if (param) {
      this.customData = JSON.stringify(param);
    }
  }

  override update(props: Object, events?: EventHandlerArray[]): void {
    super.update(props, events);
    const entries: [string, Object][] = Object.entries(props);

    for (const entry of entries) {
      LLog.e('UIClickView', `Update prop ${entry[0]}: ${entry[1]}`);
    }
  }

  hasCustomizedLayout(): boolean {
    return true;
  }

  override invokeMethod(method: string, params: Object, callback: (code: number, res: Object) => void): boolean {
    LLog.e('UIClickView', `Invoke method ${method}`);
    return false;
  }

  insertChild(nativeContent: NativeContent, index: number): void {
    super.insertChild(nativeContent, index);
    this.updateCount++;
    this.builderNode?.update(this);
  }

  removeChild(nativeContent: NativeContent): void {
    super.removeChild(nativeContent);
    this.updateCount++;
    this.builderNode?.update(this);
  }

  updateCount: number = 0;
  extraDataString: string = '';

  override layout(x: number, y: number, width: number, height: number, paddingLeft: number, paddingTop: number,
    paddingRight: number, paddingBottom: number, marginLeft: number, marginTop: number, marginRight: number,
    marginBottom: number, sticky?: number[] | undefined): void {
    // throw new Error('Method not implemented.');
  }

  override updateExtraData(data: Object): void {
    this.extraDataString = data as string;
    this.updateCount++;
    this.builderNode?.update(this);
  }
}