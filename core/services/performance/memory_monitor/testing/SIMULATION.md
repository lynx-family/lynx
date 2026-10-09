# Memory Monitor Simulation Fixtures

These fixtures are deterministic synthetic workloads used to validate the
memory monitor with many page, VM, and process lifecycles. They are not user
recordings and their distributions are not production estimates.

Each JSON file contains:

- lifecycle inputs for replay;
- virtual memory checkpoints;
- independent population oracles computed directly from those lifecycles.

The files do not contain prebuilt telemetry. The C++ replay drives the production
memory monitor, collects its emitted events, and compares event-only aggregates
with the independent oracles.

The current fixtures use `format_version=15` and event `schema_version=10`.
Metric definitions and query formulas belong in
the [memory monitor documentation](../docs/README.md), not in this document.

## Files and Scale

- `simulation_android.json`, `simulation_ios.json`,
  `simulation_harmony.json`, and `simulation_pc.json`: one workload shard per
  platform model.
- `generate_simulation.py`: standard-library-only deterministic generator.
- `memory_monitor_unittest.cc`: virtual-time replay through production monitor
  entry points and event builders.
- `memory_simulation_test_support.h`: event-only aggregation and oracle
  verification.

Each shard contains 500 normal sessions and six edge sessions. The generated
files are kept below 15,000,000 bytes.

| Platform | Sessions | Memory checkpoints | Lynx intervals |
| --- | ---: | ---: | ---: |
| Android | 506 | 8,262 | 66,755 |
| iOS | 506 | 8,126 | 65,268 |
| Harmony | 506 | 8,462 | 67,358 |
| PC | 506 | 8,226 | 66,485 |

The local `.gitattributes` stores only `simulation_*.json` through Git LFS.
Missing or unhydrated fixtures fail the test. The generator does not stage or
commit generated files.

`memory_simulation_data` copies all four fixtures to the build output directory,
so tests can run from either the repository root or the output directory.

## Generate and Run

```bash
git lfs pull --include="core/services/performance/memory_monitor/testing/simulation_*.json"
tools/env.sh python3 core/services/performance/memory_monitor/testing/generate_simulation.py --check
tools/env.sh ninja -C out/Default memory_monitor_test_exec
tools/env.sh out/Default/memory_monitor_test_exec \
  --gtest_filter='MemoryMonitorTest.Simulated*'
```

Omit `--check` to regenerate all four fixtures. The default seeds are
`20260915 + platform_index`. Any generation-rule or format change requires
regenerating every shard.

To test the copied resources directly:

```bash
cd out/Default
./memory_monitor_test_exec --gtest_filter='MemoryMonitorTest.Simulated*'
```

## Workload Model

### Sessions and page lifecycles

Normal sessions last from 1 minute to 2 hours. RAM is assigned in an exact
50%/30%/20% split across 8/12/16 GiB devices.

Each session contains:

- two pinned Lynx pages that remain until session termination;
- seven renewing Lynx lanes;
- thirteen renewing Native lanes;
- three renewing WebView lanes.

Most finite page lifetimes are shorter than five minutes. Pages still active at
session termination are right-censored. Four initial `shop/product` pages create
same-key concurrency in otherwise independent page lifecycles.

Renewal lanes start within the first 12 seconds. Normal Lynx lifetimes are
60..285 seconds, Native lifetimes are 80..310 seconds, and 4% of pages use a
300..1200-second long lifetime. A completed page is followed by a 0..12-second
gap before the next page in that lane.

The page identities are fixed across shards. A nonempty host page ID is the page
key; the `article/detail` case has an empty host page ID and therefore uses its
complete wrapper URL.

### Page memory

Nominal peak memory is generated from these ranges:

- Lynx: URL-conditioned truncated normal values in 50..250 MiB;
- Native: uniform values in 20..150 MiB;
- WebView: uniform values in 80..300 MiB.

Normal Lynx pages follow a deterministic load curve: 60% of peak before 5s, 80%
before 10s, 100% before 20s, 90% before 25s, and 85% afterward, with a 50 MiB
floor. `media/heavy` uses a higher peak distribution.

For a normal Lynx page, the generated total is divided into Element 10%, MTS
20%, BTS 40%, and UI 30%. Edge flags can omit UI, MTS, or BTS records. Shared
QuickJS page attribution uses the page's current slot value rather than the
whole VM heap.

Native and WebView pages leave 5..20 MiB and 5..30 MiB residuals after exit.
Normal sessions reclaim those residuals after 1..4 minutes. Pressure sessions
retain them until process termination.

### VMs, slots, and full GC

Most Lynx pages use shared VMs; a minority use private QuickJS. Android and PC
favor QuickJS with some V8, iOS favors JSC, and Harmony favors JSVM. These
ratios exist only to exercise runtime capability differences.

Shared VMs have 4..8 MiB of base engine memory. Recyclable VMs accept batches
of page loads and may share an analytics name while remaining distinct VM
objects. V8 and JSVM RuntimeManager pages are coalesced per runtime within a
session.

Tracked QuickJS VMs normally complete a full GC every 60 seconds. An exited page
keeps an additional 32 MiB until the first full GC strictly after its exit.
After that GC, the first exit of a page key in a VM retains 10..20 MiB and later
exits of the same key retain 3..8 MiB. `article/detail` residuals are later
reclaimed; other generated QuickJS residuals remain until VM destruction.

Tracked page slots are allocated at load and never reused:

