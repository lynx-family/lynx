// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import {
  Behavior,
  BehaviorRegistryMap,
  LynxContext,
  LynxExtraTiming,
  LynxGenericResourceFetcher,
  LynxMediaResourceFetcher,
  LynxTemplateResourceFetcher,
  LynxView,
  MetaData,
  ModuleClassWrapper,
  TemplateData
} from '@lynx/lynx';
import { EntryLynxClient } from '../client/EntryLynxClient';
import { ExplorerModule } from '../module/ExplorerModule';
import { ExampleGenericResourceFetcher } from '../provider/ExampleGenericResourceFetcher';
import { ExampleMediaResourceFetcher } from '../provider/ExampleMediaResourceFetcher';
import { ExampleTemplateResourceFetcher } from '../provider/ExampleTemplateResourceFetcher';
import { UIClickView } from '../component/UIClickView';
import { ClickShadowNode } from '../component/UIClickShadowNode';
import { LynxExplorerInput } from '../component/LynxExplorerInput';
import { ConfigurationConstant } from '@kit.AbilityKit';


@Entry
@Component
struct Index {
  @StorageProp('currentColorMode') @Watch('onColorModeChange') currentColorMode: ConfigurationConstant.ColorMode =
    AppStorage.get<ConfigurationConstant.ColorMode>('currentColorMode')!;
  private genericResourceFetcher: LynxGenericResourceFetcher = new ExampleGenericResourceFetcher();
  private mediaResourceFetcher: LynxMediaResourceFetcher = new ExampleMediaResourceFetcher();
  private templateResourceFetcher: LynxTemplateResourceFetcher = new ExampleTemplateResourceFetcher();
  private modules: Map<string, ModuleClassWrapper> = new Map();
  private url: string = 'main.lynx.bundle';
  private behaviors: BehaviorRegistryMap = new Map([['click-view',
    new Behavior(UIClickView, ClickShadowNode, "this is a test custom data")],
    ['explorer-input', new Behavior(LynxExplorerInput, undefined)]]);
  private metaData = new MetaData(
    new TemplateData('{"text":"TD"}'),
    new TemplateData({ "theme": "Light" }));
  private clients: EntryLynxClient[] = [new EntryLynxClient()];
  private extraTiming: LynxExtraTiming = new LynxExtraTiming();
  private lynxContext?: LynxContext;

  onColorModeChange(): void {
    if (this.currentColorMode === ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT) {
      this.lynxContext?.updateMetaData(new MetaData(
        undefined,
        new TemplateData({ "theme": "Light" })));
    } else if (this.currentColorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK) {
      this.lynxContext?.updateMetaData(new MetaData(
        undefined,
        new TemplateData({ "theme": "Dark" })));
    }
  }

  aboutToAppear() {
    this.metaData.globalProps?.merge({
      "theme": this.currentColorMode === ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT ? "Light" : "Dark"
    });
    // mock extraTiming here
    this.extraTiming.openTime = new Date().getTime();
    this.extraTiming.containerInitStart = new Date().getTime();
    this.extraTiming.containerInitEnd = new Date().getTime();
    this.extraTiming.prepareTemplateStart = new Date().getTime();
    this.extraTiming.prepareTemplateEnd = new Date().getTime();
    this.modules.set('ExplorerModule', {
      moduleClass: ExplorerModule,
    });
    this.metaData.templateData?.merge('{"text":"string"}');
  }

  build() {
    Column() {
      LynxView({
        clients: this.clients,
        url: this.url,
        metaData: this.metaData,
        genericResourceFetcher: this.genericResourceFetcher,
        mediaResourceFetcher: this.mediaResourceFetcher,
        templateResourceFetcher: this.templateResourceFetcher,
        modules: this.modules,
        behaviors: this.behaviors,
        onCreate: (context: LynxContext) => {
          this.lynxContext = context;
          context.sendGlobalEvent('viewAppear', [true]);
          context.setExtraTiming(this.extraTiming);
        }
      }).width('100%').height('100%');
    }
    .size({ width: '100%', height: '100%' })
  }
}
