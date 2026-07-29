# ESP32-NUT documentation archive

This directory preserves completed, historical, and superseded documentation
that is no longer needed during normal agent startup. No archive file is an
active source of branch state.

Start with [AGENTS.md](../../AGENTS.md) and
[ESP32_CURRENT_STATUS.md](../ESP32_CURRENT_STATUS.md). Read an archive only
when a current document links to it or historical evidence is required.

## Archive inventory

| Archive | Preserved material |
| --- | --- |
| [AGENTS_SNAPSHOT_2026-07-29.md](AGENTS_SNAPSHOT_2026-07-29.md) | Complete agent instructions before the fast-start routing rewrite |
| [ESP32_CURRENT_STATUS_HISTORY.md](ESP32_CURRENT_STATUS_HISTORY.md) | Every line of the 101 KB pre-optimization status handoff, retained in original order |
| [ESP32_DEVELOPMENT_PLAN_COMPLETED_HISTORY.md](ESP32_DEVELOPMENT_PLAN_COMPLETED_HISTORY.md) | Completed foundation, OTA, release-slice, v2.7.0, and sequencing history removed from the active roadmap |
| [ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT_QA_RECORDS.md](ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT_QA_RECORDS.md) | Completed v2.7.0 QA and target-acceptance records |
| [ESP32_README_CHANGELOG.md](ESP32_README_CHANGELOG.md) | Superseded current-version summary from the detailed downstream README |
| [AGENT_STARTUP_CONTEXT_AUDIT_2026-07-29.md](AGENT_STARTUP_CONTEXT_AUDIT_2026-07-29.md) | Before/after startup-context measurements for this optimization |

## Archive rules

- Preserve chronology, headings, evidence labels, hashes, and original wording.
- Do not silently promote an archived observation into a current fact.
- Historical IP addresses, USB paths, branches, firmware, and releases must be
  rediscovered or verified before reuse.
- Do not edit an archived record merely to match current behavior. Add a
  current correction or a clearly dated archival note instead.
- Archives are excluded from the default startup set and should not be loaded
  wholesale into an agent context window.
