# Page Memory Reporting Design

This document describes `schema_version=10` of `lynxsdk_performance_entry_memory_v2`.
Page observations aggregate cached values without querying producers on demand.
Their schedule does not depend on app foreground state or page visibility.
The primary metric is an equal-weight mean of planned observations, published
once per instance (Q1.1). The same summary provides total-memory,
page-attributed BTS, and MTS means/peaks (Q1.2-Q1.6). Destruction publishes a
pending summary without measuring memory again.

Related documents:

- [Global reporting design](./memory_monitor_reporting_design.md): resident
  pages, shared VMs, bundle-local MTS pools, exited-page residuals, and
  concurrent same-key instances.
- [Query guide](./memory_monitor_query_guide.md): complete query recipes.
- [Property reference](./memory_monitor_property_registration.md): property
  types and conditional emission rules.
- [MemoryMonitor](../memory_monitor.h): monitoring questions and interfaces.

## 1. Goals and primary metric

The page event answers which page keys have high memory per instance. The global
event answers which objects keep contributing to process memory and how much
concurrent instances of the same key consume together. The two event types have
separate schedules and reporting rules.

The page event combines fixed observation points with one summary per instance:

```text
window = [first-load-completion anchor, min(exit, anchor + 20s)]
M = sum of observed page-memory values at due planned points
    / number of observed planned points
page memory score in MiB = PCT95(M) / 1048576
```

Each instance contributes one M, with no weighting by lifetime, event count, or
number of observations. A page ending at 1.5s uses only the 0s and 1s
observations. If both are 300MiB, it contributes 300MiB: do not scale by
1.5/20, fill the remaining window with zeros, or estimate memory at exit.
A page lasting less than 1 second can contribute its valid anchor observation;
its target duration identifies that short-page cohort. The complete nine-point
cohort provides a full-window comparison using the same metric.

Name the main leaderboard **“P95 of early planned page-memory means.”** It is
a low-cost screening metric based on available caches, not a real-time curve,
lifetime peak, exclusive PSS, or post-GC live memory. Page events do not collect
process PSS. Cache freshness and measurement error require separate validation.

## 2. Anchor and fixed schedule

### 2.1 First-load completion

When [PerformanceController::OnPerformanceEvent](../../performance_controller.cc)
first handles a completion entry with `entryType=pipeline` and `name=loadBundle`,
it calls `MemoryMonitor::OnFirstLoadComplete()`.

```text
anchor = monotonic time when the report thread first handles that entry
```

This is the observation schedule's origin, not the actual paint completion time.
The anchor remains local. A late cache observation must not be backdated to an
earlier rendering timestamp.

The entry and name must exist and have the expected types. Duplicate completion
notifications are ignored. Actual FMP, later update pipelines, image completion,
and GC do not restart the schedule or add another 0s point. The hook is invoked
when the entry is handled, independently of whether the pipeline event is
ultimately sampled for delivery.

Reload does not create a new window, renew summary eligibility, or invalidate
the existing observations.

Global residency uses **page creation + 10 minutes**, independently of this
first-load anchor. Pages that never finish loading can still participate in
global residency.

### 2.2 Planned points and event budget

Offsets from the anchor:

```text
early: 0, 1, 2, 3, 5, 8, 12, 16, 20 seconds
late:  40, 60, 120, 300, 600, 1800 seconds
```

| Event or summary | Limit per instance |
| --- | --- |
| `scheduled` | 15 events in processes selected by `page_scheduled_sample_rate`; none otherwise |
| `early_summary` | At most one at the early-window boundary when scheduled delivery is disabled, or at destruction if still pending |
| Primary early summary | One, attached to a scheduled event or carried by `early_summary` |
| Total events | At most 15; a short page emits its observed points plus a pending summary |

`page_scheduled_sample_rate` defaults to 0.1. Like the UI draw, it selects
processes once at settings initialization, conditional on global selection;
the two draws are independent. It gates construction and delivery of scheduled
rows, not the observation plan or any of the three early accumulators. All
globally selected processes still deliver one early summary when possible.
Forced monitoring enables scheduled delivery as well.

A selected page ending at 1.5s emits 0s, 1s, and `early_summary`: three events,
with no new observation in the final one. Without scheduled delivery, that page
emits only `early_summary`. A complete 20s window also emits a standalone
`early_summary` at the window boundary when scheduled delivery is disabled.
Once the summary has been published, destruction adds no page event. There is
no automatic sixteenth exit event.
After 1800s, scheduled page observations end while caches continue to update;
the global monitor provides long-term observations.

