# Global Memory Reporting Design

This document describes the `schema_version=10` global memory event family. It
covers process accounting, page residency, concurrent page groups, shared VMs,
bundle-local MTS pools, post-exit residuals, and memory-pressure observations.

Related documents:

- [Page reporting design](./page_memory_monitor_reporting_design.md): fixed
  observations and one total/BTS/MTS early mean/peak summary per page instance
  (Q1.1-Q1.6).
- [Query guide](./memory_monitor_query_guide.md): complete filters and formulas.
- [Property reference](./memory_monitor_property_registration.md): all emitted
  fields and their conditions.
- [GlobalMemoryMonitor](../global_memory_monitor.h) and its
  [implementation](../global_memory_monitor.cc): state ownership and reporting.

## 1. Goals and event budget

Events are delivered through `EventTracker::OnGlobalEvent`. One collected
snapshot is routed into compact analysis-specific events. There are no dynamic
Top-N properties or nested dictionaries. Values are frozen before queueing;
later flush and delivery do not change the observation.

The monitor supports these questions:

1. Which pages survive a complete observation window and contribute substantial
   resident memory?
2. Which page keys have high combined memory because many instances coexist?
3. Which large shared VMs persist, including RuntimeManager-owned V8/JSVM?
4. Which exited pages leave substantial residuals in living shared QuickJS VMs?
5. Which VMs repeatedly serve pages, and when do QuickJS page slots fill?
6. Which shared QuickJS VMs have high Unknown, Common, or Overflow memory?
7. What fraction of process PSS is accounted for by Lynx, grouped by physical
   RAM and observed PSS milestones?
8. Which bundle-local MTS pools have high heap memory or runtime counts?

| Event | Production gate | Analysis |
| --- | --- | --- |
| `lynxsdk_memory_process` | Positive PSS and no synchronous VM query failure | Process accounting and Q8 PSS milestones |
| `lynxsdk_memory_page` | Timer snapshot with an active-page sample | Q3.2 and Q4 |
| `lynxsdk_memory_page_residency` | One or more due 10-minute observations | Q3.1 |
| `lynxsdk_memory_page_residual` | Timer snapshot with a positive residual PPS sample | Q6 |
| `lynxsdk_memory_persistence` | A valid interval with at least one valid area term | Q7 |
| `lynxsdk_memory_vm_stat` | Timer snapshot with an all-engine VM sample | Q5.1, Q5.2, and Q9.1 |
| `lynxsdk_memory_vm_quickjs` | `report_kind=sample` for usable timer data; `report_kind=slot_overflow` for a first overflow | Q5.3, Q9.2, and Q10 |
| `lynxsdk_memory_mts_pool` | Timer snapshot with a bundle-local pool sample | Q11 |

Event names intentionally have no `_v2` suffix. There is no separate collection
quality event: source-specific field/event presence is the validity contract.

`lynxsdk_memory_page_residency` evaluates instances once at creation + 10
minutes and combines due instances by `page_key`. It emits actual
`evaluated_page_count`/`alive_page_count` values. Instances whose processes
cannot complete their observation window contribute neither count.

