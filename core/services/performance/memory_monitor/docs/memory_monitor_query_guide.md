# Memory Monitor Query Guide

Applies to `schema_version=10` and the memory events below.
Recipes use top-level property filters, grouping, aggregates, and arithmetic
**between aggregate results**. They do not require bitwise operations,
cross-event joins, or custom row-wise expressions.

The question overview is also documented in [memory_monitor.h](../memory_monitor.h).
For full property types and emission conditions, see the
[property reference](./memory_monitor_property_registration.md).

## Event timing and reporting rules

“Reporting time” here means when the monitor constructs an event and hands it to
EventTracker. Queueing, flush, and platform delivery occur afterward. Arrival
time must not be interpreted as the exact observation time.

### Enablement and collection

When monitoring configuration is first initialized, `global_sample_rate`
determines process inclusion. The decision remains fixed for that process.
The default is 0: no monitoring events are produced unless enabled.

Element, MTS, BTS, bundle-local MTS pool, and QuickJS slot tracking follow the
global process decision. Lynx UI is the only source with an additional
process-level draw. The page
event plus `lynxsdk_memory_process` and `lynxsdk_memory_page` emit
`lynx_ui_memory_enabled`; keep it fixed when querying their page-memory totals.

`page_scheduled_sample_rate` separately controls scheduled page-event delivery.
It defaults to 0.1 and is drawn once per process, conditional on global
selection and independently of UI collection. It leaves planned observations,
early-summary statistics/delivery, and global events unchanged.

Allocations, frees, Element/UI updates, and engine memory callbacks refresh
caches without emitting these telemetry events themselves.
`mts_threshold_mb` and `bts_threshold_mb` configure engine cache
updates, not event triggers or forced GC.

### Page event: `lynxsdk_performance_entry_memory_v2`

The page schedule starts when the monitor handles the first completed
`loadBundle` pipeline. This anchor differs from Q3 residency, which starts at
page creation.

| Carrier | Reporting time | Per-instance rule |
| --- | --- | --- |
| `scheduled` | 0, 1, 2, 3, 5, 8, 12, 16, 20, 40, 60, 120, 300, 600, 1800 seconds after the anchor | Selected processes only; the 0s observation is immediate and each planned point is attempted at most once, up to 15 events |
| `scheduled` with `has_early_usage_summary=1` | Normally the 20s observation; if delayed, the first planned callback that actually reaches 20s | Selected processes publish the summary once alongside that point's current-memory fields |
| `early_summary` with `has_early_usage_summary=1` | At the same window boundary when scheduled delivery is disabled, or at monitor destruction if still pending | Summarize saved observations once; no extra measurement or current-memory/offset fields |

Planned-point rules:

- Elapsed time drives the schedule regardless of visibility, foreground state,
  or interaction. A late callback reports its pending point; further elapsed
  deadlines are skipped, not backfilled.
- Logical exit or missing page state stops scheduled observations. There are no
  planned offsets beyond 1800s.
- Reload neither resets the anchor nor affects the schedule or summary.
- `current_page_memory_bytes` is emitted on scheduled rows and sums all
  available page records. Missing MTS/BTS contributions remain absent from
  that sum rather than invalidating the row.
- A page without a first-load anchor has no page schedule or summary, but can
  participate in global accounting and residency.

Early-summary rules:

- The window covers the first 20s after the anchor, shortened to logical exit
  for a short page. Its duration is clamped to 0–20000ms.
- At destruction, Shell's saved exit time bounds the summary when available;
  otherwise destruction time is used. Shell then flushes queued page events.
- Only early planned observations enter the equal-weight total/BTS/MTS means
  and sample peaks. Summary-metric emission requires every due point to have
  been observed.
- Incomplete summaries still publish their summary marker and window but omit
  all means/peaks. They consume the same once-per-instance budget.
- After summary publication, destruction adds no page event. Crashes, OOM, and
  forced termination may bypass destruction, so not every instance necessarily
  delivers a summary.

See the [page design](./page_memory_monitor_reporting_design.md) for details.

### Global snapshot events

The first enabled page, shared-VM, or bundle-local pool registration starts an
**8-second process poll** and installs a memory-pressure listener. Registration
itself does not immediately generate a snapshot; polling generates one only on
a trigger.

| Trigger | Condition | Emitted analysis events |
| --- | --- | --- |
| Timer | `time_interval_sec` deadline; default 300s | `lynxsdk_memory_page`, `lynxsdk_memory_page_residual`, `lynxsdk_memory_vm_stat`, `lynxsdk_memory_mts_pool`, and `report_kind=sample` QuickJS events, when each has an eligible sample |
| PSS | Positive PSS reaches a higher high-water bucket | `lynxsdk_memory_process`; default bucket width is 256MiB |
| Memory warning | Platform pressure level above NONE | Contributes an endpoint to `lynxsdk_memory_persistence`; each notification queues a snapshot |
| QuickJS overflow | First page-slot allocation failure | `lynxsdk_memory_vm_quickjs` with `report_kind=slot_overflow`, once per VM lifetime |

Trigger and collection rules:

- Timer and PSS in one poll share one collected snapshot but produce their
  independently qualified events. The PSS high-water bucket begins at zero, so
  the first poll can cross multiple buckets.
  Peaks between polls may be missed; suspension and reporter congestion delay
  detection.
- Warning and overflow requests queue independently. They do not automatically
  merge into the next timer/PSS snapshot or reset its deadline.
