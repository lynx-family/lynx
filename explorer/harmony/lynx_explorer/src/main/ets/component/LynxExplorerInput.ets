// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { EventHandlerArray, LynxUIMethodConstants, UIBase } from '@lynx/lynx';

// Sample usage code.
// export default function App() {
//   return (
//     <view style={{ flexDirection: 'column' }} className="container">
//   <input placeholder="Enter Card URL"/>
//   </view>
// );
// }

interface InputEvent {
  value: string,
}

@ObservedV2
class InputParams {
  constructor(ui: LynxExplorerInput) {
    this.ui = ui;
  }

  @Trace inputText: string = '';
  @Trace placeholder: string = '';
  @Trace fontColor: ResourceColor = Color.Black;
  @Trace paddingLeft: number = 0;
  @Trace paddingTop: number = 0;
  @Trace paddingRight: number = 0;
  @Trace paddingBottom: number = 0;
  ui: LynxExplorerInput;
}

@ComponentV2
struct InputView {
  @Param @Require inputParams: InputParams;

  build() {
    Stack() {
      TextInput({
        controller: this.inputParams.ui.controller,
        text: this.inputParams.inputText,
        placeholder: this.inputParams.placeholder
      })
        .id(this.inputParams.ui.sign.toString())
        .backgroundColor(Color.Transparent)
        .padding({
          left: this.inputParams.paddingLeft,
          top: this.inputParams.paddingTop,
          right: this.inputParams.paddingTop,
          bottom: this.inputParams.paddingBottom
        })
        .borderRadius(0)
        .showPasswordIcon(false)
        .fontColor(this.inputParams.fontColor)
        .defaultFocus(this.inputParams.ui.focused)
        .style(TextContentStyle.DEFAULT)
        .focusable(true)
        .width('100%')
        .onFocus(() => {
          this.inputParams.ui.focused = true;
          this.inputParams.ui.setFocusedUI();
        })
        .onBlur(() => {
          this.inputParams.ui.focused = false;
          this.inputParams.ui.unsetFocusedUI();
          this.inputParams.ui.sendCustomEvent('blur', Object, 'detail');
        })
        .onChange((value: string) => {
          this.inputParams.inputText = value; // legacy behavior
          this.inputParams.ui.sendCustomEvent('input', {
            value: value,
          } as InputEvent, 'detail');
        })
    }
    .alignContent(Alignment.Center)
    .width('100%')
    .height('100%')
  }
}

@Builder
export function buildInput(ui: UIBase) {
  if (ui as LynxExplorerInput) {
    InputView({ inputParams: (ui as LynxExplorerInput).inputParams });
  }
}

export class LynxExplorerInput extends UIBase {
  controller: TextInputController = new TextInputController();
  inputParams: InputParams = new InputParams(this)
  readonly builder: WrappedBuilder<[UIBase]> = wrapBuilder<[UIBase]>(buildInput);
  focused: boolean = false;

  layout(x: number, y: number, width: number, height: number, paddingLeft: number, paddingTop: number,
    paddingRight: number, paddingBottom: number, marginLeft: number, marginTop: number, marginRight: number,
    marginBottom: number, sticky?: number[] | undefined): void {
    this.inputParams.paddingLeft = this.px2vpIfNeeded(paddingLeft);
    this.inputParams.paddingTop = this.px2vpIfNeeded(paddingTop);
    this.inputParams.paddingRight = this.px2vpIfNeeded(paddingRight);
    this.inputParams.paddingBottom = this.px2vpIfNeeded(paddingBottom);
  }

  static PropSetter: Map<string, Function> = new Map([['value', (ui: LynxExplorerInput, value: Object) => {
    ui.updateInputTextIfNecessary(value, true);
  }], ['placeholder', (ui: LynxExplorerInput, value: Object) => {
    ui.inputParams.placeholder = value as string;
  }], ['text-color', (ui: LynxExplorerInput, value: Object) => {
    ui.inputParams.fontColor = value as string
  }]]);

  update(prop: Record<string, Object>, events?: EventHandlerArray[]): void {
    for (const entry of Object.entries(prop)) {
      LynxExplorerInput.PropSetter.get(entry[0])?.(this, entry[1])
    }
  }

  onNodeReady(): void {
    this.builderNode?.update(this)
  }

  override invokeMethod(method: string, params: Object, callback: (code: number, res: Object) => void): boolean {
    switch (method) {
      case 'focus':
        this.focus(callback);
        break;
      default:
        return false;
    }
    return true;
  }

  updateInputTextIfNecessary(value: Object, valueIsChangedFromLynx: boolean) {
    const strValue = value as string ?? '';
    const mValue = this.inputParams.inputText ?? '';
    if (strValue !== mValue) {
      this.inputParams.inputText = strValue;
    }
  }

  focus(callback: (code: number, res: Object) => void) {
    focusControl.requestFocus(this.sign.toString());
    this.focused = true;
    this.setFocusedUI();
    callback(LynxUIMethodConstants.SUCCESS, new Object);
  }

  override focusChange(focus: boolean, isFocusTransition: boolean) {
    if (!focus) {
      if (!isFocusTransition) {
        this.controller.stopEditing();
      }
      this.focused = false;
    }
  }
}