The existing 8-second process poll checks the earliest residency deadline before
scanning. Pending `unique_ptr` observations move to a replacement vector, leaving
object addresses stable; expired entries are released. Observations contain
only identity/timing metadata and a saved exit key, with no owning page/VM
references. See [Q3](./memory_monitor_query_guide.md#q3-which-pages-remain-resident-after-creation).

The all-engine VM draw serves heap and reuse analysis. A separate QuickJS-only
draw serves residual and reserved-slot analysis, with a denominator based on
the QuickJS population rather than all engines.

## 2. Observable scope

### 2.1 Engine capabilities

| Engine | Shared whole heap | Shared page bytes / post-exit residuals | Available global analysis |
| --- | --- | --- | --- |
| QuickJS | Supported | Supported with actual tracking and valid slots | Cached heap, age, attribution, slots, reuse |
| V8 / JSVM | Supported | Unsupported | Heap, age, idle time, active pages, cumulative bindings |
| JSC | Unsupported | Unsupported | Lifecycle, counts, and reuse; no heap-byte estimate |

Only LepusNG/QuickJS MTS heaps are measured here, and they belong to their pages.
An existing but unmeasured MTS source remains unsupported rather than becoming
inapplicable. Sampling settings cannot supply missing JSC heap measurements or
V8 page-level residuals.

Bundle-local `MTSRuntimePool` objects are separate process-owned candidates.
`ReportPoolState` reads each runtime's `GetCurrentHeapSizeBytes()` while holding
the pool mutex after creation, preload, removal, or trace start, then publishes
the aggregate.
Every context type remains in the pool population; a getter result of zero is a
measured zero. `LynxGlobalPool` is identified by its separate construction path
and excluded.

### 2.2 VM identity and names

QuickJS/JSC group sharing registers actual VMs by `group_id`. V8/JSVM share a
whole heap at RuntimeManager/runtime-type scope, so business groups must not
count that heap repeatedly. RuntimeManagers on different JS threads can own
different VM objects; each object is counted once.

```text
vm_name:
  group_id
  runtime_manager/v8
  runtime_manager/jsvm

vm_runtime_type:
  quickjs | v8 | jsvm | jsc | unknown
```

`GlobalMemoryMonitor` derives the name. Local registration uses object identity
and weak-reference ownership; pointer addresses are not included in names.
Same-name VMs remain distinct candidates and contribute separately. A page's
business group does not necessarily identify its actual VM name.

Each object has its own creation time. The next snapshot removes an expired weak
reference; registration first removes an expired object that reused the same
address, so a replacement cannot inherit age or counters. The name is an
aggregation dimension, not a unique lifecycle ID or a basis for joining
reconstructed VM lifetimes.

V8/JSVM heap queries post to the owning JS thread and wait for at most one
second. The JS task locks the weak reference, reads the heap, and releases the
strong reference there. The report thread updates the cache only when the task
completes before the timeout. Ownership must not move engine destruction to the
report thread.

### 2.3 Meaning of residual memory

Residuals are tracked slot bytes measured by a full GC completed **after** page
exit. Monitoring does not force GC. Pending pages do not enter residual totals
or sampling. Unknown residual is not zero.

```text
post-exit residual != proven permanent leak
```

Full-GC survivors may still be held by legitimate tasks or references.
The `leak_page_*` prefix does not prove unreachable or permanently
unreclaimable memory.

QuickJS identifies a full GC and captures its monotonic measurement time on the
owner thread. `BtsVmState` stores the latest valid `full_gc_slots[256]` and
`full_gc_at_ms`; ordinary callbacks update only current caches.

Qualification is strict:

```text
exited_at_ms < full_gc_at_ms
```

Equal-millisecond ordering is unknown and must wait for another full GC.
Measurement time, rather than notification arrival order, handles delayed exit
and GC notifications. An older GC cannot replace a newer saved snapshot.
Invalid heap/slot samples cannot confirm residuals.

The retained full-GC array avoids a per-slot lifecycle map or a full page scan
on every GC. It costs one 256-entry array and a timestamp per VM and is released
with the VM.

## 3. Triggers and collection

### 3.1 Trigger rules

Monitoring starts at the first enabled page, VM, or bundle-local pool
registration. Registration starts an 8-second process poll without immediately
emitting a snapshot.

| Trigger token | Rule |
| --- | --- |
| `timer` | Periodic deadline on the monitor timeline; `time_interval_sec`, default 300 seconds |
| `pss` | Observed PSS enters a higher high-water bucket; `pss_threshold_mb`, default 256MiB |
| `memory_warning` | A platform memory-pressure notification above NONE |
| `slot_overflow` | First page-slot allocation failure in a VM |

The trigger is encoded by event production rather than a query-side reason
field. The same poll can satisfy timer and PSS gates from one snapshot. Memory
warnings and the first slot overflow queue separate requests. They do not reset
the timer deadline or automatically merge into its next event.

Timer/PSS settings of 0 disable those triggers individually, not pressure,
slot, or residency processing. Positive-pressure notifications have no additional
monitor-level cooldown.

Missed periodic deadlines are skipped, not replayed after suspension. Monitoring
that starts after process launch cannot reconstruct earlier samples. Crashes or
forced termination may prevent a final event.

Starting a Perfetto trace with memory tracing enabled forces collection for the
current process and writes a marker in the platform trace directory. On
subsequent app launches, that marker overrides the configured sample rates so
collection begins before pages and VMs are created. The marker remains until the
app's data is removed.

### 3.2 PSS high-water buckets

With the default bucket width:

```text
bucket_bytes = 268435456
bucket = floor(pss / bucket_bytes)
trigger only when bucket > highest_observed_bucket
```

The initial high-water bucket is zero. The first poll may already cross several
buckets. One upward jump produces one event with:

```text
trigger_pss_previous_milestone_bytes
trigger_pss_milestone_bytes
```

These fields occur only for a PSS trigger. Falling PSS does not reset the high
water, so oscillating near a boundary does not repeatedly trigger it. Polling can
miss short peaks between observations.

### 3.3 Single-task collection

Each request completes in one report-thread task. Other report-thread updates
remain queued while synchronous heap queries run:

```text
read reporter-owned page identities, bindings and unique shared VMs
-> refresh page BTS attribution from shared QuickJS slot caches
-> query V8/JSVM heaps on their owning JS threads with a 1s timeout
-> update each VM cache only after its query completes in time
-> aggregate totals and select all-engine/QuickJS/page/residual samples
-> build only the qualified analysis events
-> freeze and enqueue each event through OnGlobalEvent
```

Page Element/MTS/UI/owned-BTS and shared QuickJS values use existing caches.
A missing QuickJS cache remains missing; reporting does not force an engine
query or call a cache read a new measurement.

V8/JSVM heap queries are necessary because these engines do not provide the same
push path. Failed queries retain cached values for later recovery but suppress
that VM's heap fields in the current VM sample. A process event is not produced
when any synchronous heap query fails or times out. Lifecycle/reuse data remains
valid, and another VM is not resampled to manufacture a heap result. Expired
registrations are cleaned before selection.

Cross-thread heap queries wait for at most one second. A timeout keeps the
previous cache for later recovery but excludes that VM's heap fields from the
current snapshot. Other JS/UI activity can still change memory: reporter
confinement does not make all sources simultaneous or real-time.

## 4. Identity and totals

### 4.1 Common properties

```text
schema_version
```

Global-event deduplication is provided by the data platform. The monitor does
not add process/session or snapshot-sequence properties, and the documented
queries require no cross-event join.

At first-load completion, the reporter queries
`GetGenericInfoOrExtraParam(instance_id, "page_id")` once. A nonempty value other
than `unknown` becomes `page_key`; otherwise
[NormalizeMemoryPageUrl](../../memory_page_key.cc) supplies the fallback.
The implementation in this source tree returns the URL unchanged.

`page_id` is a host input, not a monitor-owned output property. Before resolution,
URL changes update the fallback. After resolution, planned observations and
snapshots read the saved key. Exit before first load preserves the current
fallback without querying host storage. Residency begins at creation and uses
the active state's key or the key saved at exit.

Completed events capture values rather than mutable states or owning object
references.

### 4.2 Configuration and producer gates

The global process draw enables every non-UI source. Lynx UI has one additional
process-level draw. Events whose totals can include UI memory emit
`lynx_ui_memory_enabled`; keep that value fixed when comparing those totals.

There is no standalone quality event and no
`vm_heap_snapshot_complete` property. A V8/JSVM query failure or timeout
suppresses `lynxsdk_memory_process`; if that VM is selected for
`lynxsdk_memory_vm_stat`, its lifecycle/reuse fields remain but its heap value
and `vm_heap_selection_weight` are absent. JSC follows the same
field-absence contract because its heap is structurally unsupported.

Residual totals and PPS samples contain only confirmed post-exit full-GC slots.
Pending pages are reported separately and do not invalidate confirmed values.
Residual byte-time fields are emitted only when the producer's internal
residual-completeness check succeeds. No separate event-level completeness
filter is required.

No field certifies freshness, simultaneous measurements, full widget coverage,
or physical ownership. Missing MTS/BTS records lower the aggregate; use runtime
dimensions to define comparable cohorts.

### 4.3 Deduplicated accounting

`InstanceState::memory_usage` contains the latest page subtotal, including the
BTS component recorded in that same total. The corresponding component must be
subtracted before counting a shared heap once:

```text
lynx_accounted_memory_bytes =
    sum(active_page.total_bytes
        - (page.bts_shared ? page.bts_bytes : 0))
    + sum(unique_supported_shared_vm.heap_bytes)
    + sum(bundle_local_mts_pool.heap_bytes)
```

Use available, non-overlapping measured components. Unsupported MTS/BTS records
are absent from the aggregate rather than represented by a validity state.

For pages of 10MiB and 20MiB containing shared slots of 4MiB and 6MiB, a 40MiB
shared heap, and a separate 60MiB V8 heap:

```text
accounted = (10 - 4) + (20 - 6) + 40 + 60 = 120MiB
```

Adding page totals directly to shared heaps duplicates 10MiB. Adding only page
totals omits common bytes, residuals, and the V8 heap. Exited-page slots already
belong to shared heaps and must not be added again.

### 4.4 Registry totals

```text
active_page_count
shared_vm_count
shared_vm_heap_bytes_sum
mts_pool_count
mts_pool_runtime_count
mts_pool_heap_bytes_sum
shared_vm_tracked_exited_page_retained_bytes
```

Shared V8/JSVM pages may have measurable Element/MTS/UI subtotals without page
BTS attribution. Shared heap includes active allocations, residuals, and common
overhead. Total, heap, and residual views overlap and cannot be added.

Lifecycle survival uses residency rows' `SUM(alive_page_count) /
SUM(evaluated_page_count)`, independently of memory support or successful load.

## 5. Active pages and concurrent same-key groups

### 5.1 Uniform active-instance selection

Among N registered active instances:

```text
p_i = 1/N
N = active_page_count
page_memory_bytes_weighted = page_memory_i * N
```

Age is measured from creation. If N=0, the event is not produced.

The event contains key, age, raw/weighted memory, BTS runtime type, and sharing
mode. See the
[property table](./memory_monitor_property_registration.md).

### 5.2 Residency and resident-memory contribution

For survival, filter residency rows and group by `page_key`. Every instance is
evaluated once after a complete creation-based window. An early exit waits
until the original deadline; short processes must not contribute failures
before comparable windows are observed.

The resident-memory query uses `lynxsdk_memory_page` with:

```text
page_age_ms >= 600000
```

Group by `page_key`:

```text
A = SUM(page_memory_bytes_weighted)
B = SUM(active_page_count)
resident-memory contribution = A
mean memory per resident instance-snapshot = A/B
```

A combines memory size, instance count, and repeated presence over snapshots.
A/B estimates individual resident-instance size. Both retain the original N
after the age filter. Keep UI/runtime dimensions fixed.

This is a snapshot population, not a distribution of all load lifetimes.
With varying N, raw PCT is not corrected for instance selection.

### 5.3 Same-key aggregation

After selecting a page with key u, scan all active instances with that key:

```text
n = active instances with key u
S = sum of their accounted page memory
q_u = n/N
group_weight = N/n
weighted_bytes = S*N/n
weighted_instance_count = n*N/n = N
```

The group includes different business data, VMs, JS threads, and visibility
states. It excludes exited pages and does not add shared whole heaps.

Group properties include instance count, memory sum, maximum individual memory,
selection weight, and weighted sum. Presence and key are shared with the
selected active-page block.

For the same filtered group cohort:

```text
A = SUM(page_group_memory_bytes_weighted)
B = SUM(active_page_count)
C = SUM(page_group_selection_weight)

mean concurrent group memory when present = A/C
mean concurrent instance count when present = B/C
byte-snapshot contribution = A
```

For a fixed snapshot, `E[I(selected u)*S*N/n] = S`. The ratio A/C is
self-normalized and not exactly unbiased in every finite sample. Ordinary
AVG/PCT(S) favors groups with higher n/N.

To estimate the fraction above a threshold with simple aggregates, divide the
weight SUM filtered by `S >= threshold` by the baseline weight SUM. Weighted
quantiles require suitable query support.

For N=30, n=20, and S=200MiB, selection probability is 2/3, weight is 1.5, and the
selected row carries 300MiB of weighted memory. Its expected group contribution
is 200MiB. The 300MiB value is an estimator term, not measured group size.

Do not filter the selected page's engine to classify the whole group: S and n
include every engine sharing the key, while such a filter changes selection to
`n_type/N`. Count queries use B/C and retain all observed members.

## 6. Exited-page residual sampling

Candidates have logically exited, retain a valid slot in a living shared
QuickJS VM, and have a strictly post-exit full-GC value Lᵢ > 0.
Values come from `full_gc_slots`, not the current heap/slot cache.

Confirmed zero slots and pending pages are not sampled and have no Q6 event
fields. They remain outside confirmed total T and the PPS population. Their
presence does not invalidate the probability or weight of a confirmed sample.

Exit notification updates lifecycle state without waiting for
`MemoryMonitor` destruction. Tracked shared-slot identity remains until VM
destruction; non-attributable page state can be cleared immediately. Page
summary delivery uses saved early observations and is independent of this path.

Let T be the sum of all confirmed exited-slot residuals:

```text
p_i = L_i/T
selection_weight = T/L_i
L_i/p_i = T = shared_vm_tracked_exited_page_retained_bytes
```

A weighted reservoir selects one candidate in one pass:

```text
total = 0
selected = none
for each positive-weight candidate:
    total += weight
    replace selected with probability weight/total
```

`lynxsdk_memory_page_residual` is produced only when this timer-snapshot draw
succeeds. Apply optional page-key filters directly. SUM(T) on the selected rows
estimates the confirmed subset's contribution even when another page is
pending.

```text
A = SUM(shared_vm_tracked_exited_page_retained_bytes)
B = SUM(leak_page_selection_weight)
positive-residual mean = A/B
```

Both sums use identical filters/grouping and require a present positive weight.
Retain the original T/Lᵢ after all filters. A is cumulative byte-snapshot burden;
A/B is mean positive residual per page-snapshot. Neither counts each exited
instance only once, and the mean excludes zero residuals.

For a key's share of all residual bytes, divide its selected-row weighted SUM
by SUM(T) over all baseline timer snapshots.

The compact event deliberately omits owning-VM and tracking-coverage
diagnostics. Tracking cannot recover untracked, overflow, other-engine, or
private-VM post-exit bytes. An inferred raw residual `T/selection_weight` remains
size-biased under ordinary AVG/PCT; observed residuals are not a guaranteed
lower bound on permanent leakage.

## 7. Shared-VM sampling and rankings

### 7.1 Candidates and lifecycle times

Candidates are unique registered shared VMs, including JSC, zero heaps, and
missing heaps. Selection does not require active pages or tracked slots.

```text
H_i = heap value used by this snapshot
age_i = snapshot start - VM creation time
idle_i = time with no bound active pages
```

Idle starts when the last active page exits, or at creation for an unused VM.
A new binding resets it. It describes page use, not whether JavaScript tasks are
running. Common binding/exit paths maintain creation, active-page, and cumulative
binding counts across engines.

### 7.2 Uniform selection across engines

With V registered shared VMs:

```text
p_i = 1/V
vm_selection_weight = V
vm_heap_bytes_weighted = H_i*V
```

For a fixed snapshot, `E[I(select i)*H_i/p_i] = H_i`. Summing corrected values
by stable name estimates that name's heap contribution. V=0 gives no selected VM;
otherwise missing or zero heaps can still be selected.

Heap validity determines whether raw/weighted heap fields exist. Lifecycle and
reuse fields remain useful without heap support. Actual slot-specific fields
require tracking. Missing data must not be represented as a measured zero.

### 7.3 Old-VM heap contribution, mean, and percentiles

For `lynxsdk_memory_vm_stat`:

```text
vm_age_ms >= 1800000
```

Group by name and runtime type:

```text
A = SUM(vm_heap_bytes_weighted)
B = SUM(vm_heap_selection_weight)
heap contribution = A
mean heap per VM-snapshot = A/B
```

Valid zero heaps are included because the heap denominator is emitted with them.
To focus on VMs unused by pages for at least 10 minutes, additionally require
`vm_active_page_count=0` and `vm_idle_ms>=600000`.

Age and heap thresholds change the cohort, not the original selection weight.
A is accumulated contribution, not one VM's current size. Same-name objects
remain distinct sampling units; the ratio is not exactly unbiased for every
finite sample. Display actual row and independent-process counts.

For a 900MiB V8 VM aged 1 hour and a 100MiB QuickJS VM aged 1 minute, each has 50%
selection probability. Their weighted terms are 1800MiB and 200MiB. After an
age>=30min filter, expected V8 contribution is `0.5*1800=900MiB`.
No growth history is needed.

A high contribution with a low mean can result from many small same-name VMs.
Engine-used heap differs from page memory, reserved engine space, native
overhead, and PSS. QuickJS callback caches may understate later growth or
overstate later frees; inspect `vm_heap_source_age_ms` for diagnosis.
Within identical rows, a positive-weight mean must lie between raw heap MIN/MAX.

For ordinary P50/P90/P95/P99, additionally group by exact
`vm_selection_weight` or filter it to one integer. Equal V gives equal
selection probability within that stratum. A pooled cross-V quantile needs
weighted-quantile support; neither
PCT(Hᵢ×V)/PCT(V) nor an average of stratum P95s can reconstruct it.
See [Q5.2](./memory_monitor_query_guide.md#q52-heap-size-percentiles).

`lynx_shared_vm_process_pss_ratio` includes all measurable shared heaps and cannot
be attributed by the selected VM's name/type. A VM that is old and large now is
not proven to have been large throughout its lifetime. Heap × age is not
measured byte-time or proof of a leak.

### 7.4 Complete QuickJS VM residuals

`lynxsdk_memory_vm_quickjs` rows with `report_kind=sample` draw uniformly from
registered QuickJS VMs. `vm_retained_bytes` sums all exited-page full-GC slots
of the selected VM. The weighted property multiplies this by the QuickJS VM
count. Emission requires:

- QuickJS tracking, valid slots, and a valid full-GC snapshot.
- A strictly post-exit full GC for every tracked exited page in that VM.
- Fewer than 253 allocated page slots.
- No allocation failure or lost history from an untracked exit.

Confirmed zero is emitted; incomplete history is omitted. Another VM's pending
pages do not invalidate this selected VM. All qualifying exited slots are
included.

SUM(weighted residual) ranks contribution. Dividing by
`SUM(vm_retained_selection_weight)` gives mean residual including zero
VMs. Reserved slots are excluded. These values overlap whole heap and Q6
residuals and must not be added to either.
See [Q5.3](./memory_monitor_query_guide.md#q53-quickjs-vm-residual-contribution).

### 7.5 Reserved-slot leaderboards

Shared QuickJS VMs with tracking and valid slot caches emit three raw values:

| Slot | Raw bytes |
| --- | --- |
| Unknown=0 | `vm_unknown_bytes` |
| Common=1 | `vm_common_bytes` |
| Overflow=2 | `vm_overflow_bytes` |

Unknown records allocations without page-slot task attribution. Common records
shared engine infrastructure. Overflow combines allocations from pages after
slot exhaustion, including active bytes and exited residuals; it is not all
leaked memory.

The draw includes registered QuickJS VMs. Event production requires actual
tracking and valid slot data. Post-exit residual completeness is not required
for these current slot caches.

```text
P50/P90/P95/P99(category_bytes), grouped by vm_name
```

Include valid zeros; a raw>0 filter changes the percentile population. Names
aggregate multiple actual VM objects.

These bytes are already part of shared heap, not additions to accounted memory
or exited-page residuals. Page totals cannot detect every attribution error
represented by Unknown bytes.

## 8. Slot transitions and VM reuse

Cross-engine reuse uses timer VM details:

```text
SUM(vm_total_page_count_weighted)
/ SUM(vm_selection_weight)
```

No heap/slot validity filter applies. New page bindings and leaving/rebinding can
increment the counter; reload within the same VM does not. Summing over
snapshots repeats existing binding history rather than counting new loads in
the query period.

QuickJS slot layout:

```text
array length = 256
reserved slots = 0, 1, 2
page slots = 3..255
page capacity = 253
```

Allocating the 253rd page slot reaches capacity but does not trigger an event. A
later first `-1` allocation triggers overflow, and that page's bytes are routed
to Overflow. Previously allocated slots retain identity. Tracking disabled is
not saturation.

At the transition, save the VM name, creation time, counters, valid cached heap,
and allocated-slot count. After binding commits, queue one request with those
scalar values. Do not capture the entire VM, slot arrays, or runner. Later VM
destruction or a same-name replacement must not rewrite the trigger.

First-failure rows are independent `lynxsdk_memory_vm_quickjs` events with
`report_kind=slot_overflow`. A timer sample and first failure at the same
snapshot therefore produce two rows. The common `vm_*` properties are shared
with sample rows; `vm_heap_bytes` carries the copied trigger-time cache and is
always present.

Trigger age is measured at snapshot start from saved creation time and therefore
includes queue delay. Trigger VM is not the independent random VM sample; do not
apply entity weights to it.

Overflow rows do not form a complete VM lifecycle stream. Observation-window
censoring, queueing, process termination, and delivery loss can omit them.

## 9. Byte-time boundaries

Optional byte-time metrics use a snapshot-endpoint rectangle estimate, clipped
to the entity's actual existence:

```text
interval_start = max(previous_snapshot_start, current_snapshot_start - 300000)

Exited page i:
  dt_i = max(0, current_snapshot_start - max(interval_start, exit_i))
  leak_page_attributed_byte_ms = (L_i * dt_i) / p_i = T * dt_i

VM i:
  dt_i = max(0, current_snapshot_start - max(interval_start, creation_i))
  vm_attributed_heap_byte_ms = (H_i * dt_i) / p_i = H_i * V * dt_i
```

The first snapshot has no preceding boundary. A selected VM with a failed heap
query contributes no VM area; an incomplete residual endpoint contributes no
residual area. Other valid sources remain usable. All snapshot trigger types
define boundaries; residency events do not. Intervals use monotonic time and a
5-minute cap, independent of foreground state.

Residual endpoints use the latest qualifying full-GC value. A pending residual
endpoint emits no complete residual area, and later snapshots do not fill that
missing interval. This is not a continuous GC-to-GC integral.

If a page exits 290 seconds after the preceding boundary and the next snapshot
is at 300 seconds, its maximum residual interval is 10 seconds, not 300.
Entity-specific clipping means one global duration cannot replace every dtᵢ.

Limits:

- Endpoint memory may have changed within the interval.
- VMs destroyed between endpoints are absent, so transient area can be missed.
- Allocations, GC, and exit ordering introduce approximation error.
- Filtering to timer rows after all-trigger interval construction drops
  intervals; it cannot produce a complete timer-based integral.
- Byte-snapshot contribution, byte-time, and VM age are different metrics.

Complete lifecycle area would require additional boundary observations; these
events do not claim to reconstruct it.

## 10. Bundle-local MTS pool sampling

For P physical bundle-local pools, timer snapshots draw one pool uniformly:

```text
p_i = 1/P
heap_bytes_weighted = heap_i * P
runtime_count_weighted = runtime_count_i * P
selection_weight = P
```

Group by `page_key` and `mts_context_type`. The key begins as the normalized
template URL. Before a snapshot, the monitor deterministically chooses the
lowest active instance ID whose original URL exactly matches the template URL,
resolves that instance's host `page_id` once, and freezes the result. With no
matching active instance, later snapshots retry and use the URL fallback.

All context types and zero heaps remain in the population. The selected pool's
raw heap, runtime count, age, weighted values, and matching denominators are
always emitted. Pool state is erased at destruction. These heap bytes are
process overhead: they contribute to `lynx_accounted_memory_bytes` but never to
page memory or `GlobalMemoryUsage.total.mts_bytes`.

## 11. Process memory ratios

Both PSS triggers and ratio denominators use
[GetProcessPssBytes](../../../../../base/include/memory/process_memory_info.h).
Apple/Windows provide their platform equivalents within that implementation.
Lynx's allocator/platform accounting is not identical to physical PSS, so ratios
can exceed 1.

```text
lynx_accounted_process_pss_ratio =
    lynx_accounted_memory_bytes / process_pss_bytes
lynx_shared_vm_process_pss_ratio =
    shared_vm_heap_bytes_sum / process_pss_bytes
lynx_mts_pool_process_pss_ratio =
    mts_pool_heap_bytes_sum / process_pss_bytes
lynx_tracked_page_retained_process_pss_ratio =
    shared_vm_tracked_exited_page_retained_bytes / process_pss_bytes
```

Ratios exist only for positive PSS. Read failure is -1, not zero or a reused
measurement.

Collection:

- The process poll reads PSS every 8 seconds and compares high-water buckets.
- Android reads system PSS and converts KB to bytes. Linux uses `smaps_rollup`
  with a `smaps` fallback. Harmony uses Native HiDebug PSS.
- After heap collection, another PSS read supplies event ratios. It may differ
  from the triggering read. Bucket boundaries are not the ratio denominator.
- A failed/nonpositive final PSS read suppresses the process event.
- A synchronous VM heap query failure also suppresses the process event, so
  consumers do not need a completeness filter.

Example 1GiB crossing query:

```text
schema_version = 10
lynx_ui_memory_enabled = <fixed cohort>
trigger_pss_previous_milestone_bytes < 1073741824
trigger_pss_milestone_bytes >= 1073741824
```

Use 2147483648 for 2GiB and 3221225472 for 3GiB. A jump across several buckets
means the threshold was crossed by the poll, not that measurement occurred at
exactly that byte value.

Fix platform, use host RAM metadata for groups, and calculate
PCT50/PCT90/PCT99 of `lynx_accounted_process_pss_ratio`, displaying actual rows
and distinct processes/users. Missing MTS/BTS records can lower the numerator.

```text
P99(accounted/PSS) != P99(accounted)/P99(PSS)
AVG(accounted/PSS) != AVG(accounted)/AVG(PSS)
```

Component percentiles also cannot be added into a total percentile. Investigate
scope, timing, and duplicate accounting before clipping a ratio above 1.
Heap/residual/PSS comparisons show magnitude, not strict causal contribution.

Pressure-triggered snapshots favor high-memory conditions. Keep trigger cohorts
separate; entity weights cannot repair trigger-frequency or source-coverage
bias.

## 12. Sampling and cost

The global process draw enables all non-UI sources. The UI draw is conditional
on process selection, and its result is emitted on every event.

Scheduled page rows use a separate process draw, `page_scheduled_sample_rate`
(default 0.1), conditional on global selection and independent of UI collection.
It reduces scheduled-event volume without changing global snapshots, page
observation offsets, or the one early summary per page.

N, N/n, V, pool count, and residual weights correct only within-process entity
draws.
Weighted counts are not independent sample counts, and user UV cannot be
recovered by multiplying by an entity weight. Analyze different sampling
configurations separately.

VM, pool, and page scans aggregate values and select their samples; the selected
page key requires another page scan. The code uses linear `FindVM` lookups in a
normally small VM registry, so the page pass can cost O(P×V), plus O(P) for the
selected group. Reservoir and group accumulators use O(1) extra space; expired
VM cleanup uses a small inline vector. The VM registry stores stable pointees in
a `vector<unique_ptr>`; normal snapshots do not copy it. A first-overflow
trigger copies only its VM state once, then moves the owning pointer.

QuickJS arrays have a fixed 256 entries. Cached slot reads do not query an
engine; each unique V8/JSVM heap is queried on its owning thread.
Residency retains pending creation records until evaluation; suspended polling
can extend their lifetime.

## 13. Validation

Validate emitted events against independent workload truth, with both precise
boundary cases and sampled aggregate checks:

1. Shared-slot replacement, page/shared-heap deduplication, multiple JS threads,
   and concurrent same-key instances.
2. Unsupported JSC/MTS, disabled tracking, overflow, missing values, valid zero,
   VM expiration, and missing runners.
3. Uniform VM weights after age/idle filters, cross-engine bindings, and
   complete VM residuals.
4. Same-name/address VM replacement without inherited age, and owner-thread
   strong-reference release during synchronous collection.
5. Page capacity 253, first failure, deferred overflow emission, and failed-page cleanup.
6. Multi-bucket PSS jumps, late polls, trigger combinations, and frozen delivery.
7. Complete residency windows, early exits, exact-deadline exits, never-loaded
   pages, reused IDs, and out-of-order creation notifications.
8. Full-GC/exit notification ordering, pending residuals, later zeroing, and
   incomplete tracking history.
9. Byte-time clipping, skipped intervals, cache error, and selection introduced
   by quality filters.
10. Reporter latency and the absence of a reverse JS-to-reporter synchronous
    dependency.
11. Four MTS context types, valid zero pool heaps, template-URL page-key
    resolution, process accounting, lifecycle cleanup, and uniform pool
    estimators.

Sampled sums should be compared with independently computed truth and tolerances
chosen before observing results. Uniform VM selection has per-snapshot group
variance `V*SUM(x_i^2) - SUM(x_i)^2`, with zero contribution for nonmembers.
Validate ratio numerators and denominators separately before their quotient.

Simulation can establish consistency under its workload model, not actual
engine measurement accuracy, event delivery, or production population behavior.
Those require runtime validation.