- Disabling timer/PSS does not disable warnings, slot events, or residency.
- Each snapshot independently selects at most one active page, one all-engine
  shared VM, one QuickJS VM, one bundle-local MTS pool, and one confirmed
  positive-residual exited page.
  Page/VM draws are uniform in their respective populations; exited pages use
  positive-byte proportional sampling. An ineligible analysis event is omitted.
- Trigger VM details describe the cause, not a random sample, and can differ
  from the sampled VM.
- Page and QuickJS data use caches; shared V8/JSVM heaps are queried on their
  owning JS threads with a 1s timeout. Failure or timeout suppresses the process
  event and that VM sample's heap fields, while its lifecycle/reuse fields
  remain usable.
- Page/VM creation, exit, destruction, and full-GC callbacks do not themselves
  request additional lifecycle snapshots. Subsequent snapshots read residuals.
  The process poll continues after individual pages exit.

### Residency event: `lynxsdk_memory_page_residency`

Each monitored creation enrolls one 600000ms observation, including pages that
never finish loading. The process poll checks the earliest deadline before
scanning. Due instances are combined into one event per `page_key`.

- Each instance is evaluated once. Duplicate registration/reload does not
  restart its window.
- Early exits wait until the original deadline. Only a completed process
  observation window contributes to `evaluated_page_count`; termination before
  completion contributes neither count.
- `alive_page_count` includes instances whose exit is later than the fixed
  deadline, or that have not exited. Exit exactly at the deadline is not alive.
  A late poll changes delivery time, not the evaluation timestamp.
- Active instances use the key saved at evaluation; exited instances use the
  key saved at exit. Before first load, the URL fallback is used.
- These rows contain identity, key, threshold, and the two counts. They have no
  trigger, PSS, memory, or random-sample details.
  Counts are actual counts in selected processes, without page weights.
- Residency needs no additional heap/PSS query. The same poll may independently
  produce snapshot-derived events. Termination after a deadline but before
  polling/delivery can still lose data.

See the [global design](./memory_monitor_reporting_design.md), and the
[page](../memory_monitor.cc), [global](../global_memory_monitor.cc), and
[sample aggregation](../memory_monitor_data.h) implementations.

## Common query settings

| Setting | Value or rule |
| --- | --- |
| Page event | `lynxsdk_performance_entry_memory_v2` |
| Global events | `lynxsdk_memory_process`, `lynxsdk_memory_page`, `lynxsdk_memory_page_residency`, `lynxsdk_memory_page_residual`, `lynxsdk_memory_persistence`, `lynxsdk_memory_vm_stat`, `lynxsdk_memory_vm_quickjs`, `lynxsdk_memory_mts_pool` |
| Version | `schema_version=10` |
| Cohort | Fix SDK/build, date range, platform, experiment, and source configuration |
| Event deduplication | Provided by the data platform; no monitor-owned deduplication fields |
| Timer cohort | Encoded by production of the page, residual, VM-stat, and sampled QuickJS events |
| Valid PSS | Encoded by production of `lynxsdk_memory_process` |
| Units | Divide bytes by 1048576 for MiB; multiply fractions by 100 for percent |

### Page identity

At the first `OnFirstLoadComplete`, the reporter calls
`GetGenericInfoOrExtraParam(instance_id, "page_id")` once. A nonempty value other
than `unknown` becomes `page_key`; otherwise use the URL fallback. The result is
saved in `InstanceState` and later observations/destruction do not query host
storage again.

`page_id` is a host input, not an output property owned by the monitor.
Multiple instances can share a key.

`InstanceState::SetUrl` preserves the original URL and updates the fallback only
when that URL changes before identity resolution.
[NormalizeMemoryPageUrl](../../memory_page_key.cc) returns the URL unchanged in
this source tree. Host integrations may provide their own stable `page_id`.
An exit before first load uses the saved fallback.

VM names use group IDs for shared QuickJS/JSC and fixed RuntimeManager names
for V8/JSVM; page URL normalization does not apply to VM names.

Bundle-local pools first use their template URL as `page_key`. Before a
snapshot, the monitor finds the lowest active instance ID whose original URL
exactly matches that template URL, resolves the instance's host page ID once,
and freezes it for the pool. Pools without a current match retain the URL
fallback and retry later. `LynxGlobalPool` is excluded by its separate
construction path.

Unless a recipe explicitly specifies a population-wide denominator, numerator
and denominator use identical filters and groups.
If SUM is unavailable, `AVG(field) × non-null count(field)` expresses the same
sum. Do not substitute the event count of an unrelated field.

### Units, grouping, and denominators

Q3 survival is a fraction; its memory subquestion is a byte-snapshot
contribution. Q5/Q6 A/B values are mean bytes. Q6 A alone is cumulative residual
burden in **byte-snapshots**, not one page's size and not Q7 byte-time.

Display A and B separately to check grouping. A UI label such as “event A” does
not establish that B shares the same group. Query B independently with the
same settings and compare each group. A population-wide denominator would
change a group mean into a contribution relative to that population.

For multiple days, combine sums before division; do not average daily A/B.
Display actual rows and independent processes. SUM(weight) estimates population
scale; it is not the number of independent observations.
Identity rules can vary by build, so fix the build or use an explicit key mapping.

### Fixed sampling configuration

`MEMORY_MONITOR_CONFIG` is a compact comma-separated key/value list, for
example:

```text
global_sample_rate,1,lynx_ui_sample_rate,1,page_scheduled_sample_rate,0.1,pss_threshold_mb,256
```

