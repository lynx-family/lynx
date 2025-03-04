/// <reference types="@lynx-js/rspeedy/client" />

import type * as Lynx from '@lynx-js/types';
declare module 'react' {
  namespace JSX {
    interface IntrinsicElements extends Lynx.IntrinsicElements {
      // The following IntrinsicElements has been declared by both `@types/react` and `@lynx-js/types`.
      // We have to override here to make TypeScript work.
      image: Lynx.IntrinsicElements['image'];
      text: Lynx.IntrinsicElements['text'];
      view: Lynx.IntrinsicElements['view'];
    }
  }
}

type V8Enable = 0 | 1 | 2

declare module '@lynx-js/types' {
  interface GlobalProps {
    appLocale?: string
  }

  interface NativeModules {
    LynxDevToolSetModule: {
      isLynxDebugEnabled?: () => boolean

      isDevToolEnabled?: () => boolean

      isLogBoxEnabled?: () => boolean

      getV8Enabled?: () => V8Enable

      isLongPressMenuEnabled?: () => boolean

      isDomTreeEnabled?: () => boolean

      isQuickjsDebugEnabled?: () => boolean

      isQuickjsCacheEnabled?: () => boolean

      isPixelCopyEnabled?: () => boolean

      isHighlightTouchEnabled?: () => boolean

      isDebugModeEnabled?: () => boolean
      
      switchLynxDebug?: (enable: boolean) => void

      switchDevTool?: (enable: boolean) => void

      switchLogBox?: (enable: boolean) => void

      switchHighlightTouch?: (enable: boolean) => void

      switchPixelCopy?: (enable: boolean) => void

      switchQuickjsCache?: (enable: boolean) => void

      switchV8?: (enable: V8Enable) => void

      switchLongPressMenu?: (enable: boolean) => void

      enableDomTree?: (enable: boolean) => void

      switchQuickjsDebug?: (enable: boolean) => void

      switchDebugModeEnable?: (enable: boolean) => void
    }
  }
}
