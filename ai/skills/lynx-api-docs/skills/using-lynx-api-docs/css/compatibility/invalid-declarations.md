# Invalid declarations and the cascade

When styles are parsed directly into a `StyleMap`, a property handler can
reject an invalid value without overwriting an earlier valid parsed value.

The deferred `RawStyleMap` path stores only one raw value per property ID. If
compilation or style merging has already replaced an earlier valid declaration
with a later invalid declaration, runtime parsing can reject the invalid value
but cannot recover the discarded declaration.

```css
.item {
  flex-shrink: 1;
  flex-shrink: -1; /* invalid */
}
```

Web CSS retains the valid `1`. This package baseline rejects the negative
longhand value, but deferred raw style data might no longer contain the earlier
declaration. The same risk applies to other handler-rejected values and merged
style maps, including important data.

Validate generated values before bundle compilation and emit only the final
valid declaration for each property.