The deadline is always `anchor + offset`, rather than the preceding callback
completion plus a delay. `MemoryMonitor` owns a one-shot timer, advances it to
the next deadline, and stops it on destruction. Reporter-thread confinement and
timer cancellation prevent callbacks from using a destroyed monitor. A single
publication flag resolves races between the 20s observation and destruction.

### 2.3 Delayed and missed points

The schedule uses monotonic time. Visibility and show/hide notifications neither
mean page exit nor reset the anchor.

If suspension or reporter congestion delays a timer, the pending point is
reported with its planned `scheduled_offset_ms`. Further deadlines that have
already elapsed are skipped. One current cache value must not be copied into
several missed historical points.

Summary emission depends only on whether every due planned point was observed:

| Rule | Limit |
| --- | --- |
| Required planned-point coverage | 100% |

The monitor does not report or reject scheduling delay. These observations still
use cached producer values and do not prove that every source measured memory at
the exact planned time.

## 3. Accounting and engine capabilities

### 3.1 Reusing `memory_records_`

`MemoryMonitor` sums non-BTS record categories and ignores `detail_` for flat
telemetry aggregation. It then obtains BTS attribution independently:

```text
bts = GetInstanceBtsMemoryUsage(instance_id)
if bts == max(size_t):
    bts = owned-VM heap from kCategoryBTSEngine

page_accounted = sum(non-BTS record.size_bytes_) + bts
```

A BTS record need not exist before a shared slot is consulted. A missing shared
VM returns the sentinel and selects the standalone record path. RuntimeManager
owns creation and binding order.

When a shared BTS record exists, refresh replaces its VM-heap value with the
page's current attributed slot value. This keeps later
`ReportMemoryForTrace()` iteration source-agnostic: the cached BTS record
already represents this page rather than the whole shared VM.

An owned VM without a BTS record, or a JSC VM without heap support, contributes
no BTS bytes to the page total. Producers must provide balanced allocation
deltas and values representable by the record types.

Source callbacks update only per-category records and do not aggregate the page
total. Scheduled page observations and the initial anchor call
`RefreshMemoryUsage`; an actual Trace emission may also refresh after confirming
that tracing is active and the 16ms throttle permits an event. Each refresh
updates `InstanceState::memory_usage` and the BTS component already included in
that total. Before a global snapshot, every active page with a live
`MemoryMonitor` refreshes from its current category records. Pages without one
retain the existing aggregate while their shared BTS component is recomposed
from current slot caches, so shared heaps remain deduplicated without querying
QuickJS on demand.

Once the page has logically exited or its registry entry is gone, further
scheduled observations stop. Summary delivery does not execute this accounting
path or read the global registry.

### 3.2 Single page-memory scope

```text
Owned BTS:
  page memory = Element + MTS heap + BTS heap + platform UI

Attributable shared QuickJS:
  page memory = Element + MTS heap + this page's slot + platform UI
```

Do not assign a shared heap to every page or divide it evenly among active
pages. Common, Unknown, and Overflow bytes cannot be attributed precisely to
one page instance.

| Scenario | Page BTS value | Page total |
| --- | --- | --- |
| Standalone QuickJS with a heap record | Owned heap | Includes the owned heap |
| Shared QuickJS with a valid sampled slot | Slot bytes, including measured zero | Includes the slot |
| Shared QuickJS with tracking unavailable or an overflow page | Unavailable | Omits BTS |
| Shared V8/JSVM | Unavailable per page | Omits BTS; whole heap remains in the global event |
| JSC | Unavailable | Omits BTS |
| Architecture explicitly disables BTS | Not applicable | Omits BTS |

LepusNG/QuickJS MTS heaps belong to their pages. Other MTS context types have no
measured MTS heap in this monitor, so their page totals omit that contribution.
Runtimes still owned by a bundle-local `MTSRuntimePool` are not page instances.
Their reported heaps appear only in `lynxsdk_memory_mts_pool` and process
accounting; they never contribute to page `current_mts_heap_bytes`,
`GlobalMemoryUsage.total.mts_bytes`, or page `total_bytes`.
Use `mts_context_type`, `bts_runtime_type`, and `bts_shared` to define comparable
cohorts.

