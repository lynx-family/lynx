# Memory Monitor Documentation

This directory documents the Lynx memory monitoring module. The 
documents separate implementation design, event contracts, and
data-analysis guidance so that each concern can evolve without 
mixing collection details with query recipes.

## Documents

| Document | Purpose |
| --- | --- |
| [Global Memory Reporting Design](./memory_monitor_reporting_design.md) | Describes process-level collection and reporting, including active pages, residency, shared VMs, exited-page residuals, persistence, sampling, and PSS-based analysis. |
| [Page Memory Reporting Design](./page_memory_monitor_reporting_design.md) | Describes per-page scheduled observations and the once-per-instance early total/BTS/MTS mean and peak summaries. |
| [Memory Monitor Event Property Reference](./memory_monitor_property_registration.md) | Defines every emitted event property, its payload type, production gate, and conditional presence rule. |
| [Memory Monitor Query Guide](./memory_monitor_query_guide.md) | Provides supported filters, aggregates, formulas, denominators, and interpretation guidance for the monitoring questions. |

## Reading Paths

For implementation or review:

1. Read the relevant global or page reporting design.
2. Check the property reference for the resulting event contract.
3. Use the query guide to verify that the emitted data supports the intended
   analysis.

For data analysis:

1. Start with the property reference to identify valid fields and production
   gates.
2. Follow the query guide for formulas and cohort definitions.
3. Consult the reporting designs when timing, sampling, ownership, or cache
   freshness affects interpretation.

The C++ implementation is authoritative for runtime behavior. When event
schemas or collection semantics change, update the relevant design, property
reference, and query guidance together.
