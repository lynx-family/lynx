# Memory Monitor Event Property Reference

Applies to `schema_version=11`. The page event keeps its `_v2` name. The global
snapshot has been split into analysis-specific events whose names intentionally
have no `_v2` suffix.

Types reflect the actual `MoveOnlyEvent::SetProps` payload: `string`, `int32`,
and `double`. Boolean properties are `int32` values of 0 or 1. C++ 64-bit
integers are reported as `double`. Missing conditional properties mean that the
event does not provide that metric; do not replace them with zero.

All global events contain:

| Property | Type | Description |
| --- | --- | --- |
| `schema_version` | int32 | Event schema version, currently 11. |

Global-event deduplication is provided by the data platform rather than
monitor-owned properties.

## `lynxsdk_memory_process` - 13 properties

Produced only for positive PSS with no V8/JSVM heap-query failure or timeout.

| Property | Type | Description |
| --- | --- | --- |
| `lynx_ui_memory_enabled` | int32 | Whether Lynx UI memory collection is enabled for this process. |
| `process_pss_bytes` | double | Process PSS after heap collection. |
| `lynx_accounted_memory_bytes` | double | Available active-page memory plus deduplicated measurable shared heaps and bundle-local MTS pool heaps. |
| `active_page_count` | double | Registered non-exited page count. |
| `shared_vm_count` | double | Registered shared VMs, including JSC. |
| `shared_vm_heap_bytes_sum` | double | Sum of usable shared heaps. |
| `mts_pool_count` | double | Registered bundle-local MTS pool count. |
| `mts_pool_runtime_count` | double | Total runtimes currently owned by those pools. |
| `mts_pool_heap_bytes_sum` | double | Sum of reported bundle-local pool heaps, including zero-valued pools. |
| `shared_vm_tracked_exited_page_retained_bytes` | double | Confirmed tracked residual total. |
| `trigger_pss_previous_milestone_bytes` | double | Previous PSS high-water boundary; PSS-trigger rows only. |
| `trigger_pss_milestone_bytes` | double | New PSS high-water boundary; PSS-trigger rows only. |

Process memory ratios are calculated in queries by dividing the relevant byte
field by `process_pss_bytes` from the same event. For ratio percentiles, perform
this division on each row before taking the percentile.

## `lynxsdk_memory_page` - 14 properties

Produced only for timer snapshots with at least one active page. One active
instance is selected uniformly from N instances.

| Property | Type | Description |
| --- | --- | --- |
| `lynx_ui_memory_enabled` | int32 | Whether page totals include Lynx UI memory. |
| `active_page_count` | double | N, also the selected page's inverse probability. |
| `page_age_ms` | double | Selected page age from creation. |
| `page_memory_bytes` | double | Selected page's available-record total. |
| `page_vm_shared` | int32 | Whether the selected page's BTS VM is shared. |
| `page_vm_runtime_type` | string | Selected page's BTS engine. |
| `page_memory_bytes_weighted` | double | Selected page bytes multiplied by N. |
| `page_key` | string | Host page ID or URL fallback. |
| `page_group_instance_count` | double | Active same-key instance count n. |
| `page_group_memory_bytes` | double | Same-key memory sum S. |
| `page_group_max_instance_bytes` | double | Largest same-key instance. |
| `page_group_selection_weight` | double | N/n. |
| `page_group_memory_bytes_weighted` | double | S multiplied by N/n. |

## `lynxsdk_memory_page_residency` - 5 properties

| Property | Type | Description |
| --- | --- | --- |
| `page_key` | string | Page identity for this due cohort. |
| `residency_threshold_ms` | double | Fixed 600000ms creation-based window. |
| `evaluated_page_count` | double | Instances whose process completed the window. |
| `alive_page_count` | double | Evaluated instances alive at the deadline. |

## `lynxsdk_memory_page_residual` - 5 properties

Produced only for timer snapshots with a confirmed positive-residual page
selected by probability proportional to bytes.

| Property | Type | Description |
| --- | --- | --- |
| `shared_vm_tracked_exited_page_retained_bytes` | double | Confirmed residual total T. |
| `leak_page_key` | string | Selected exited page key. |
| `leak_page_selection_weight` | double | Inverse PPS probability T/L. |
| `leak_page_vm_post_exit_page_count` | double | Accepted bindings to the selected page's BTS VM between its logical exit and residual selection; includes untracked pages and incomplete loads, excludes repeated updates/reload on the same binding. Present even when zero. |

## `lynxsdk_memory_persistence` - up to 6 properties

Produced at a snapshot when VM endpoint area or unreported residual area
exists; also produced with residual fields only when a VM is destroyed.