SDK-accounted bytes have coverage limits for shared images, textures, global
caches, and platform estimates. Their sum need not equal physical resident
pages.

## 4. Configuration and freshness

### 4.1 Observation time versus measurement time

Aggregation time is when the report thread combines caches. Producer measurement
time is when each source actually reads its memory. Reading a cache does not
update its true measurement time.

Identical values at multiple planned points mean that the observed cache is
unchanged, not that underlying engine usage is unchanged. An `OnGC` callback name
alone does not make a page observation a post-GC sample. Global exited-page
residuals separately require the engine's full-GC flag and strict comparison of
measurement and exit times.

### 4.2 Cohort dimensions

The global process draw enables Element, MTS, BTS, and QuickJS slot collection.
Lynx UI has one additional process-level draw. The event exposes that decision
as `lynx_ui_memory_enabled`; no other source-enablement or source-completeness
fields are needed.

Scheduled rows always expose `current_page_memory_bytes` and four numeric
components. Missing records contribute zero to these fields. This intentionally
trades per-row completeness states for a simpler contract:

- keep `lynx_ui_memory_enabled` fixed to decide whether UI is in scope;
- filter `mts_context_type=1` when MTS heap contribution is required;
- filter `bts_runtime_type="quickjs"` and use `bts_shared` when attributable BTS
  is required;
- use the global VM event for whole shared V8/JSVM heaps.

These dimensions do not prove freshness or that asynchronous images have
finished loading. The page event has no evidence sufficient to select a real-time
measurement cohort.

### 4.3 Summary failure denominator

All anchored pages are eligible for the single early summary. Compare received
unique summaries with and without `early_sample_mean_memory_bytes` to measure
planned-point coverage. Missing values remain in the denominator rather than
being silently retried. Scheduling delay and reload do not invalidate a
summary.

## 5. Early planned-sample mean and peak

Let the anchor be t₀ and logical exit be tₑ:

```text
T = clamp(tₑ - t₀, 0, 20000ms)
```

For a page still alive when the summary is published at 20s, T is 20000ms.
T determines which planned points are due. It is neither the mean's denominator
nor a new exit observation. Without an anchor there is no summary. An instance
whose anchor and exit share a millisecond can contribute if a valid 0s
observation was actually made.

Only the 0/1/2/3/5/8/12/16/20s points enter this accumulator:

```text
required = early planned points whose offset <= T
count = number of observed planned points
M = sum(sample_bytes) / count
coverage = count / required.size()
peak = max(sample_bytes)
```

Emit M and peak only when all required observations exist. Do not fill missing
points with zero, shrink the required set, interpolate, extrapolate, or integrate
between points. The uneven schedule still gives each point equal weight in M.
One required point gives coverage=1 for that short window, not coverage of the
entire page lifetime.

Three independent accumulators keep the total, page-attributed BTS, and MTS
sum, count, attempted/accepted masks, and peak. These are bounded online
statistics over at most nine early points. The presence of each mean/peak pair
carries that accumulator's complete-observation gate.
`kEarlyMemorySampleCount` defines the early prefix of `kPageMemoryOffsetsMs`;
`kEarlyMemoryWindowMs` is its final offset. Accumulation, required masks, and
publication all use this shared count.

### 5.1 Dynamic requirements

A page ending at 8.5s requires 0s, 1s, 2s, 3s, 5s, and 8s, without an exit
endpoint or future observations. A failed 5s observation remains required.

A summary with mean/peak requires:

1. All due planned points observed, with no duplicates.
2. At least one valid observation.
3. Attempted and accepted point sets exactly matching the required set.

Repeated calls do not replace failed samples. Q1.1/Q1.2 use the total-memory
mean/peak; Q1.3/Q1.4 use the page-attributed BTS mean/peak; Q1.5/Q1.6 use the
MTS heap mean/peak.

### 5.2 Complete nine-point comparison

```text
has_early_usage_summary = 1
early_sample_mean_memory_bytes EXISTS
early_window_target_duration_ms = 20000

early_sample_mean_memory_bytes = [m(0s) + ... + m(20s)] / 9
early_sample_mean_bts_attributed_bytes = [bts(0s) + ... + bts(20s)] / 9
early_sample_mean_mts_heap_bytes = [mts(0s) + ... + mts(20s)] / 9
```