The supported independent keys are `global_sample_rate`,
`lynx_ui_sample_rate`, `page_scheduled_sample_rate`, `time_interval_sec`,
`pss_threshold_mb`, `mts_threshold_mb`, and `bts_threshold_mb`. The format
does not use sections, dots, or `=`. It does not trim whitespace. An incomplete
final pair, unknown key, or invalid value is ignored; a later duplicate key
replaces the earlier value.

`global_sample_rate` controls process inclusion and all non-UI producers.
`lynx_ui_sample_rate` is conditional on that process decision. The resulting
`lynx_ui_memory_enabled` value is emitted on events whose totals can include UI
memory, so UI-inclusive and UI-excluded processes can be queried separately.

`page_scheduled_sample_rate` defaults to 0.1: 0 suppresses scheduled rows and
1 enables them for every globally selected process. Its decision is independent
of `lynx_ui_sample_rate`. Effective inclusion for Q2 scheduled rows is
`global_sample_rate * page_scheduled_sample_rate`; Q1 summaries and global
events retain `global_sample_rate`. Forced monitoring enables all three gates.
Do not infer summary coverage from scheduled-row counts or restrict Q1 to
either carrier.

Within an equal-inclusion-probability cohort, AVG/PCT does not need division by
the process sampling rate. Mixing equally sized populations sampled at 0.01 and
0.1 gives roughly ten times as many events from the latter. N, N/n, V, and
residual weights correct object selection inside a process, not process
selection.

### Available-record accounting

Page memory has one scope:

```text
page memory = sum(all available non-BTS memory records)
            + owned BTS heap or attributable shared QuickJS slot
```

No per-source validity state is emitted. A missing record contributes nothing to
the sum. This keeps every page in the cohort but can underestimate pages whose
MTS is not LepusNG/QuickJS or whose BTS is JSC/shared V8/shared JSVM. Use
`mts_context_type`, `bts_runtime_type`, and `bts_shared` to form comparable
runtime cohorts. Use `lynx_ui_memory_enabled` to choose whether UI is part of the
configured scope.

Component fields are diagnostics and are always numeric. A zero can mean a
measured zero or that no record was available; the runtime/configuration
dimensions define the intended cohort. Shared VM whole heaps remain available
through Q5 and are not copied into each page.

## Q1. Which pages have high memory per instance?

### Q1.1 Early mean memory

Use `lynxsdk_performance_entry_memory_v2`:

```text
has_early_usage_summary = 1
AND early_sample_mean_memory_bytes EXISTS
```

Group by `page_key`, calculate `PCT95(early_sample_mean_memory_bytes)`, and sort
descending. Each instance contributes one mean regardless of duration, sample
count, or event count. Do not restrict `report_kind`: full-window summaries
can use either carrier depending on the scheduled-delivery decision.

- Short-page comparison: add `early_window_target_duration_ms < 5000`.
  A 3-second high-memory page is not scaled by 3/20.
- Complete-window comparison: require target duration=20000 and an emitted mean.
- Quality: compare summaries with an emitted mean against all received summary
  rows after platform deduplication.

A page with 0s/1s values of 100/200MiB that ends at 1.5s contributes 150MiB.
A page ending before 1s with one 240MiB observation contributes 240MiB, without
estimating later memory. Bytes assigned to Unknown or unavailable runtime
sources can reduce the page's value; see Q10 and keep runtime cohorts fixed.

This metric is a distribution of instance means. It is not P95 of all raw
events, the mean of per-offset P95s, or a lifetime peak.

### Q1.2 Early peak memory

Use `lynxsdk_performance_entry_memory_v2`:

```text
has_early_usage_summary = 1
AND early_sample_peak_memory_bytes EXISTS
```

Group by `page_key`, calculate `PCT95(early_sample_peak_memory_bytes)`, and sort
descending. P50/P90/P99 can use the same field. Divide by 1048576 for MiB.
Each instance contributes one maximum across its due early observations at
0, 1, 2, 3, 5, 8, 12, 16, and 20s. Do not restrict `report_kind`; full-window
summaries use either the 20s scheduled row or a standalone `early_summary`.

Keep UI/runtime dimensions and target-duration cohorts fixed as in Q1.1. For
the complete window, require `early_window_target_duration_ms = 20000`.
Short pages contribute the maximum of the planned points before exit; missing
due points omit the peak. Include measured zeros.

This is the peak of sampled caches: transients between points or before the
first-load-completion anchor can be missed. Longer observed windows provide
more opportunities to observe a high value. Later scheduled rows and teardown
values do not enter this peak.

### Q1.3 Early mean BTS memory

Use `lynxsdk_performance_entry_memory_v2`:

```text
has_early_usage_summary = 1
AND early_sample_mean_bts_attributed_bytes EXISTS
```

Group by `page_key`, calculate
`PCT95(early_sample_mean_bts_attributed_bytes)`, and sort descending. Each
instance contributes one equal-weight mean over the same due points as Q1.1.
Keep `bts_runtime_type`, `bts_shared`, and target-duration cohorts fixed.

This is page-attributed BTS memory: a standalone runtime contributes its private
heap, while a tracked shared QuickJS runtime contributes the page slot. Shared
whole-VM heaps are intentionally excluded and remain a Q5 metric. Unsupported or
unattributed shared runtimes contribute numeric zero, so runtime cohorts must not
be mixed.

### Q1.4 Early peak BTS memory

Use `lynxsdk_performance_entry_memory_v2`:

```text
has_early_usage_summary = 1
AND early_sample_peak_bts_attributed_bytes EXISTS
```

