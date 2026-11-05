# Element Attribute ID registry

This directory owns the stable numeric identity of public ordinary Lynx
Element attributes. Element typings define the current public names, while
`element_attribute_index.json` preserves protocol history. An allocated ID
never changes and is never reused.

Compatibility annotations such as `@Android`, `@ClayAndroid`, and
`@compatOverride` belong to the Element documentation pipeline. They do not
affect numeric identity and are intentionally ignored here.

## Validate the registry

Run from the repository root through the repository toolchain:

```bash
tools/env.sh python3 lynx/tools/element_attribute_generator/validate.py
```

For an update, also compare against the registry from the merge base:

```bash
tools/env.sh python3 lynx/tools/element_attribute_generator/validate.py \
  --baseline /path/to/previous/element_attribute_index.json
```

To add an attribute, append an active entry using the next ID after the
largest ID ever allocated. Do not fill an old numeric gap. To remove an
attribute, preserve its ID and canonical name and change its state to
`tombstone`.

The validator enforces the following rules:

- IDs and canonical names are unique.
- IDs fit in `uint16` and do not overlap a reserved range.
- Existing ID/name pairs cannot be changed or removed.
- Tombstoned IDs cannot become active again.
- Active names exactly match public ordinary Element attributes.
- Dedicated `id`, `class`, `className`, `style`, event properties, and
  typing-only helpers are excluded.

Language-specific definitions are derived from the registry during their
builds and are not protocol sources.