All nine points must have been observed; the internal required mask is `0x1ff`.
Select this cohort from the primary summary itself. A short window cannot
provide this comparison but may still have a short-window summary.

### 5.3 Early peak query (Q1.2)

Select `has_early_usage_summary=1` and a present
`early_sample_peak_memory_bytes`. Group by `page_key` and calculate
`PCT95(early_sample_peak_memory_bytes)`; P50/P90/P99 use the same field.
Each instance contributes one maximum of its observed early points, including
valid zero. For complete-window comparisons, also require T=20000ms.

Keep target-duration and UI/runtime cohorts fixed. The denser initial schedule
improves the opportunity to observe loading spikes, but caches can miss
transients between points or before the first-load-completion anchor. Later
scheduled values and teardown values do not enter the early peak.

### 5.4 Early BTS queries (Q1.3/Q1.4)

Select the unique summary and require
`early_sample_mean_bts_attributed_bytes` or
`early_sample_peak_bts_attributed_bytes`. Group by `page_key` and calculate the
desired percentile. These fields use exactly the same due points and
completeness gate as the total-memory fields.

The BTS value is a standalone private heap or the page's tracked shared
QuickJS slot. It is not the whole shared VM heap. Keep runtime type, sharing,
and target-duration cohorts fixed; unsupported or unattributed shared runtimes
remain numeric zero.

### 5.5 Early MTS queries (Q1.5/Q1.6)

Select the unique summary and require `early_sample_mean_mts_heap_bytes` or
`early_sample_peak_mts_heap_bytes`. Group by `page_key` and calculate the
desired percentile. These fields use the same planned points as the total and
BTS fields, but an independent accumulator.

Keep `mts_context_type` and target-duration cohorts fixed. Unsupported or
unavailable MTS records remain numeric zero. The peak covers scheduled cache
observations rather than a continuous MTS measurement.

## 6. Destruction and once-only publication

### 6.1 Delivering saved observations

At the anchor, `MemoryMonitor` saves identity metadata. Planned observations
accumulate statistics. Destruction emits `report_kind=early_summary` only if
the summary is still pending:

```text
cancel the page timer
if there is no anchor or the summary was published: emit nothing
use saved identity, planned-sample statistics and end time
capture the completed event by value and enqueue it with OnEvent
release monitor state
Shell's controller-destruction task explicitly calls Flush(instance_id)
```

The same saved-summary path runs at the early-window boundary for processes
without scheduled delivery. It uses the normal 20s target and consumes the
same once-per-instance budget, even when missed observations invalidate the
statistics.

This path does not call `RefreshMemoryUsage` or read records, `last_usage_`,
slots, VMs, or `GlobalMemoryMonitor`. Runtime teardown and GC cannot change saved
observations. An end summary therefore carries neither `current_*` values nor
`scheduled_offset_ms`.

[LynxShell::Destroy](../../../../shell/lynx_shell.cc) records logical exit and
calls `OnInstanceDestroyed` once. Pages without attributable shared slots can be
removed immediately. The performance and runtime actors continue independent
asynchronous teardown; neither must wait for an exit measurement.

`SetExitTime` supplies the logical bound used to determine which planned points
were due. Direct destruction without that time uses the destruction timestamp;
the fallback may include additional due points and omit the mean when those
points were not observed.

The saved identity includes the page key and engine types. The first
`OnFirstLoadComplete` queries
`GetGenericInfoOrExtraParam(instance_id, "page_id")` once on the report thread.
A nonempty value other than `unknown` takes precedence; otherwise use
[NormalizeMemoryPageUrl](../../memory_page_key.cc), which preserves the input URL
in this implementation. Later observations and the end summary do not query
platform storage again. Same-key instances are still measured separately.

### 6.2 Publication outcomes

| Scenario | Summary carrier | Result |
| --- | --- | --- |
| Page ends before 20s | `early_summary` | Summarize saved planned points |
| Page reaches its normal 20s observation with scheduled delivery | 20s `scheduled` | No additional summary at destruction |
| Page reaches the window boundary without scheduled delivery | `early_summary` | No scheduled rows or additional summary at destruction |
| 20s observation races with exit | Reporter-thread publication state | Publish once, without a new exit observation |
| Sampling fails | The applicable carrier without mean/peak | Do not publish a second summary to retry |
| No anchor | None | No empty summary or primary-score entry |
| 20s point is missed | A delayed planned callback or end summary | Missing points invalidate the summary; no backfill |