Group by `page_key`, calculate
`PCT95(early_sample_peak_bts_attributed_bytes)`, and sort descending. Each
instance contributes one maximum over the same planned observations as Q1.2.
Use the same runtime, sharing, and target-duration cohorts as Q1.3. The value is
a sampled attributed-memory peak, not a continuous peak or a shared VM heap.

### Q1.5 Early mean MTS memory

Use `lynxsdk_performance_entry_memory_v2`:

```text
has_early_usage_summary = 1
AND early_sample_mean_mts_heap_bytes EXISTS
```

Group by `page_key`, calculate `PCT95(early_sample_mean_mts_heap_bytes)`, and
sort descending. Each instance contributes one equal-weight mean over the same
due points as Q1.1. Keep `mts_context_type` and target-duration cohorts fixed.
Unsupported or unavailable MTS records contribute numeric zero, so context
types must not be mixed.

### Q1.6 Early peak MTS memory

Use `lynxsdk_performance_entry_memory_v2`:

```text
has_early_usage_summary = 1
AND early_sample_peak_mts_heap_bytes EXISTS
```

Group by `page_key`, calculate `PCT95(early_sample_peak_mts_heap_bytes)`, and
sort descending. Each instance contributes one maximum over the same planned
observations as Q1.2. Use the same context and target-duration cohorts as Q1.5.
The value is a sampled MTS heap peak, not a continuous peak.

## Q2. Fixed points and source diagnostics

### Q2.1 Page-memory fixed points

Use `lynxsdk_performance_entry_memory_v2`:

```text
report_kind = scheduled
AND scheduled_offset_ms = 5000
```

Group by `page_key` and calculate AVG/PCT95 of `current_page_memory_bytes`.
Change the offset to study a different point; do not mix offsets into an
instance distribution.

Keep `lynx_ui_memory_enabled`, `mts_context_type`, `bts_runtime_type`, and
`bts_shared` fixed as required by the analysis. The total is always emitted,
including zero.

### Q2.2 MTS-only page ranking

Keep the scheduled/offset conditions and require:

```text
mts_context_type = 1
```

Group by `page_key` and `bts_runtime_type`, then calculate AVG/PCT95 of
`current_mts_heap_bytes`.

### Q2.3 BTS-only page ranking

Use the same scheduled/offset conditions and add:

```text
bts_runtime_type = "quickjs"
```

Group by `page_key`, `bts_runtime_type`, and `bts_shared`, then calculate
AVG/PCT95 of `current_bts_attributed_bytes`.

- Standalone QuickJS contributes its page-owned whole heap.
- Shared QuickJS contributes its page slot and requires actual slot tracking
  plus slot data.
- Shared V8/JSVM and JSC have no page-key BTS attribution; query their whole
  heaps by VM name in Q5.
- A page whose architecture explicitly disables BTS contributes zero BTS bytes.

### Q2.4 VM-only fixed-point mean from MTS+BTS

For supported QuickJS MTS/BTS rows:

```text
report_kind = scheduled
AND scheduled_offset_ms = 5000
AND mts_context_type = 1
AND bts_runtime_type = "quickjs"
```

Group by `page_key`, `bts_runtime_type`, and `bts_shared`:

| Metric | Expression |
| --- | --- |
| A | `SUM(current_mts_heap_bytes)` |
| B | `SUM(current_bts_attributed_bytes)` |
| C | Event count after identical filters, or non-null count of `current_mts_heap_bytes` |
| Mean VM-only bytes per instance | `(A+B)/C` |

Because A/B/C use identical rows, `(A+B)/C` equals AVG(MTS+BTS) over those rows.
Shared QuickJS still requires actual slot attribution.

Top-level aggregates cannot derive `PCT95(MTS+BTS)`:
`PCT95(MTS)+PCT95(BTS)` is not the combined-value P95. Without backend row-wise
expression support, name the primary view “Mean VM-only page memory at 5s” and
display separate MTS and BTS P95s.

## Q3. Which pages remain resident after creation?

### Q3.1 Survival within a complete observation window

Event: `lynxsdk_memory_page_residency`.

```text
residency_threshold_ms = 600000
```

Group both aggregates by `page_key`:

| Metric | Expression | Meaning |
| --- | --- | --- |
| A: surviving instances | `SUM(alive_page_count)` | Evaluated instances still logically alive at creation + 10min |
| B: evaluated instances | `SUM(evaluated_page_count)` | Instances whose processes completed the 10min observation |
| Survival fraction | `A/B` | Conditional page-instance survival; display as a percentage |

Each instance is evaluated once. Reload does not add another instance, and
same-key instances due in one batch are combined. Counts are actual counts in
selected processes, not weighted page samples.

An early exit waits until its original deadline before B can increment. If the
process completes the window, it contributes A=0/B=1; if it ends before the
window completes, neither count is contributed. Thus a page that survives every
eligible window has 100% survival regardless of short-process frequency.
Process age exceeding 10min is insufficient: the window begins at this page's
creation.

No timer, presence, or memory-validity filter is needed. All engines and
never-loaded pages participate. Survival does not imply visibility,
interaction, or permanent retention.

The 8-second poll normally adds up to about 8 seconds of detection delay;
suspension can add more. Evaluation uses recorded logical exit time and treats
an exact-deadline exit as not alive. Active keys are read at evaluation; exited
keys were saved at exit.

Show A, B, row count, and independent process count. Do not display a ratio for
B=0. Date filters select report delivery periods, so combine A/B sums across
days. This is conditional instance survival, not user UV or a probability of
remaining forever.

