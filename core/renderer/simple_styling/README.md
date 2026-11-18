# SimpleStyle direction

Declare `direction` in the node's first resolved style batch. Its effective
presence is fixed for the node's lifetime; later batches may change its value.
For example, start with `direction: ltr` and update it to `direction: lynx-rtl`.
Keep the property declared when toggling between the two values.

With property-based SimpleStyle, presence is checked after resolving static
styles and dynamic overrides.
Removing a dynamic override may reveal a static direction value: the effective
property remains declared, so this is a value change. Adding direction to an
already rendered node is rejected with a diagnostic. Style producers must keep
the final declaration present; removing it is outside the SimpleStyle contract.
Internal resets retain the recorded direction until a new value is consumed,
without tracking a pending deletion.

Direction is recorded only as the property is consumed. Direction changes run
once after the complete style batch is resolved, using the recorded value.
Each current style source is visited once to apply its new horizontal mapping.
Only old physical properties unused by the new result are then cleared, so
swapping two occupied edges needs no reset. Local bitsets track the positions
without heap allocation. Property-based SimpleStyle reuses its existing maps;
object-based SimpleStyle reads the current decoded StyleObjects directly,
without copying their properties into an additional map. The object-based path
retains its existing non-overlapping StyleObject requirement. No style source
is scanned for remapping when direction is unchanged.

Only nodes with an explicit direction declaration convert
direction-related properties. Nodes without that declaration use the original
property IDs. Logical horizontal properties require an explicit direction,
including for LTR layout.

These restrictions apply to SimpleStyle. Ordinary CSS direction handling is
unchanged.