`has_early_usage_summary` identifies the one carrier. Later scheduled rows use
0 and omit summary metrics. Fixed-point queries select `scheduled`; an end
summary is not memory at exit.

Event deduplication is provided by the data platform. Crashes, OOM, or forced
termination may bypass destruction, so received summaries cannot prove coverage
of every short, high-memory page.

## 7. Page-key leaderboard and quality gates

Q1.1 cached-memory mean query:

```text
event = lynxsdk_performance_entry_memory_v2
schema_version = 10
has_early_usage_summary = 1
early_sample_mean_memory_bytes EXISTS

group by page_key
metric = PCT95(early_sample_mean_memory_bytes) / 1048576
```

Fix platform, SDK/build, experiment, and sampling configuration. Do not take PCT
over all raw events, average per-offset P95s, or divide the sum of every
instance's sample sums by the sum of every instance's sample counts. Those
operations change instance weighting.

P95 is a percentile, not 95% confidence. More users do not remove systematic
source or cache error.

Illustrative quality thresholds to validate for a deployment:

| Check | Suggested starting point |
| --- | --- |
| Summaries with a mean in the last 7 days | At least 1000 |
| Distinct users in the same cohort | At least 200 |
| Mean-emission rate | At least 90%, also checked by target-duration strata |
| Stability | At least 3 days with at least 200 valid summaries each |
| Daily versus 7-day score difference | Within 20% as a diagnostic, not a statistical guarantee |

Actual sample size means summaries, not weighted event counts. Instances from
the same user/process are correlated; rigorous confidence intervals need
clustered analysis. A 256MiB threshold can be a screening example, not an engine
safety limit. Sparse, low-coverage, or stale/unknown-freshness groups require
investigation rather than a low-memory label.

For summary success, let A count received unique summaries and B count rows with
emitted summary metrics. B/A is collection success among received summaries,
not end-to-end delivery.

Include missing-point failures in A. Pages without an anchor emit no page-memory
summary, so their coverage needs a separate lifecycle denominator.

Stratify target duration:

| Stratum | Duration |
| --- | --- |
| Under 5s | 0 <= T < 5000ms |
| 5–10s | 5000 <= T < 10000ms |
| 10–20s | 10000 <= T < 20000ms |
| Full window | T = 20000ms |

Visit-duration mix affects this metric. Good long-window coverage does not prove
short-page coverage.

## 8. Fixed points and the global complement

```text
event = lynxsdk_performance_entry_memory_v2
schema_version = 10
report_kind = scheduled
scheduled_offset_ms = 5000

group by page_key
AVG / PCT95 / PCT99(current_page_memory_bytes)
```

This cohort need not survive until 20s or have a primary summary. Include zero.
Do not combine different offsets or end summaries with the fixed-point cohort.
Later offsets select longer-lived pages; subtracting two offsets' P95s is not
same-instance growth. Fix UI/runtime dimensions to keep the available-record
scope comparable.

The full 20s comparison, sampled peak, and tail P99 provide complementary views.
P99 needs more observations, and a sample maximum is not a stable tail estimate.

Page means describe individual instances. Twenty concurrent instances of one key
at 10MiB each can still consume 200MiB in aggregate. Global same-key queries use
the selected group's total and N/n weight; their raw PCT does not inherit the
page summary's equal-instance interpretation. These overlapping views must not
be added or forced into one score.

## 9. Flat event properties