| Property | Type | Description |
| --- | --- | --- |
| `vm_name` | string | Selected all-engine VM name; present with VM area. |
| `vm_runtime_type` | string | Selected VM engine. |
| `vm_attributed_heap_byte_ms` | double | Q7.1: heap endpoint area H x V x clipped interval. |
| `leak_page_key` | string | Page key drawn independently by positive unreported residual integral. |
| `leak_page_attributed_byte_ms` | double | Q7.2: total pending slot byte-ms in this draw, equivalent to selected area / selection probability. Closed full-GC segments plus the final segment at VM destruction; consumed once, no 5-minute cap. |

A now-zero slot can carry historical area. Residual field presence does not
require every exited page to have completed a post-exit GC. Schema 10 used a
snapshot-endpoint residual estimate; do not merge those Q7.2 semantics.

## `lynxsdk_memory_vm_stat` - up to 27 properties

Produced only for timer snapshots with a uniform all-engine shared-VM sample.

| Property | Type | Description |
| --- | --- | --- |
| `vm_name` | string | Stable VM grouping name. |
| `vm_runtime_type` | string | `quickjs`, `v8`, `jsvm`, or `jsc`. |
| `vm_age_ms` | double | VM age at snapshot start. |
| `vm_idle_ms` | double | Time with no bound active pages, or zero. |
| `vm_active_page_count` | double | Currently bound active pages. |
| `vm_total_page_count` | double | Cumulative accepted page-to-VM bindings. |
| `vm_total_page_count_weighted` | double | Binding count multiplied by all-engine weight V. |
| `vm_selection_weight` | double | All-engine inverse selection probability V. |
| `vm_heap_bytes` | double | Usable selected heap, including valid zero. |
| `vm_heap_bytes_weighted` | double | Heap multiplied by V. |
| `vm_heap_selection_weight` | double | Heap denominator, emitted exactly with heap fields. |
| `vm_heap_source_age_ms` | double | Age of the heap measurement when known. |

Q12 adds the following 14 optional properties when the selected VM has at
least one observed cache creator. There is no heap gate. Counts cover observed
insertions over the VM lifetime, not the current cache contents. Missing
fields do not establish a measured empty cache. Each VM retains at most 128
creating app IDs across both cache types, and each JS sidecar also admits at
most 128 creators. Counters and weights cover admitted creators only; later
creators are not recovered by weighting.

All counter fields below include their selection weights. The `page` segment
identifies the selected creating app; other counters describe the sampled VM.

| Property | Type | Description |
| --- | --- | --- |
| `js_cache_page_key` | string | Selected creating app's page key, preserved after exit; never the hitting app's key. |
| `js_cache_page_weight` | double | V × C, with C distinct observed creating app IDs in this VM. |
| `js_cache_factory_inserted` | double | Lifetime factory insertions × V. |
| `js_cache_factory_reused` | double | Inserted factory entries looked up again at least once × V. |
| `js_cache_factory_cross_app` | double | Inserted factory entries looked up by another app at least once × V. |
| `js_cache_script_inserted` | double | Lifetime loadScript cache insertions × V. |
| `js_cache_script_reused` | double | Inserted loadScript entries looked up again at least once × V. |
| `js_cache_script_cross_app` | double | Inserted loadScript entries looked up by another app at least once × V. |
| `js_cache_page_factory_inserted` | double | Selected creator's factory insertions × V × C. |
| `js_cache_page_factory_reused` | double | Selected creator's reused factory entries × V × C. |
| `js_cache_page_factory_cross_app` | double | Selected creator's cross-app reused factory entries × V × C. |
| `js_cache_page_script_inserted` | double | Selected creator's loadScript insertions × V × C. |
| `js_cache_page_script_reused` | double | Selected creator's reused loadScript entries × V × C. |
| `js_cache_page_script_cross_app` | double | Selected creator's cross-app reused loadScript entries × V × C. |

First reuse may also be the first cross-app reuse, incrementing both counters.
Deleting/replacing a cache entry or closing a realm does not subtract history.
Native app IDs and module keys are not emitted. Same-key creators remain
separate sampling units; aggregate their weighted rows by page key.

## `lynxsdk_memory_vm_quickjs` - up to 14 properties

Every row has one `report_kind`. Timer snapshots produce `sample` rows using a
uniform QuickJS-only draw. A first allocation failure produces a separate
`slot_overflow` row, including when the same snapshot also produces a sample.

