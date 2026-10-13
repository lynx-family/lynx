#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Generate bounded, explicit workloads and independent census/query oracles.

No production-event implementation is imported or translated here. Truth is a
direct scan of page/VM intervals; sampling is exclusively done by the C++ test.
Times are integer milliseconds, memory inputs integer MiB, outputs bytes.
"""

import argparse
import collections
import hashlib
import json
import math
import random
from pathlib import Path

MIB = 1024 * 1024
OFFSETS = (0, 1000, 2000, 3000, 5000, 8000, 12000, 16000, 20000,
           40000, 60000, 120000, 300000, 600000, 1800000)
EARLY_WINDOW_MS = 20000
PAGE_IDS = ("app/shell", "global/nav", "feed/detail", "search/result",
            "shop/product", "media/heavy", "")
URLS = tuple("demo://open?actual_url=https%3A%2F%2Fexample.test%2F" +
             route.replace("/", "%2F") + "&source=home#content"
             for route in ("app/shell", "global/nav", "feed/detail",
                           "search/result", "shop/product", "media/heavy",
                           "article/detail"))
PAGE_KEYS = tuple(page_id or url for page_id, url in zip(PAGE_IDS, URLS))
NAMES = ("shared_feed", "shared_search", "recyclable")
PLATFORMS = ("android", "ios", "harmony", "pc")
# Page: kind,url,start,end,vm,peak_mib,residual_mib,residual_end,flags,unknown_pct.
# Flags: missing UI=1, no BTS=2, private MTS=4, short=8, late 5s=16, reload=32.
# VM: runtime,name,create,destroy,common_mib,tracking. Runtime qjs=0,jsc=1,v8=2,jsvm=3.
# The final VM column is a full-GC interval (0 means no full GC).


def last_full_gc(vm, at):
    interval = vm[6]
    return vm[2] + (at - vm[2]) // interval * interval if interval and at >= vm[2] + interval else -1


def exited_bytes(page, vm, at):
    # Delayed collection makes the pre-GC cache substantially larger. Later
    # full GCs also reclaim finite-lived residuals; some references persist.
    gc_at = last_full_gc(vm, at)
    if page[8] & 2:
        return 0
    if vm[0] == 0 and gc_at <= page[3]:
        return (page[6] + 32) * MIB
    return (0 if page[1] == 6 and gc_at >= page[7] else page[6] * MIB)


def quantile(values, p):
    values = sorted(values)
    return values[max(0, math.ceil(p * len(values)) - 1)] if values else 0


def stable_sum(values):
    total = 0
    for value in values:
        total += value
    return total


def size(page, at):
    age = at - page[2]
    percent = 100 if page[8] & 8 else (
        60 if age < 5000 else 80 if age < 10000 else
        100 if age < 20000 else 90 if age < 25000 else 85)
    return max(50 * MIB, page[5] * MIB * percent // 100)


def components(page, at):
    total = size(page, at)
    bts = 0 if page[8] & 2 else total * 4 // 10
    element, mts = total // 10, total // 5
    return element, mts, bts, total - element - mts - bts


def page_memory(s, index, at):
    page = s["pages"][index]
    element, mts, bts, ui = components(page, at)
    total = element
    if not page[8] & 4:
        total += mts
    if not page[8] & 1:
        total += ui
    vm = page[4]
    if not page[8] & 2:
        if vm < 0:
            total += bts
        elif (s["vms"][vm][0] == 0 and s["vms"][vm][5]
              and s["slots"][index] >= 3):
            total += bts - bts * page[9] // 100
    return total


def page_bts_memory(s, index, at):
    page = s["pages"][index]
    if page[8] & 2:
        return 0
    bts = components(page, at)[2]
    vm = page[4]
    if vm < 0:
        return bts
    if s["vms"][vm][0] == 0 and s["vms"][vm][5] and s["slots"][index] >= 3:
        return bts - bts * page[9] // 100
    return 0


def page_mts_memory(s, index, at):
    page = s["pages"][index]
    return 0 if page[8] & 4 else components(page, at)[1]


def make_session(rng, sid, platform, scenario="normal"):
    ram = (8, 8, 8, 8, 8, 12, 12, 12, 16, 16)[sid % 10]
    duration = rng.randint(60, 7200) * 1000
    oom = ((sid - 1) % 100 == (4, 6, 9, 2, 8)[((sid - 1) // 100) % 5]
           and scenario == "normal")
    if oom:
        duration = max(duration, 600000) // 8000 * 8000
    if scenario == "saturated":
        duration = 7200000
    # Keep heterogeneous runtimes so observable page totals exercise missing
    # MTS/BTS contributions without dropping those pages from the cohort.
    dominant = (1 if platform == "ios" and (sid // 10) % 5 else
                3 if platform == "harmony" and (sid // 10) % 5 else
                2 if platform in ("android", "pc") and sid % 7 == 0 else 0)
    if scenario == "heap_only":
        dominant = 3 if platform == "harmony" else 2
    if scenario in ("saturated", "short", "quality", "slot_off"):
        dominant = 0
    vms = [[dominant, 0, 0, duration + 1, 24, int(dominant == 0)],
           [0, 1, 0, duration + 1, 16, 1]]
    if scenario == "slot_off":
        vms[0][5] = vms[1][5] = 0
    pages = []
    for url, peak in ((0, 90), (1, 70)):
        pages.append([0, url, 0, duration + 1, 0, peak, 0, duration + 1, 0])
    # Independent renewal lanes keep typical concurrency bounded; two pinned
    # Lynx URLs are additional to the seven transient Lynx lanes.
    for kind, lanes in ((0, 7), (1, 13), (2, 3)):
        for lane in range(lanes):
            start = rng.randint(0, 12000)
            while start < duration:
                life = rng.randint(60000, 285000)
                if kind == 1:
                    life = rng.randint(80000, 310000)
                if rng.random() < .04:
                    life = rng.randint(300000, 1200000)
                if scenario == "saturated" and kind == 0:
                    life = 60000
                url = rng.choices(range(2, 7), (35, 18, 22, 10, 15))[0]
                if kind == 0 and lane < 4 and start < 12000:
                    url, life = 4, 420000
                mean = 215 if url == 5 else 105 if url == 4 else 145
                peak = (max(50, min(250, round(rng.gauss(mean, 24))))
                        if kind == 0 else rng.randint(20, 150)
                        if kind == 1 else rng.randint(80, 300))
                end = start + life
                residual = 0 if kind == 0 else rng.randint(5, 20 if kind == 1 else 30)
                cleanup = duration + 1 if oom else end + rng.randint(60000, 240000)
                pages.append([kind, url if kind == 0 else -1, start, end,
                              -1, peak, residual, cleanup, 0])
                start = end + rng.randint(0, 12000)
    pages.sort(key=lambda p: (p[2], p[0], p[1]))
    pool, pool_left = -1, 0
    for p in pages:
        if p[0] or p[4] == 0:
            continue
        if scenario == "saturated":
            p[4] = 0
        elif rng.random() < .09:
            # Standalone QuickJS. A private VM is never a global candidate.
            p[4] = -1
        elif p[1] == 3 and p[2] < duration // 2:
            p[4] = 1
        elif rng.random() < .55:
            p[4] = 0
        else:
            if pool_left == 0:
                runtime = dominant
                pool = len(vms)
                vms.append([runtime, 2, p[2], 0, 12, int(runtime == 0)])
                pool_left = rng.randint(2, 16)
            p[4] = pool
            pool_left -= 1
            vms[pool][3] = max(vms[pool][3], p[3] + 1)
    # RuntimeManager VMs share across business groups. Coalesce each heap-only
    # engine into at most one instance per session; recyclable group VMs stay
    # distinct even though their analytics names repeat.
    heap_vm = {}
    for p in pages:
        if p[0] == 0 and p[4] >= 0 and vms[p[4]][0] in (2, 3):
            runtime = vms[p[4]][0]
            owner = heap_vm.setdefault(runtime, p[4])
            p[4] = owner
            vms[owner][2] = min(vms[owner][2], p[2])
            vms[owner][3] = duration + 1
    used = sorted({p[4] for p in pages if p[0] == 0 and p[4] >= 0})
    remap = {old: new for new, old in enumerate(used)}
    vms = [vms[i] for i in used]
    for p in pages:
        if p[4] >= 0:
            p[4] = remap[p[4]]
    if scenario in ("short", "quality"):
        for p in pages:
            if p[0] == 0 and p[1] > 1:
                if scenario == "short":
                    p[3], p[8], p[5] = p[2] + 3000, 8, 240
                else:
                    p[8] = (1, 2, 4, 16, 32)[p[1] - 2]
    seen = set()
    for p in sorted(pages, key=lambda p: p[3]):
        if p[0] == 0 and p[4] >= 0:
            key = (p[4], p[1])
            p[6] = rng.randint(3, 8) if key in seen else rng.randint(10, 20)
            seen.add(key)
        if p[8] & 2:
            p[6] = 0
    # Slots are allocated at load, never reused, including after zero residual.
    allocations = collections.Counter()
    slots = []
    for p in pages:
        slot = -1
        if (p[0] == 0 and not (p[8] & 2) and p[4] >= 0
                and vms[p[4]][0] == 0 and vms[p[4]][5]):
            allocations[p[4]] += 1
            slot = allocations[p[4]] + 2 if allocations[p[4]] <= 253 else 2
        slots.append(slot)
    # Separate RNG preserves the lifecycle workload while extending accounting.
    slot_rng = random.Random(20260916 + 10000 * PLATFORMS.index(platform) + sid)
    for vm in vms:
        vm[4] = slot_rng.randint(4, 8)
        vm.append(60000 if vm[0] == 0 and scenario not in ("slot_off", "short") else 0)
    for p, slot in zip(pages, slots):
        # 3% of tracked page loads misroute 5..20% of their BTS allocations.
        # Their remaining allocations retain their own (possibly Overflow) slot.
        p.append(slot_rng.randint(5, 20)
                 if slot >= 2 and slot_rng.random() < .03 else 0)
    # PSS is a resident estimate, not the sum of allocator counters. Normal
    # host cleanup bounds foreign residuals. Selected stress sessions receive
    # a native allocation ramp, calibrated to cross RAM-4GiB at app termination.
    pools = [[0, 0, 0, 2, 12], [1, 1, 0, 1, 0],
             [0, 2, 0, 0, 8], [1, 3, 0, 3, 20]]
    session = dict(id=sid, ram=ram, duration=duration, scenario=scenario,
                   vms=vms, pages=pages, slots=slots, pools=pools,
                   pressure_mib=0, oom=False)
    end_pss = census(session, duration)["pss"]
    if oom:
        session["pressure_mib"] = max(
            0, math.ceil(((ram - 4) * 1024 * MIB - end_pss) / MIB) + 16)
    return session


def census(s, at):
    pages, vms, slots = s["pages"], s["vms"], s["slots"]
    live_vm = {i for i, vm in enumerate(vms) if vm[2] <= at < vm[3]}
    heaps = {i: vms[i][4] * MIB for i in live_vm}
    reserved = {i: [0, vms[i][4] * MIB, 0] for i in live_vm
                if vms[i][0] == 0 and vms[i][5]}
    attributed = collections.Counter()
    active, residual = [], []
    pending_gc = 0
    foreign = 0
    for i, p in enumerate(pages):
        if p[2] > at:
            continue
        alive = at < p[3]
        if p[0]:
            if alive:
                foreign += p[5] * MIB
            elif at < p[7]:
                foreign += p[6] * MIB
            continue
        vm = p[4]
        bts = (components(p, at)[2] if alive else
               exited_bytes(p, vms[vm], at) if vm >= 0 else 0)
        unknown = bts * p[9] // 100
        if vm in reserved:
            reserved[vm][0] += unknown
            if slots[i] == 2:
                reserved[vm][2] += bts - unknown
            elif slots[i] >= 3:
                attributed[vm] += bts - unknown
        if alive:
            parts = components(p, at)
            total = page_memory(s, i, at)
            if vm >= 0 and vm in live_vm:
                heaps[vm] += parts[2]
            active.append((i, total))
        elif vm in live_vm:
            heaps[vm] += bts
            if slots[i] >= 3:
                if last_full_gc(vms[vm], at) > p[3]:
                    residual.append((i, bts - unknown))
                else:
                    pending_gc += 1
    for vm, categories in reserved.items():
        assert stable_sum(categories) + attributed[vm] == heaps[vm]
    physical = stable_sum(heaps.values())
    accounted = stable_sum(value for vm, value in heaps.items() if vms[vm][0] != 1)
    pool_heap = stable_sum(pool[4] * MIB for pool in s["pools"]
                    if pool[2] <= at)
    physical += pool_heap
    accounted += pool_heap
    for i, total in active:
        p = pages[i]
        parts = components(p, at)
        attributed_bts = 0
        if (not p[8] & 2 and p[4] >= 0 and vms[p[4]][0] == 0
                and vms[p[4]][5] and slots[i] >= 3):
            attributed_bts = parts[2] - parts[2] * p[9] // 100
        physical += stable_sum(parts) - (parts[2] if p[4] >= 0 else 0)
        accounted += total - attributed_bts
    pss = 256 * MIB + (physical + foreign) * 3 // 5
    pss += s["pressure_mib"] * MIB * at // s["duration"]
    return dict(pss=pss, accounted=accounted, active=active,
                heaps=heaps, residual=residual, reserved=reserved, pending_gc=pending_gc,
                physical=physical, foreign=foreign)


def add_stat(stats, key, truth, variance=0):
    entry = stats.setdefault(key, [0.0, 0.0])
    entry[0] += truth
    entry[1] += max(0, variance)

def add_uniform_vm_stat(oracle, metric, s, population, quantities):
    # Same-name candidates are mutually exclusive in one uniform VM draw.
    groups = collections.defaultdict(list)
    for vm, value in quantities.items():
        groups[vm_name(s["vms"][vm])].append(value)
    for name, values in groups.items():
        total = stable_sum(values)
        add_stat(oracle[metric], name, total,
                 population * stable_sum(x * x for x in values) - total * total)


def reference(s, oracle):
    # Snapshots are generated at the same virtual boundaries as the runtime:
    # 8s polls, 300s timers, first slot overflow and a pressure warning.
    triggers = collections.defaultdict(lambda: [0, -1])
    counts = collections.Counter()
    for i, p in enumerate(s["pages"]):
        if p[0] == 0 and s["slots"][i] != -1:
            counts[p[4]] += 1
            n = counts[p[4]]
            if n == 254:
                triggers[(p[2], i)] = [8, i]
    next_timer, high = 300000, 0
    checkpoints = []
    stop = s["duration"]
    for at in range(8000, s["duration"] + 1, 8000):
        c = census(s, at)
        if c["pss"] >= (s["ram"] - 4) * 1024 * MIB:
            stop, s["oom"] = at, True
            break  # OOM has no guaranteed final event or page summary.
        if at == s["duration"]:
            break
        reason, before = 0, high
        if at >= next_timer:
            reason |= 1
            next_timer += 300000
        if c["pss"] // (256 * MIB) > high:
            high = c["pss"] // (256 * MIB)
            reason |= 2
        if reason:
            checkpoints.append([at, reason, -1, before * 256 * MIB, high * 256 * MIB])
    if s["pressure_mib"]:
        triggers[(max(1, stop - 8000), -1)] = [4, -1]
    for (at, _), (reason, page) in triggers.items():
        if at < stop:
            checkpoints.append([at, reason, page, 0, 0])
    checkpoints.sort(key=lambda row: (row[0], row[2]))
    s["stop"] = stop
    previous = 0
    for row in checkpoints:
        at, reason = row[:2]
        c = census(s, at)
        vm_count = len(c["heaps"])
        quickjs_count = stable_sum(s["vms"][vm][0] == 0 for vm in c["heaps"])
        heaps = {i: b for i, b in c["heaps"].items() if s["vms"][i][0] != 1}
        heap_total, residual_total = stable_sum(heaps.values()), stable_sum(x[1] for x in c["residual"])
        row.extend([c["pss"], c["accounted"], heap_total, residual_total,
                    len(c["active"]), c["pending_gc"],
                    len(c["residual"])])
        # Q9: the trigger VM is deterministic, not the independently drawn VM.
        # Derive age/load count from raw intervals, never from emitted fields.
        if row[2] >= 0:
            page = s["pages"][row[2]]
            owner = page[4]
            name = vm_name(s["vms"][owner])
            loads = stable_sum(p[0] == 0 and p[4] == owner and not (p[8] & 2)
                        and p[2] <= at for p in s["pages"])
            if reason & 8:
                oracle["slot_overflow_age"].setdefault(name, []).append(
                    at - s["vms"][owner][2])
                oracle["slot_overflow_loads"].setdefault(name, []).append(loads)
        # All-trigger byte-time uses clipped endpoints, never heap*age.
        begin = max(previous, at - 300000)
        if previous:
            add_uniform_vm_stat(oracle, "vm_area", s, vm_count, {
                vm: b * (at - max(begin, s["vms"][vm][2]))
                for vm, b in heaps.items()})
            by_url = collections.defaultdict(list)
            for i, b in c["residual"] if not c["pending_gc"] else []:
                by_url[PAGE_KEYS[s["pages"][i][1]]].append((b, at - max(begin, s["pages"][i][3])))
            for url, items in by_url.items():
                truth = stable_sum(b * dt for b, dt in items)
                second = residual_total * stable_sum(b * dt * dt for b, dt in items)
                add_stat(oracle["residual_area"], url, truth, second - truth * truth)
        previous = at
        if reason & 2:
            for gib in (1, 2, 3):
                if row[3] < gib * 1024 * MIB <= row[4]:
                    key = f'{s["ram"]}/{gib}'
                    oracle["ratios"].setdefault(key, []).append(c["accounted"] / c["pss"])
        if not reason & 1:
            continue
        pool_count = stable_sum(pool[2] <= at for pool in s["pools"])
        pool_groups = collections.defaultdict(list)
        for pool in s["pools"]:
            if pool[2] <= at:
                pool_groups[f"{PAGE_KEYS[pool[0]]}|{pool[1]}"].append(pool)
        for key, items in pool_groups.items():
            pool_heaps = [pool[4] * MIB for pool in items]
            runtimes = [pool[3] for pool in items]
            heap_total = stable_sum(pool_heaps)
            runtime_total = stable_sum(runtimes)
            add_stat(oracle["mts_pool_heap"], key, heap_total,
                     pool_count * stable_sum(value * value for value in pool_heaps)
                     - heap_total * heap_total)
            add_stat(oracle["mts_pool_runtime"], key, runtime_total,
                     pool_count * stable_sum(value * value for value in runtimes)
                     - runtime_total * runtime_total)
            add_stat(oracle["mts_pool_weight"], key, len(items),
                     pool_count * len(items) - len(items) ** 2)
        n = len(c["active"])
        groups = collections.defaultdict(list)
        for item in c["active"]:
            groups[PAGE_KEYS[s["pages"][item[0]][1]]].append(item)
        for url, items in groups.items():
            resident = [item[1] for item in items
                        if at - s["pages"][item[0]][2] >= 600000]
            if resident:
                total = stable_sum(resident)
                add_stat(oracle["resident_bytes"], url, total,
                         n * stable_sum(x * x for x in resident) - total * total)
                add_stat(oracle["resident_memory_weight"], url, len(resident),
                         n * len(resident) - len(resident) ** 2)
            total = stable_sum(item[1] for item in items)
            add_stat(oracle["url_bytes"], url, total,
                     total * total * (n / len(items) - 1))
            add_stat(oracle["url_weight"], url, 1, n / len(items) - 1)
            add_stat(oracle["url_count"], url, len(items),
                     len(items) ** 2 * (n / len(items) - 1))
        old = {vm: b for vm, b in heaps.items()
               if at - s["vms"][vm][2] >= 1800000 and b}
        idle, reuse, retained_vm = {}, {}, {}
        for vm in c["heaps"]:
            bound = [(i, p) for i, p in enumerate(s["pages"])
                     if p[0] == 0 and p[4] == vm and not p[8] & 2 and p[2] <= at]
            reuse[vm] = len(bound)
            if vm in old:
                bound_pages = [p for _, p in bound]
                if not any(p[3] > at for p in bound_pages):
                    idle_at = max((p[3] for p in bound_pages), default=s["vms"][vm][2])
                    if at - idle_at >= 600000:
                        idle[vm] = old[vm]
            gc_at = last_full_gc(s["vms"][vm], at)
            if (s["vms"][vm][0] == 0 and s["vms"][vm][5] and gc_at > 0
                    and stable_sum(s["slots"][i] >= 3 for i, _ in bound) < 253
                    and not any(s["slots"][i] == 2 for i, _ in bound)
                    and not any(p[3] <= at and (s["slots"][i] < 3 or gc_at <= p[3])
                                for i, p in bound)):
                retained_vm[vm] = stable_sum(b for i, b in c["residual"]
                                      if s["pages"][i][4] == vm)
        for metric, quantities, population in (
                ("old_vm", old, vm_count),
                ("old_vm_weight", {vm: 1 for vm in old}, vm_count),
                ("idle_vm", idle, vm_count),
                ("vm_reuse", reuse, vm_count),
                ("vm_reuse_weight", {vm: 1 for vm in reuse}, vm_count),
                ("vm_retained", retained_vm, quickjs_count),
                ("vm_retained_weight", {vm: 1 for vm in retained_vm},
                 quickjs_count)):
            add_uniform_vm_stat(oracle, metric, s, population, quantities)
        residual_groups = collections.defaultdict(list)
        for i, b in c["residual"]:
            residual_groups[PAGE_KEYS[s["pages"][i][1]]].append(b)
        for url, members in residual_groups.items():
            b = stable_sum(members)
            add_stat(oracle["residual"], url, b, b * (residual_total - b))
            # Only positive slots can be drawn. Same-key candidates are
            # mutually exclusive: E[W^2] = T*SUM(1/L_i), E[W] = count.
            positive = [value for value in members if value > 0]
            count = len(positive)
            add_stat(oracle["residual_weight"], url, count,
                     residual_total * stable_sum(1 / value for value in positive) - count ** 2)
    # Page oracles include every observed unique summary, never event-count
    # weighting. All normal pages are constant after 25s.
    for i, p in enumerate(s["pages"]):
        if p[0] or p[2] >= stop:
            continue
        if p[2] + 600000 < stop:
            key = PAGE_KEYS[p[1]]
            add_stat(oracle["resident_weight"], key, 1)
            add_stat(oracle["resident"], key, int(p[3] > p[2] + 600000))
        end = min(p[3], stop)
        target = min(end - p[2], EARLY_WINDOW_MS)
        vm = p[4]
        at_5s = p[2] + 5000 + (1001 if p[8] & 16 else 0)
        # Q2: completed fixed-age observations survive even when an OOM
        # prevents this page's later summary from being delivered.
        if not (p[8] & 4) and at_5s < end:
            runtime = ("quickjs", "jsc", "v8", "jsvm")[
                s["vms"][vm][0] if vm >= 0 else 0]
            key = PAGE_KEYS[p[1]] + "|" + runtime
            oracle["page_mts_5s"].setdefault(key, []).append(
                components(p, at_5s)[1])
        if at_5s < end:
            oracle["page_5s"].setdefault(PAGE_KEYS[p[1]], []).append(
                page_memory(s, i, at_5s))
        if s["oom"] and end == stop and target < EARLY_WINDOW_MS:
            continue
        # Exit is ordered before a sample at the exact same timestamp.
        due = [t for t in OFFSETS if t <= target]
        values = [
            page_memory(s, i, p[2] + t + (1001 if t == 5000 and p[8] & 16 else 0))
            for t in due
            if p[2] + t + (1001 if t == 5000 and p[8] & 16 else 0) < end
        ]
        bts_values = [
            page_bts_memory(
                s, i, p[2] + t + (1001 if t == 5000 and p[8] & 16 else 0))
            for t in due
            if p[2] + t + (1001 if t == 5000 and p[8] & 16 else 0) < end
        ]
        mts_values = [
            page_mts_memory(
                s, i, p[2] + t + (1001 if t == 5000 and p[8] & 16 else 0))
            for t in due
            if p[2] + t + (1001 if t == 5000 and p[8] & 16 else 0) < end
        ]
        valid = len(values) == len(due)
        if valid:
            oracle["page_mean"].setdefault(PAGE_KEYS[p[1]], []).append(stable_sum(values) / len(values))
            oracle["page_peak"].setdefault(PAGE_KEYS[p[1]], []).append(max(values))
            oracle["page_bts_mean"].setdefault(PAGE_KEYS[p[1]], []).append(
                stable_sum(bts_values) / len(bts_values))
            oracle["page_bts_peak"].setdefault(PAGE_KEYS[p[1]], []).append(
                max(bts_values))
            oracle["page_mts_mean"].setdefault(PAGE_KEYS[p[1]], []).append(
                stable_sum(mts_values) / len(mts_values))
            oracle["page_mts_peak"].setdefault(PAGE_KEYS[p[1]], []).append(
                max(mts_values))
            if target < 5000:
                oracle["page_short"].setdefault(PAGE_KEYS[p[1]], []).append(
                    stable_sum(values) / len(values))
                oracle["page_peak_short"].setdefault(PAGE_KEYS[p[1]], []).append(max(values))
                oracle["page_bts_short"].setdefault(PAGE_KEYS[p[1]], []).append(
                    stable_sum(bts_values) / len(bts_values))
                oracle["page_bts_peak_short"].setdefault(PAGE_KEYS[p[1]], []).append(
                    max(bts_values))
                oracle["page_mts_short"].setdefault(PAGE_KEYS[p[1]], []).append(
                    stable_sum(mts_values) / len(mts_values))
                oracle["page_mts_peak_short"].setdefault(PAGE_KEYS[p[1]], []).append(
                    max(mts_values))
    return checkpoints


def vm_name(vm):
    return "runtime_manager/v8" if vm[0] == 2 else (
        "runtime_manager/jsvm" if vm[0] == 3 else NAMES[vm[1]])


def generate(platform, count):
    rng = random.Random(20260915 + PLATFORMS.index(platform))
    oracle = {key: {} for key in ("resident", "resident_weight", "url_bytes", "url_weight", "url_count",
                                  "old_vm", "old_vm_weight", "idle_vm",
                                  "resident_bytes", "resident_memory_weight",
                                  "vm_reuse", "vm_reuse_weight",
                                  "vm_retained", "vm_retained_weight",
                                  "residual", "residual_weight", "vm_area", "residual_area",
                                  "page_mean", "page_short", "page_peak", "page_peak_short",
                                  "page_bts_mean", "page_bts_short",
                                  "page_bts_peak", "page_bts_peak_short",
                                  "page_mts_mean", "page_mts_short",
                                  "page_mts_peak", "page_mts_peak_short",
                                  "page_5s", "page_mts_5s", "ratios",
                                  "slot_overflow_age", "slot_overflow_loads",
                                  "mts_pool_heap", "mts_pool_runtime",
                                  "mts_pool_weight")}
    sessions = []
    stats = collections.Counter()
    normal_concurrency = [[], [], []]
    normal_memory = [[], [], []]
    normal_lifetimes = []
    runtime_counts = collections.Counter()
    for sid in range(count + 6):
        scenario = ("normal" if sid < count else
                    ("short", "quality", "slot_off", "heap_only", "saturated", "normal")[sid - count])
        s = make_session(rng, sid + 1, platform, scenario)
        s["checkpoints"] = reference(s, oracle)
        # Basic workload assertions are independent of monitor telemetry.
        if sid < count:
            stats[f'ram_{s["ram"]}'] += 1
            stats["oom"] += s["oom"]
            assert 60000 <= s["duration"] <= 7200000
            for at in range(60000, s["stop"], 60000):
                for kind in range(3):
                    normal_concurrency[kind].append(stable_sum(
                        p[0] == kind and p[2] <= at < p[3] for p in s["pages"]))
        for p in s["pages"]:
            stats[("lynx", "native", "webview")[p[0]]] += 1
            if p[0] == 0:
                stats["shared"] += p[4] >= 0
                stats["short"] += p[3] - p[2] < 60000
                stats["under_5m"] += p[3] - p[2] < 300000
                stats["unknown_pages"] += p[9] > 0
            if sid < count:
                normal_memory[p[0]].append(p[5])
                normal_lifetimes.append(p[3] - p[2])
                if not p[0]:
                    runtime_counts[str(s["vms"][p[4]][0] if p[4] >= 0 else 0)] += 1
        stats["snapshots"] += len(s["checkpoints"])
        stats["tracked_pages"] += stable_sum(slot >= 2 for slot in s["slots"])
        assert all(4 <= vm[4] <= 8 for vm in s["vms"])
        sessions.append(s)
    assert stats["oom"] == count // 100, (platform, stats)
    assert stats["shared"] > .8 * stats["lynx"]
    assert stats["under_5m"] > .5 * stats["lynx"]
    assert 0 < stats["unknown_pages"] < .05 * stats["tracked_pages"]
    for ram, proportion in ((8, .5), (12, .3), (16, .2)):
        assert stats[f"ram_{ram}"] == count * proportion
    assert all(50 <= x <= 250 for x in normal_memory[0])
    assert all(20 <= x <= 150 for x in normal_memory[1])
    assert all(80 <= x <= 300 for x in normal_memory[2])
    assert stable_sum(x < 300000 for x in normal_lifetimes) > .5 * len(normal_lifetimes)
    stats["concurrency_mean"] = [stable_sum(v) / len(v) for v in normal_concurrency]
    for actual, low, high in zip(stats["concurrency_mean"], (5, 10, 2), (10, 15, 5)):
        assert low <= actual <= high
    stats["normal_runtime_page_counts"] = dict(runtime_counts)
    stats["memory_mean_mib"] = [stable_sum(v) / len(v) for v in normal_memory]
    # Compact aggregate order-statistic references, retaining exact counts.
    for key in ("page_mean", "page_short", "page_peak", "page_peak_short",
                "page_bts_mean", "page_bts_short", "page_bts_peak",
                "page_bts_peak_short", "page_mts_mean", "page_mts_short",
                "page_mts_peak", "page_mts_peak_short", "page_5s",
                "page_mts_5s", "ratios",
                "slot_overflow_age", "slot_overflow_loads"):
        oracle[key] = {k: [len(v), stable_sum(v) / len(v), quantile(v, .50),
                           quantile(v, .90), quantile(v, .95), quantile(v, .99)]
                       for k, v in oracle[key].items()}
    return dict(format_version=15, event_schema=10, platform=platform,
                seed=20260915 + PLATFORMS.index(platform), normal_users=count,
                urls=URLS, page_ids=PAGE_IDS, vm_names=NAMES, page_columns=[
                    "kind", "url", "start_ms", "end_ms", "vm", "peak_mib",
                    "residual_mib", "residual_end_ms", "flags", "unknown_pct"],
                vm_columns=["runtime", "name", "create_ms", "destroy_ms", "common_mib", "tracking",
                            "full_gc_interval_ms"],
                pool_columns=["url", "context_type", "create_ms",
                              "runtime_count", "heap_mib"],
                checkpoint_columns=["time_ms", "reasons", "slot_page", "pss_before",
                                    "pss_after", "pss", "accounted", "heap",
                                    "tracked_residual", "active_count", "pending_gc_count",
                                    "confirmed_exited_count"],
                stats=dict(stats), oracle=oracle, sessions=sessions)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--users-per-platform", type=int, default=500)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    assert args.users_per_platform >= 100 and args.users_per_platform % 100 == 0
    for platform in PLATFORMS:
        data = generate(platform, args.users_per_platform)
        content = (json.dumps(data, separators=(",", ":"), sort_keys=True) + "\n").encode()
        assert len(content) < 15_000_000, (platform, len(content))
        target = Path(__file__).with_name(f"simulation_{platform}.json")
        if args.check:
            assert target.read_bytes() == content, f"Regenerate {target}"
        else:
            target.write_bytes(content)
        print(platform, len(content), hashlib.sha256(content).hexdigest(), data["stats"], flush=True)
    print("All four fixtures validated" if args.check else "All four fixtures generated", flush=True)


if __name__ == "__main__":
    main()
