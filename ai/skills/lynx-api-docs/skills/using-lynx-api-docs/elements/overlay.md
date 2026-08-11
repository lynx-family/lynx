# Lynx overlay Element

`<overlay>` renders content above the normal Lynx view tree. Use it for modal-like or floating UI that needs its own overlay surface while the authored node stays out of normal layout sizing.

## Basic Usage

```xml
<overlay
  visible="{{showOverlay}}"
  bindshowoverlay="onShow"
  binddismissoverlay="onDismiss"
>
  <view class="panel" />
</overlay>
```

The overlay wrapper itself does not take normal layout space. Its first native child is measured against the screen or window-sized overlay area.

## Core Props

| Prop | Platforms | Description |
| --- | --- | --- |
| `visible` | Android, iOS, Harmony | Shows or hides the overlay |
| `events-pass-through` | Android, iOS, Harmony | Lets events pass through the overlay surface when the platform path allows it |
| `level` | Android, iOS | Controls stacking level |
| `nest-scroll` | Android, iOS | Connects overlay gesture handling to a nested scroll view |
| `ignore-focus` | Android, iOS, Harmony | Avoids focus capture |
| `pointer-events` | iOS, Harmony | Controls pointer hit testing |
| `mode` | iOS, Harmony native path | Chooses iOS mounting behavior and Harmony native dialog level behavior |

## Platform Props

Android:

- `status-bar-translucent`
- `status-bar-translucent-style`
- `cut-out-mode`
- `always-show`
- `android-lazy-init-context`
- `android-full-screen`
- `android-hide-navigation-bar`
- `android-container-popup-tag`
- `android-overlay-scope`: `fragment` ties the overlay to the AndroidX Fragment that owns the
  page's LynxView; the default, `global`, preserves window-level behavior.
- `android-adapt-edge-to-edge`
- `android-navigation-bar-style`: `auto` inherits the host Activity navigation bar and is the default. `light` uses a white background with dark system controls, `dark` uses a black background with light system controls, and `transparent` removes the background while inheriting the host Activity control appearance. Missing and invalid values behave as `auto`.
- `android-set-soft-input-mode`
- `android-native-event-pass`

iOS:

- `allow-pan-gesture`
- `ios-enable-swipe-back`
- `mode`
- `ios-not-adjust-top-margin`
- `ios-not-adjust-left-margin`

Harmony:

- `mode`: `page` embeds the native overlay in the current page; the default, `window`, `top`, unknown strings, and non-string values display it above app pages.

Harmony native overlay handling covers `visible`, `events-pass-through`, and `mode`, with generic focus and pointer behavior inherited from the base UI path. The native level-mode call requires Harmony SDK API version 15 or later; older runtimes keep the platform's existing dialog level behavior. Some Harmony hosts can select a newer overlay-manager path; verify host behavior when relying on pass-through or level-mode details.

## Events

| Event | Platforms | Payload |
| --- | --- | --- |
| `showoverlay` | Android, iOS, Harmony | Android includes `errorCode` and `errorMsg`; iOS and Harmony use empty detail |
| `dismissoverlay` | Android, iOS, Harmony | Empty detail |
| `requestclose` | Android, Harmony | Empty detail |
| `onRequestClose` | iOS | Empty detail |
| `overlaytouch` | Android, iOS | `x`, `y`, `vx`, `vy`, `state` |
| `overlaymoved` | Android, iOS | `x`, `y`, `vx`, `vy`, `state` |

`showoverlay` on Android reports whether the dialog context was valid. Use that payload when debugging failed Android overlay display.

## Usage Notes

- Toggle `visible` for deterministic show and dismiss behavior.
- Use `android-overlay-scope="fragment"` in single-Activity hosts when an overlay must suspend
  with its owning Fragment while still rendering outside the LynxView bounds. The owner must be
  discoverable through `FragmentManager.findFragment(View)`. Resolution failures do not fall back
  to global scope, and `android-container-popup-tag` is ignored in this mode.
- Bind `dismissoverlay` when the page needs to synchronize state after native dismissal.
- Treat Android window/status-bar props as Android-only.
- Treat iOS margin-adjustment props as iOS-only.
- Treat Harmony `mode="page"` as native-path scoped.
- For Harmony, validate behavior in the target host if the host enables its newer overlay path.