- slots 0/1/2 are Unknown/Common/Overflow;
- slots 3..255 identify individual pages;
- after 253 page slots, new allocations use Overflow;
- about 3% of tracked pages route 5..20% of BTS memory to Unknown.

The reserved slots and page-attributed slots always partition the complete
tracked QuickJS heap. Pages without a post-exit full GC remain pending rather
than being treated as zero residual.

Each session also contains four bundle-local MTS pools covering all context
types, multiple runtime counts, positive heaps, and a valid zero heap.

### Virtual observation timeline

Page observations use offsets of 0, 1, 2, 3, 5, 8, 12, 16, 20, 40, 60, 120,
300, 600, and 1800 seconds after first load.

The generator scans process state every 8 seconds and records a memory
checkpoint when one of these conditions occurs:

- the 300-second periodic timer is due;
- PSS reaches a new 256 MiB high-water bucket;
- a synthetic pressure warning occurs;
- a QuickJS VM first exhausts its page slots.

Residency is evaluated once at page creation plus 10 minutes, but only when the
synthetic process survives long enough to complete that observation window.

### PSS and process termination

Synthetic PSS is:

```text
256 MiB + 0.6 * (physical Lynx memory + foreign memory/residuals)
        + pressure-session Native ramp
```

This formula is a workload model, not a platform memory calibration. The
synthetic termination threshold is RAM minus 4 GiB. Exactly five of the 500
normal sessions in each shard receive a pressure ramp that crosses this
threshold. Terminated sessions do not emit an assumed final snapshot or
unfinished page summary.

### Edge sessions

The six additional sessions cover:

- short high-memory pages;
- missing UI, unsupported MTS, disabled BTS, delayed samples, and reload;
- disabled QuickJS slot tracking;
- heap-only V8 or JSVM collection;
- QuickJS slot saturation and overflow;
- one additional normal session.

## Current Fixture Format

Top-level fields describe the shard:

- `format_version`, `event_schema`, `platform`, `seed`, and `normal_users`;
- `urls`, `page_ids`, and `vm_names` identity dictionaries;
- `page_columns`, `vm_columns`, `pool_columns`, and `checkpoint_columns`;
- `stats`, `oracle`, and `sessions`.

Rows are compact arrays. Their current layouts are:

```text
page:
  kind, url, start_ms, end_ms, vm, peak_mib, residual_mib,
  residual_end_ms, flags, unknown_pct

vm:
  runtime, name, create_ms, destroy_ms, common_mib, tracking,
  full_gc_interval_ms

pool:
  url, context_type, create_ms, runtime_count, heap_mib

checkpoint:
  time_ms, reasons, slot_page, pss_before, pss_after, pss, accounted,
  heap, tracked_residual, active_count, pending_gc_count,
  confirmed_exited_count
```

Array indices are stable IDs within one session. Times are relative integer
milliseconds. Page, VM, and pool memory inputs use integer MiB; checkpoint and
oracle memory values use bytes.

Encoded values:

- page kind: Lynx=0, Native=1, WebView=2;
- page VM: `-1` means private QuickJS;
- VM runtime: QuickJS=0, JSC=1, V8=2, JSVM=3;
- MTS context: VM=0, LepusNG=1, RTS=2, RTSNative=3;
- page flags: missing UI=1, no BTS=2, private MTS=4, short=8,
  delayed 5s sample=16, reload=32;
- slot: -1 untracked, 2 overflow, 3..255 page slot;
- checkpoint reason bits: timer=1, PSS=2, warning=4, overflow=8.

Each session stores its lifecycle arrays, slots, checkpoints, RAM, duration,
stop time, pressure input, scenario, and termination result.

The generator derives `oracle` by scanning these lifecycles independently of
the C++ event output:

- sampled-sum entries are `[population expectation, sampling variance]`;
- distribution entries are `[count, mean, P50, P90, P95, P99]`;
- percentiles use nearest rank `ceil(p*N)-1`.

Native and WebView rows affect synthetic PSS and process termination, but they
are not replayed as Lynx instances.

## Replay and Verification

The replay injects virtual creation, load, sample, GC, checkpoint, and exit
times through the existing test friend boundary. Registration, destruction,
memory-record updates, slot allocation, VM updates, snapshot construction, and
event delivery use production code. A two-hour synthetic session therefore
completes without real-time waiting.

All emitted events pass through `EventTracker`. The test-side analytics layer
can read only event properties plus synthetic platform and RAM metadata; it
cannot read replay page/VM objects or checkpoint truth.

Exact populations and distributions are compared directly. Reservoir-sampled
sums use the generator's precomputed variance with a fixed regression bound of
six standard deviations plus floating-point allowance. The seed, truth, and
tolerance are fixed before sampled C++ results are observed.

The replay validates the complete memory query contract across all four
platform models. Focused unit tests separately cover timing boundaries,
scheduled-delivery sampling, lifecycle cleanup, shared-BTS replacement,
runtime capability gaps, slot saturation, full-GC ordering, and sampling
weights.

This validates production event construction and event-only aggregation against
a synthetic model. It does not validate platform memory collectors, real device
traffic distributions, or data-platform execution.

For the authoritative documentation index, reporting semantics, fields, and
query formulas, see:

- [Documentation index](../docs/README.md)
- [Global memory reporting design](../docs/memory_monitor_reporting_design.md)
- [Page memory reporting design](../docs/page_memory_monitor_reporting_design.md)
- [Event property reference](../docs/memory_monitor_property_registration.md)
- [Query guide](../docs/memory_monitor_query_guide.md)