See the [page-property table](./memory_monitor_property_registration.md#page-event-lynxsdk_performance_entry_memory_v2--21-fixed-properties-plus-ui-categories)
for the 21 fixed properties and dynamic UI category properties.

Common identity and carrier properties:

```text
schema_version
lynx_ui_memory_enabled
page_key
mts_context_type
bts_runtime_type
bts_shared
report_kind
```

Use platform event metadata for calendar dates and deduplication, not a direct
conversion from monotonic time.

Scheduled observations:

```text
scheduled_offset_ms
current_page_memory_bytes
current_element_bytes
current_mts_heap_bytes
current_bts_attributed_bytes
current_platform_ui_bytes
```

Every scheduled row carries the total and four components. Missing records
remain numeric zero; use UI/runtime dimensions to interpret them. Combining
component percentiles cannot produce the percentile of their sum.

When UI collection is enabled, each UI category also contributes its cached
byte size under a key formed by replacing `-` with `_` and appending `SizeBytes`
(for example, `x-viewpager-ng` becomes `x_viewpager_ngSizeBytes`). These fields
retain zero values and are absent from saved end summaries. They describe the
UI subtotal and must not be added to it again.

Summary properties:

```text
has_early_usage_summary
early_window_target_duration_ms
early_sample_mean_memory_bytes
early_sample_peak_memory_bytes
early_sample_mean_bts_attributed_bytes
early_sample_peak_bts_attributed_bytes
early_sample_mean_mts_heap_bytes
early_sample_peak_mts_heap_bytes
```

Incomplete summaries omit all means/peaks instead of emitting zero.
The complete-window comparison reuses these properties.

## 10. Ownership and delivery

`MemoryMonitor` owns bounded state: anchor, next point, timer, source caches,
three accumulators, one publication flag, and saved identity.
Publishing the summary releases its saved identity.

Producers measure on their owning threads. Category-record combination,
accounting, summary aggregation, and event construction happen on the report
thread. Callbacks update source records; the page schedule reads those records
without requesting a cross-thread all-source snapshot or inferring GC
confidence.

Events use [EventTracker](../../../event_report/event_tracker.cc):

```text
EventTracker::OnEvent(builder)
  SetName("lynxsdk_performance_entry_memory_v2")
  SetInstanceId(instance_id)
  SetProps(...)  // flat properties
```

Regular events are queued for a later flush. Shell's controller-destruction task
releases the controller, lets `MemoryMonitor` publish its saved summary, then
explicitly calls `EventTracker::Flush(instance_id)`. The performance controller
does not rely on a shell actor's automatic post-invocation flush for this step.

Builders capture completed values, never a monitor that may be destroyed before
the builder runs. Delivery remains asynchronous and cannot guarantee arrival
after a forced process termination.

## 11. Numerical examples

Assume all due planned observations are available:

| Planned seconds / exit second | Observations in MiB | Count | Mean in MiB |
| --- | --- | ---: | ---: |
| 0, 1 / 1.5 | 300, 300 | 2 | 300 |
| 0, 1 / 1.5 | 100, 200 | 2 | 150 |
| 0 / 0.5 | 300 | 1 | 300 |
| 0, 1, 2, 3, 5, 8 / 8.5 | 100, 200, 300, 400, 500, 600 | 6 | 350 |
| 0, 1, 2, 3, 5, 8, 12, 16, 20 / later | All 100 | 9 | 100 |

The second row contributes 150MiB even if actual exit-time usage is 300MiB or GC
has reduced it to zero. Changes after the last planned point are unobserved.

For `80,620,80,80,80,80,80,80,80`, the complete-window mean is 140MiB and the
sampled peak is 620MiB. Neither guarantees capture of the true instantaneous peak.

## 12. Validation scope

Behavioral validation should cover:

- Duplicate load completion, later pipelines, actual FMP, and reload having no
  effect on the existing window.
- Shared-heap records replaced by page slots, including valid zero and missing
  attribution.
- V8/JSVM subtotals and unsupported JSC/MTS sources.
- Short windows, absent anchors, missing due points, and one-point summaries.
- Delayed callbacks remaining valid without backfilling skipped points.
- Once-only publication when 20s and destruction race.
- A 1s total/BTS/MTS peak enters Q1.2/Q1.4/Q1.6 while larger values from 40s
  onward cannot alter it.
- Both runtime/monitor teardown orders; cleared records, changed slots, or a
  removed global entry must not alter saved observations.
- Producer updates without events or aggregate-page refresh, followed by
  aggregate refresh at the next scheduled observation or active Trace emission.
- Global snapshots refreshing active pages from source records changed between
  scheduled observations.
- Deferred flush and explicit destruction flush preserving captured values.

Cache-error experiments should compare independent memory trajectories with
callback caches while varying callback thresholds, delivery delay, silent
intervals, and slot attribution changes at constant heap size. Evaluate
fixed-point error, mean error, page-key ranking, and mean-emission rates by
target duration.

Simulations validate the modeled workload and query implementation, not actual
producer freshness, background scheduling, short-page delivery, or production
distributions. Those require runtime evidence. Distinguish disabled sources,
missing caches, and usable but old caches instead of filling unknowns with zero.