Example: of 100 creations, 60 processes end too early and 40 complete the window.
If all 40 pages survive, the result is 40/40=100%; if 10 of those pages exit
early, it is 30/40=75%.

### Q3.2 Resident-page memory contribution

Event: `lynxsdk_memory_page`.

```text
page_age_ms >= 600000
```

Group by `page_key`:

| Metric | Expression |
| --- | --- |
| A: cumulative resident-memory contribution, descending | `SUM(page_memory_bytes_weighted)` |
| B: observable resident instance-snapshots | `SUM(active_page_count)` |
| Auxiliary mean instance size in MiB | `A/B/1048576` |

The page was selected uniformly from all N active instances. Retain N after
filtering to resident pages. A has units of byte-snapshots and combines instance
count, size, and repeated presence. A/B estimates individual size.

For comparisons across traffic scales, fix the baseline process-snapshot volume
or normalize by all baseline timer snapshots. A is not current global memory.
Shared common overhead and exited residuals are excluded. Keep UI and runtime
cohorts fixed because unavailable MTS/BTS records can lower page values. Do not
use Q3.1's one-time evaluation count as this denominator or add this page view
to whole VM heaps.

## Q4. Which pages consume memory through concurrent instances?

Event: `lynxsdk_memory_page`. No additional base filter is required.

Group by `page_key`:

| Metric | Expression |
| --- | --- |
| A | `SUM(page_group_memory_bytes_weighted)` |
| B | `SUM(active_page_count)` |
| C | `SUM(page_group_selection_weight)` |
| Mean concurrent memory when the key is present | `A/C` |
| Mean concurrent instances | `B/C` |

To focus on multiple instances, apply
`page_group_instance_count>=2` to every metric; use >=4 for at
least four concurrent instances.

Do not filter a whole group by the selected member's engine. Same-key groups can
contain different engines, and that filter changes the selection probability
without changing the recorded group weight.

For N active instances and n members of one key, group selection probability is
n/N and weight is N/n. Ordinary PCT of raw group memory does not correct that
probability. These recipes use directly configurable weighted means.

A high per-instance page score and high concurrent-group cost are different
findings. Many moderate-sized instances can exceed one large instance in total.

## Q5. Which shared VMs are large and long-lived?

### Q5.1 Heap contribution and mean VM size

Event: `lynxsdk_memory_vm_stat`.

```text
vm_age_ms >= 1800000
```

Group by `vm_name` and `vm_runtime_type`:

| Metric | Expression | Meaning |
| --- | --- | --- |
| A: contribution, descending | `SUM(vm_heap_bytes_weighted)` | Corrected heap contribution of same-name/type old VMs, in byte-snapshots |
| B | `SUM(vm_heap_selection_weight)` | Matching inverse selection weights, emitted only with a usable heap |
| Mean heap size | `A/B` | Corrected bytes per VM-snapshot |
| Mean heap size in MiB | `A/B/1048576` | Display value |

One VM is drawn uniformly from all registered shared VMs, including JSC and
missing heaps. Heap values and `vm_heap_selection_weight` are emitted
only when the selected VM has a usable heap for this snapshot. Therefore
`A/B=SUM(h×V)/SUM(V)` over exactly the measurable heap rows.

Filtering does not change the original weight. `shared_vm_heap_bytes_sum` is
the process total and must not be attributed to the selected VM name.
The ratio is an estimate across same-name objects, processes, and times, not one
unique VM, a maximum, a P95, concurrent same-name total, or growth.

Valid zero heaps are included. Missing/unsupported heaps have neither heap value
nor heap denominator and cannot be filled with zero. JSC remains usable for Q9
but cannot enter this heap cohort.
The final mean is not divided by the process sampling rate.

For example, 11,366,260.29 bytes is about 10.84MiB (11.37 decimal MB). A low mean
does not itself demonstrate undermeasurement: a few large heaps may coexist with
many small heaps. The measured engine heap is not page memory, PSS,
reserved virtual space, or all native engine overhead.

V8 reports `used_heap_size()`, JSVM reports `usedHeapSize`, and QuickJS reports
`LEPUS_GetHeapSize()`. QuickJS updates a callback cache; V8/JSVM queries wait
for at most one second during collection. Completeness does not prove cache
freshness.

Age>=30min does not mean idle. To study VMs unused by pages but still retained,
add to both metrics:

```text
vm_active_page_count = 0
AND vm_idle_ms >= 600000
```

Even that cohort does not alone prove a permanent leak.

Possible explanations for a small mean:

- Many small same-name VMs can create a large cumulative A and a small A/B.
- Names pool objects, processes, and times; V8/JSVM RuntimeManager names can
  span business groups. Rare peaks are diluted by ordinary observations.
- Engine heap scope differs from PSS, page memory, reserved space, and
  native overhead.
- A QuickJS cache can miss later growth or overstate memory freed since its
  last update.
- V varies between snapshots, and finite-sample selection error remains.
- A population-wide denominator, mismatched grouping, or different validity
  filters can lower the ratio incorrectly.

Which explanation dominates requires same-cohort data. Display
`vm_heap_source_age_ms` and compare <=60000ms and <=300000ms subsets as
diagnostics. These filters change the population and are not default main-board
conditions. Missing source age is unknown, not zero.

Show A, B, actual rows, independent processes, and raw heap MIN/MAX together.
Within identical valid rows, positive-weight A/B must be between raw MIN/MAX.
Outside that range, inspect grouping, missing fields, and backend aggregation.
MAX is only the largest observed sample.

### Q5.2 Heap size percentiles