| Property | Type | Description |
| --- | --- | --- |
| `report_kind` | string | `sample` or `slot_overflow`. |
| `vm_name` | string | Selected or triggering QuickJS VM name. |
| `vm_age_ms` | double | VM age at report collection. |
| `vm_total_page_count` | double | Cumulative accepted bindings. |
| `vm_slot_allocated_count` | double | Successful page-slot allocations. |
| `vm_untracked_page_count` | double | Failed page-slot allocations. |
| `vm_heap_bytes` | double | Current sample heap or captured trigger-time heap; always emitted. |
| `vm_unknown_bytes` | double | Current Unknown-slot bytes; `sample` only. |
| `vm_common_bytes` | double | Current Common-slot bytes; `sample` only. |
| `vm_overflow_bytes` | double | Current Overflow-slot bytes; `sample` only. |
| `vm_retained_bytes` | double | Complete exited-page residual total, including zero; `sample` only. |
| `vm_retained_bytes_weighted` | double | Complete residual multiplied by Q; `sample` only. |
| `vm_retained_selection_weight` | double | Q5.3 denominator, emitted with retained bytes; `sample` only. |

## `lynxsdk_memory_mts_pool` - 10 properties

Produced only for timer snapshots with at least one bundle-local pool. One
physical pool is selected uniformly. `LynxGlobalPool` is excluded by its
separate construction path. Every context type and valid zero heap remains
eligible.

| Property | Type | Description |
| --- | --- | --- |
| `page_key` | string | Host page ID from one exact template-URL match, or the template URL fallback. |
| `mts_context_type` | int32 | Selected pool context type. |
| `mts_pool_age_ms` | double | Selected pool age at snapshot start. |
| `mts_pool_runtime_count` | double | Runtimes currently owned by the selected pool. |
| `mts_pool_runtime_count_weighted` | double | Runtime count multiplied by physical pool count P. |
| `mts_pool_selection_weight` | double | General inverse selection probability P. |
| `mts_pool_heap_bytes` | double | Reported selected-pool heap, including zero. |
| `mts_pool_heap_bytes_weighted` | double | Heap multiplied by P. |
| `mts_pool_heap_selection_weight` | double | Heap denominator P, always emitted with heap fields. |

## Page event: `lynxsdk_performance_entry_memory_v2` - 21 fixed properties plus UI categories

Page events have two carriers: `scheduled` observations and an `early_summary`
emitted at destruction if the summary is still pending. Scheduled delivery
requires a separate process draw using `page_scheduled_sample_rate` (default
0.1), conditional on global selection and independent of UI collection. If
scheduled delivery is disabled, the early-window boundary instead emits a
standalone `early_summary`. Planned points and early statistics are unchanged.

| Property | Type | Description |
| --- | --- | --- |
| `schema_version` | int32 | Event schema version, currently 11. |
| `lynx_ui_memory_enabled` | int32 | Whether Lynx UI memory collection is enabled. |
| `page_key` | string | Host page ID or URL fallback. |
| `mts_context_type` | int32 | MTS runtime context. |
| `bts_runtime_type` | string | BTS engine type. |
| `bts_shared` | int32 | Whether BTS is shared. |
| `report_kind` | string | `scheduled` or `early_summary`. |
| `scheduled_offset_ms` | double | Planned observation offset; scheduled rows only. |
| `current_element_bytes` | double | Current Element memory. |
| `current_mts_heap_bytes` | double | Current measured MTS heap. |
| `current_bts_attributed_bytes` | double | Current attributable BTS memory. |
| `current_platform_ui_bytes` | double | Current Lynx UI memory. |
| `current_page_memory_bytes` | double | Sum of all available page records. |
| `has_early_usage_summary` | int32 | Whether the row carries the unique early summary. |
| `early_window_target_duration_ms` | double | Summary target duration, clamped to 0-20000ms. |
| `early_sample_mean_memory_bytes` | double | Q1.1 equal-weight mean of due early observations when complete. |
| `early_sample_peak_memory_bytes` | double | Q1.2 maximum of due early observations at 0/1/2/3/5/8/12/16/20s when complete. |
| `early_sample_mean_bts_attributed_bytes` | double | Q1.3 equal-weight mean of page-attributed BTS bytes over the same complete early observations. |
| `early_sample_peak_bts_attributed_bytes` | double | Q1.4 maximum page-attributed BTS bytes over the same complete early observations. |
| `early_sample_mean_mts_heap_bytes` | double | Q1.5 equal-weight mean of MTS heap bytes over the same complete early observations. |
| `early_sample_peak_mts_heap_bytes` | double | Q1.6 maximum MTS heap bytes over the same complete early observations. |

When `lynx_ui_memory_enabled=1`, scheduled rows also contain one dynamic `double`
property per UI memory record: replace `-` with `_` in its category and append
`SizeBytes`. For example, `x-viewpager-ng` becomes `x_viewpager_ngSizeBytes`.
The value is the record's cached byte size, including zero. Element, MTS, and
BTS records do not generate these properties. Saved end summaries omit them.
These UI details overlap `current_platform_ui_bytes`; do not add them to the
page total again.

## Implementation references

See [global event construction](../global_memory_monitor.cc), [page event
construction](../memory_monitor.cc), and the
[query guide](./memory_monitor_query_guide.md).
