# Styling benchmarks

These Google Benchmark targets provide a repeatable performance baseline for
the Fiber styling and restyle pipeline:

- `css_invalidation_benchmark`: invalidation-index lookup and descendant Fiber
  invalidation traversal.
- `selector_matcher_benchmark`: selector matching costs for simple, compound,
  descendant, child, adjacent-sibling, and general-sibling selectors.
- `css_variable_handler_benchmark`: isolated CSS custom-property lookup and
  resolution costs.
- `fiber_restyle_benchmark`: end-to-end class and CSS-variable mutations from
  invalidation through style resolution, including legacy/new styling
  pipelines, representative parallel traversal, rule volume, and adopted
  stylesheets.

The synthetic trees and selectors are deterministic. The main tree sizes are
127, 1023, and 4095 nodes; sparse cases target every eighth node. Setup,
fixture validation, and result checking are outside timed loops.

## Build

Initialize the repository toolchain as described by the repository setup
instructions, generate a release-like unit-test build, and build the four
targets:

```bash
tools/env.sh gn gen out/Default \
  --args='enable_unittests=true is_debug=false use_flutter_cxx=false'
tools/env.sh ninja -C out/Default \
  css_invalidation_benchmark \
  selector_matcher_benchmark \
  css_variable_handler_benchmark \
  fiber_restyle_benchmark
```

## Record a baseline

Use the same machine, power mode, build arguments, and benchmark filters for
both revisions. Avoid running other CPU-intensive work. Ten repetitions expose
variance and retain enough raw samples for the comparison tool's statistical
test:

```bash
mkdir -p out/benchmark-baselines

out/Default/css_invalidation_benchmark \
  --benchmark_repetitions=10 \
  --benchmark_out=out/benchmark-baselines/invalidation-before.json \
  --benchmark_out_format=json

out/Default/selector_matcher_benchmark \
  --benchmark_repetitions=10 \
  --benchmark_out=out/benchmark-baselines/matcher-before.json \
  --benchmark_out_format=json

out/Default/css_variable_handler_benchmark \
  --benchmark_repetitions=10 \
  --benchmark_out=out/benchmark-baselines/variable-before.json \
  --benchmark_out_format=json

out/Default/fiber_restyle_benchmark \
  --benchmark_repetitions=10 \
  --benchmark_out=out/benchmark-baselines/restyle-before.json \
  --benchmark_out_format=json
```

Capture a matching `*-after.json` set after the optimization. The upstream
comparison tool depends on SciPy; install its declared requirements into an
ignored, repository-local virtual environment, then compare each pair. `-a`
limits terminal output to aggregates while retaining raw samples for the
Mann-Whitney U test:

```bash
tools/env.sh python3 -m venv out/benchmark-tools-venv
out/benchmark-tools-venv/bin/pip install \
  -r third_party/benchmark/tools/requirements.txt

out/benchmark-tools-venv/bin/python \
  third_party/benchmark/tools/compare.py -a benchmarks \
  out/benchmark-baselines/restyle-before.json \
  out/benchmark-baselines/restyle-after.json
```

Prefer the reported median for decisions and inspect coefficient of variation
or repeated-run spread before attributing small changes to code. Use stored JSON
results to track trends; these benchmarks intentionally do not impose a hard
pass/fail threshold.

Useful filters for quicker iteration include
`--benchmark_filter=BM_FiberClassInvalidation`,
`--benchmark_filter=BM_SelectorMatcherDescendant`, and
`--benchmark_filter=BM_FiberRestyleClass/NewDescendantSparse`.