Event: `lynxsdk_memory_vm_stat`, with the same age filter as Q5.1:

```text
vm_age_ms >= 1800000
```

Group by `vm_name`, `vm_runtime_type`, and **`vm_selection_weight`**:

| Metric | Expression in MiB |
| --- | --- |
| A: P50 | `PCT50(vm_heap_bytes)/1048576` |
| B: P90 | `PCT90(vm_heap_bytes)/1048576` |
| C: P95 | `PCT95(vm_heap_bytes)/1048576` |
| D: P99 | `PCT99(vm_heap_bytes)/1048576` |

If grouping dimensions are limited, filter `vm_selection_weight` to one exact
integer, such as 1 or 2, then group by name/type. A range such as <=5 is not an
equal-probability stratum. Valid zero is already included. Show actual rows and
independent processes; sparse P99s are unstable.

Within one V stratum, each VM has probability 1/V, so ordinary PCT estimates
that stratum's **VM-snapshot heap distribution**. It is not one observation per
VM lifetime, and cache/quality/process-selection constraints still apply.

Removing V grouping gives the distribution of “one VM per process snapshot”:
each VM in a process with fewer VMs has higher inclusion probability.
It is not a corrected percentile of all VM-snapshots.

Do not take PCT of weighted heap, divide PCT(h×V) by PCT(V), or average stratum
P95s. Pooled percentiles require a weighted quantile using
`vm_selection_weight`; ordinary aggregates and arithmetic between their
results cannot express it. The stratified query needs no additional event fields.

### Q5.3 QuickJS VM residual contribution

Event: `lynxsdk_memory_vm_quickjs`, with:

```text
report_kind = sample
```

Use the emitted retained fields directly; their presence is the producer-side
completeness gate. Group by `vm_name`; this event is QuickJS-only and therefore
omits a redundant runtime-type property:

| Metric | Expression | Meaning |
| --- | --- | --- |
| A: cumulative residual contribution, descending | `SUM(vm_retained_bytes_weighted)` | Attributable VM residual burden in byte-snapshots |
| B: VM-snapshots | `SUM(vm_retained_selection_weight)` | Estimated count for the same valid cohort |
| Mean residual per VM in MiB | `A/B/1048576` | Includes confirmed zero-residual VMs |

Field presence enforces completeness: actual QuickJS tracking; valid slots and
full-GC data strictly after exit for every tracked exited page in that VM;
fewer than 253 allocated page slots; no allocation failure or untracked exit
history.

The raw value sums all qualifying exited-page slots in the VM; the weighted
value multiplies by the number of registered QuickJS VMs. Unknown is omitted
and confirmed zero is emitted.
Field presence already enforces completeness for the selected VM; another VM's
pending pages do not exclude it.

VM-age or cumulative-binding filters may be added to both A/B.
Unknown/Common/Overflow and unattributed bytes are excluded, so this is tracked
residual rather than all permanent VM leakage. It overlaps whole heap and Q6;
do not add those values.

Excluding saturated VMs omits heavily reused objects. Use Q9 capacity and Q10
Overflow views to expose that coverage boundary.

## Q6. Which exited pages retain memory after full GC?

Event: `lynxsdk_memory_page_residual`. Event production already requires a
confirmed positive residual and a positive selection weight.

Group both metrics by `leak_page_key` with identical filters:

| Metric | Expression | Meaning |
| --- | --- | --- |
| A: cumulative residual contribution | `SUM(shared_vm_tracked_exited_page_retained_bytes)` | Corrected same-key residual burden, in byte-snapshots |
| B | `SUM(leak_page_selection_weight)` | Estimated positive-residual page-snapshots |
| Mean post-full-GC positive residual | `A/B` | Corrected bytes per positive-residual page-snapshot |
| Mean in MiB | `A/B/1048576` | Per-page size leaderboard |

Name the mean view “Mean post-full-GC positive residual.” Every selected row
already has a valid full GC captured strictly after page exit. Show A, B, rows,
and independent processes. B=0 means no mean, not zero memory.

### Contribution versus mean

For selected residual L and total confirmed residual T:

```text
p = L/T
leak_page_selection_weight = T/L = 1/p
A/B = SUM(T) / SUM(T/L) = SUM(L * weight) / SUM(weight)
```

T equals L/p. Keep the original probability after filtering by key.
`vm_selection_weight` belongs to an independent draw and cannot be
substituted.

The mean covers confirmed **positive-residual page-snapshots**. It excludes
zeroed, untracked, and pending pages, and it is not one contribution per unique
exited instance. A still-retained page can contribute at many snapshots.
Positive-byte sampling cannot estimate a mean that includes every zeroed page.

A pending page does not invalidate a confirmed sample. T and the PPS candidate
set are constructed only from pages with a strictly post-exit full-GC value.
Therefore T, p=L/T, and T/L remain valid for the **confirmed subset** when
another page is pending. The pending page contributes neither a value nor a
zero. This default Q6 query deliberately estimates that confirmed subset rather
than all tracked exited pages.

For residuals of 90 and 10 bytes, the corrected mean target is 50 bytes.
An ordinary average of inferred `L=T/selection_weight` favors the 90-byte page
because it is more likely to be selected. It cannot replace corrected means or
quantiles.

T is bytes in one event. SUM(T) after selected-key attribution estimates
**cumulative residual byte-snapshot burden** across processes and times.
It combines residual size, number of instances, and repeated observations.
It is neither newly allocated bytes nor memory simultaneously present worldwide.

