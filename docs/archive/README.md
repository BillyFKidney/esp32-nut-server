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
| [2026-07-29 target candidate smoke validation](2026-07-29-target-candidate-smoke-validation.md) | Browser-OTA, reboot, ADMIN-session, and network-service evidence for the local Wi-Fi-provisioning/header-limit integration candidate |
| [v2.7.1 validation evidence index](v2.7.1/evidence.md) | Index of the seven archived v2.7.1 planning, route, security, handoff, and validation records |
| [v2.7.2 validation evidence index](v2.7.2/evidence.md) | Source, automated, and physical acceptance evidence for disconnect invalidation and Agent diagnostics |
| [v2.7.4 validation evidence index](v2.7.4/evidence.md) | APC/CyberPower compatibility, stale/recovery, service-boundary, and token-isolation evidence |
| [v2.7.5 validation evidence index](v2.7.5/evidence.md) | USB HID/NUT compatibility hardening, tagged OTA validation, APC/CyberPower evidence, and release boundaries |
| [v2.7.6 validation evidence index](v2.7.6/evidence.md) | USB attachment-generation reprobe, APC/CyberPower replacement evidence, tagged OTA validation, and v2.7.6 release boundaries |
| [v2.7.7 validation evidence index](v2.7.7/evidence.md) | Release-confirmed Wi-Fi/factory reset, credential invalidation, fault injection, tagged OTA, and release boundaries |
| [ESP32_CURRENT_STATUS_V2_7_1.md](ESP32_CURRENT_STATUS_V2_7_1.md) | Full v2.7.1 pre-publication status and release-preparation handoff |
| [ESP32_64K_AGENT_HANDOFF_V2_7_1.md](ESP32_64K_AGENT_HANDOFF_V2_7_1.md) | Full v2.7.1 64k continuation packet before active-slice compaction |
| [ESP32_DEVELOPMENT_PLAN_V2_7_1.md](ESP32_DEVELOPMENT_PLAN_V2_7_1.md) | Frozen pre-v2.7.1 planning snapshot; active and future roadmap scope is maintained in `docs/ESP32_DEVELOPMENT_PLAN.md` |
| [ESP32_REFACTORING_PLAN_V2_7_1.md](ESP32_REFACTORING_PLAN_V2_7_1.md) | Full management/Wi-Fi extraction plan, module inventory, and completed validation record |
| [ESP32_ROUTE_INVENTORY_V2_7_1.md](ESP32_ROUTE_INVENTORY_V2_7_1.md) | Completed HTTPS management-route order and extraction acceptance baseline |
| [ESP32_README_V2_7_1.md](ESP32_README_V2_7_1.md) | Full downstream port, compatibility, troubleshooting, and changelog notes through v2.7.1 |
| [ESP32_SECURITY_V2_7_1.md](ESP32_SECURITY_V2_7_1.md) | Frozen detailed security guidance through v2.7.1, including generic and superseded material |
| [ESP32_DEVELOPMENT_PLAN_COMPLETED_HISTORY.md](ESP32_DEVELOPMENT_PLAN_COMPLETED_HISTORY.md#v271-repair-publication-and-validation-history) | Consolidated v2.7.1 HTTPS request-header repair, publication, and post-publication target validation evidence |
| [ESP32_REPOSITORY_LAYOUT_HISTORY.md](ESP32_REPOSITORY_LAYOUT_HISTORY.md) | Historical rationale for the 2026 repository-layout preservation decisions |

## Archive rules

- Preserve chronology, headings, evidence labels, hashes, and original wording.
- Do not silently promote an archived observation into a current fact.
- Historical IP addresses, USB paths, branches, firmware, and releases must be
  rediscovered or verified before reuse.
- Do not edit an archived record merely to match current behavior. Add a
  current correction or a clearly dated archival note instead.
- Archives are excluded from the default startup set and should not be loaded
  wholesale into an agent context window.
