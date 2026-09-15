# Harmony input keyboard avoidance

`x-input-ng` and `x-textarea-ng` support `set-soft-input-mode`.

| Value | ArkUI page mode |
| --- | --- |
| `nothing` | `KeyboardAvoidMode.NONE` |
| `pan` | `KeyboardAvoidMode.OFFSET` |
| `resize` | `KeyboardAvoidMode.RESIZE` |
| `unspecified`, empty or unrecognized strings | `KeyboardAvoidMode.OFFSET` (the platform default) |

Omitting the property leaves the host configuration unchanged. Non-string values
are ignored. Empty and unrecognized strings use the same fallback as `unspecified`,
matching Android's string parsing policy. Explicit string values take effect when
applied, including updates.
The setting affects every view sharing the owning UIContext. It is not scoped to
one input, and is not automatically restored on blur or removal. The last explicit
string setting wins; hosts that reuse the UIContext should restore their desired
mode when leaving the page. Avoid assigning conflicting modes to sibling inputs.

This is separate from `avoid-keyboard`, which controls Lynx's own translation.
To let Lynx exclusively manage avoidance, use `set-soft-input-mode="nothing"`
and configure `avoid-keyboard` on each input. To disable both layers:

```html
<x-textarea-ng set-soft-input-mode="nothing" avoid-keyboard="{{false}}" />
```

The attribute requires the updated Harmony native runtime and API 14 or later.
The native implementation calls the existing UIContext API through N-API.
It does not affect Android or iOS.