One eligible exited page retaining 8MiB across 1000 snapshots contributes
8000MiB-snapshots while still having an 8MiB per-snapshot residual.
Likewise, 1000 page-snapshots averaging 1MiB contribute 1000MiB-snapshots, more than
20 averaging 10MiB (200MiB-snapshots), although their mean is smaller.
Use contribution for aggregate burden and mean for individual residual size.
Fix observation scale for contribution comparisons.

### Coverage and interpretation

The query covers tracked QuickJS slots; absence of rows does not establish that
JSC/V8/JSVM have no residuals.
The full-GC timestamp must be strictly later than exit. Equal-millisecond
ordering waits for another full GC. Ordinary updates cannot replace this
confirmed residual snapshot.

Pending pages do not enter total residual or PPS and are not zero. The compact
Q6 event does not expose zero, pending, saturation, untracked, or owning-VM
diagnostics. Absence of a row therefore means there was no eligible positive
sample, not that every exited page released its memory. Tracking cannot recover
disabled, overflow, unsupported-engine, or exited-VM bytes. Valid post-GC
references or tasks can retain slot bytes, so the field prefix `leak` alone
does not prove permanent leakage.

## Q7. Which memory remains occupied over time?

Event: `lynxsdk_memory_persistence`. It is built from **all trigger types**
because PSS/overflow/pressure snapshots split intervals.

| Entity | Filters | Grouping | Metric |
| --- | --- | --- | --- |
| VM | VM area field exists | `vm_name`, `vm_runtime_type` | `SUM(vm_attributed_heap_byte_ms)` |
| Exited page | Residual area field exists | `leak_page_key` | `SUM(leak_page_attributed_byte_ms)` |

Divide by `1048576 × 3600000` for MiB-hours. VM area terms are h×V×valid interval;
residual terms are T×valid interval.

These are endpoint rectangles capped at 5 minutes and clipped to VM creation or
page exit. They are not continuous integrals or current heap × VM age, and must
not be added to instantaneous memory.

Residual endpoints use qualifying full-GC snapshots. The producer omits the
area field for an incomplete residual endpoint, so no separate completeness
filter is needed. Subsequent events do not fill missing intervals. The result
is not a continuous GC-to-GC integral.

## Q8. What is Lynx's share when PSS crosses 1/2/3GiB?

Event: `lynxsdk_memory_process`. For 1GiB:

```text
trigger_pss_previous_milestone_bytes < 1073741824
AND trigger_pss_milestone_bytes >= 1073741824
```

Group by platform and physical RAM metadata. Calculate
`PCT50/PCT90/PCT99(lynx_accounted_process_pss_ratio) × 100`, with sample counts.
Use 2147483648 for 2GiB and 3221225472 for 3GiB.
Do not require equality to a boundary, because one observation may cross
several buckets.

**P99(Lynx bytes)/P99(PSS) is not P99(Lynx bytes/PSS).**

This ratio uses the available-record numerator. Keep
`lynx_ui_memory_enabled` fixed and interpret runtime-specific missing MTS/BTS
memory as underestimation. Small threshold cohorts can validate formulas
without providing a stable production P99.

## Q9. Which VMs repeatedly serve pages?

### Q9.1 Cross-engine reuse

Event: `lynxsdk_memory_vm_stat`. No heap or additional base filter applies.

Group by `vm_name` and `vm_runtime_type`:

| Metric | Expression | Purpose |
| --- | --- | --- |
| A: cumulative binding contribution | `SUM(vm_total_page_count_weighted)` | Combines reuse count and VM-snapshot scale |
| B: VM-snapshots | `SUM(vm_selection_weight)` | Same cohort as A |
| Mean cumulative bindings per VM, descending | `A/B` | How many bindings a VM had served when observed |

QuickJS, JSC, V8, and JSVM participate without heap or slot gates.
Each weighted term is cumulative bindings × V. Add
`vm_total_page_count>=100`, age, or idle conditions to both metrics while
retaining the original V.

For a high-reuse fraction, make A the weight sum above a binding threshold and
B the weight sum for all baseline VMs, then calculate A/B.

The counter records accepted bindings while monitoring is active. Ordinary
property updates and reload within the same VM do not increment it; leaving and
rebinding can. It is not successful loads, JS calls, or lifetime distinct page
IDs. Summing A across snapshots repeats earlier history, and A/B is a
VM-snapshot mean rather than one lifetime observation per VM.

VM names aggregate objects and may span businesses for RuntimeManager engines.
`report_kind=slot_overflow` QuickJS rows describe a different population and
are unsuitable for the cross-engine periodic leaderboard.
Use the same binding threshold in Q5.3 to investigate associated residuals.

### Q9.2 QuickJS first overflow

Event: `lynxsdk_memory_vm_quickjs`, with:

```text
report_kind = slot_overflow
```

Group by `vm_name`. Calculate PCT95 of `vm_age_ms` and AVG of
`vm_total_page_count`.

The event is emitted only for the first failed page-slot allocation in each VM.
`vm_slot_allocated_count` records the successful slot count at that point.
`vm_heap_bytes` is the heap value cached at trigger time and is always present.

These rows describe the triggering VM and use no VM selection weights. Window
censoring and missing delivery prevent treating event-count ratios as complete
lifecycle conversion rates.

## Q10. Which VMs have high Unknown, Common, or Overflow memory?

Event: `lynxsdk_memory_vm_quickjs`, with `report_kind=sample`. Build three
independent leaderboards. These rows are already periodic QuickJS observations
with valid slot data.

For each leaderboard additionally require its raw field to exist, then group by
`vm_name`:

| Category | Percentile input |
| --- | --- |
| Unknown, slot 0 | `vm_unknown_bytes` |
| Common, slot 1 | `vm_common_bytes` |
| Overflow, slot 2 | `vm_overflow_bytes` |

Calculate P50/P90/P95/P99 directly from each raw field and sort the desired
percentile descending. Divide by 1048576 for MiB. No weighted field or
selection denominator is required.

Rules:

- These are current reserved-slot caches, without a post-exit full-GC residual
  gate.
- Missing means unsupported/uncollected, not zero. Preserve measured zero in
  the percentile population; a raw>0 filter changes the population.
- Unknown indicates missing task-slot attribution. Common is shared overhead.
  Overflow contains active and exited bytes after slot exhaustion and is not
  wholly leakage or a single attributable page.
- All three are already included in heap and global accounting; do not add them
  again. JSC/V8/JSVM have no corresponding page-slot measurements.

Available records do not prove correct scheduling attribution. Unknown can be
nonzero while the page's own attributed slot is correspondingly lower.
Fixed version, time, platform, configuration, and process scale are necessary
for percentile comparisons.

## Q11. Which bundle-local MTS pools use the most memory?

Event: `lynxsdk_memory_mts_pool`. Group by `page_key` and
`mts_context_type`. Use identical row filters for each numerator and
denominator:

```text
A = SUM(mts_pool_heap_bytes_weighted)
B = SUM(mts_pool_heap_selection_weight)
C = SUM(mts_pool_runtime_count_weighted)

estimated cumulative pool heap contribution = A
mean heap per physical pool snapshot = A/B
mean runtime count per physical pool snapshot = C/B
```

Rank A to find page keys whose pools contribute the most total heap. Rank A/B
to find individually large physical pools. Compare C/B separately to distinguish
large runtimes from many cached runtimes.

The producer draws one physical pool uniformly from all P bundle-local pools
and emits inverse weight P. All context types and zero heap values are valid;
do not filter `mts_pool_heap_bytes > 0`. A zero is the context getter's measured
value, while event absence means no pool was selected because none existed.

Pool heap is already included in `lynx_accounted_memory_bytes` and
`mts_pool_heap_bytes_sum`. It is not part of page
`current_mts_heap_bytes`, `GlobalMemoryUsage.total.mts_bytes`, or
`GlobalMemoryUsage.total.total_bytes`; never add the pool event to those page
totals.

## Validation principles and implementation references

Validate event-derived queries against independently calculated workload truth.
One path enumerates page/VM lifecycles, memory values, and full-GC timestamps.
The other drives the actual monitors, collects emitted events, and applies only
event-field filters and aggregates. Expected results must not be used to
construct the actual events or choose sample outcomes.

For directly observed distributions, compare group counts, means, and
P50/P90/P95/P99 using an explicit quantile convention such as nearest rank
`ceil(p*N)-1`. For Q3.1, compare exact alive/evaluated counts and the ratio.

Sampled estimates cannot equal full-enumeration truth in every run. Derive
variance independently and fix tolerances before execution, for example:

```text
abs(observed_sum - true_sum) <= 6*sqrt(variance) + floating_point_tolerance
```

Validate numerator and denominator separately, then propagate their error into
the ratio. If the denominator error bound includes zero, validate sums but do
not invent a zero mean. Assertions, rather than printed/XML diagnostic values,
must determine success.

| Query | Key checks |
| --- | --- |
| Q1.1-Q1.6 | One total/BTS/MTS mean and peak per instance, short/full-window distributions, fixed UI/runtime cohorts, early-only peaks, and intended high-memory ranking |
| Q2 | One offset, timing/scope gates, zero values, and runtime-filtered MTS/BTS diagnostics |
| Q3 | Complete creation windows, early exits, exact deadlines, 100% survival for always-resident eligible pages, and separate resident-memory estimates |
| Q4 | Group-weighted bytes/counts and mixed-engine member handling |
| Q5 | Heap contribution/weights/mean; missing and zero heaps; complete VM residuals; percentile query semantics |
| Q6 | Strict post-exit full GC, pending/zero/positive distinctions, matching weighted cohorts |
| Q7 | All-trigger boundaries, caps, entity clipping, and omitted incomplete intervals |
| Q8 | Per-event ratio distributions and crossings spanning multiple buckets |
| Q9 | Cross-engine bindings independently of heap support; capacity versus first failure |
| Q10 | Three independent reserved-slot rankings, zero values, missing tracking, and heap overlap |
| Q11 | Four context types, zero heaps, page-key resolution, process-only accounting, lifecycle cleanup, and matching pool weights |

Directed lifecycle checks should include duplicate creation/load notifications,
reload, never-loaded pages, reused IDs, out-of-order notifications, and stable
observation pointers when pending records move. Residual checks should include
both GC/exit notification orders, equal timestamps, invalid or older samples,
later zeroing, saturation, untracked exit history, and VM cleanup.

A replay validates consistency under its workload model. It does not establish
real producer freshness, background timer behavior, delivery reliability, or
production distributions. A synthetic percentile calculation also does not
prove the analytics backend implements the same grouping/quantile behavior.

Public source references:

- [Question contract and settings](../memory_monitor.h).
- [Page scheduling, identity, and summaries](../memory_monitor.cc).
- [Global triggers, residency, accounting, and selection](../global_memory_monitor.cc).
- [Accounting, planned points, and reservoir](../memory_monitor_data.h).
- [Event payload types](../../../event_report/event_tracker.h).
- [Event queueing and flush](../../../event_report/event_tracker.cc).
