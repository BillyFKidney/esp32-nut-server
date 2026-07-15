# {{PROJECT_NAME}} current status

This is the operational handoff for the active development session. Keep it
short, factual, timestamped, and safe to share. Link to stable plans rather than
duplicating them.

**Updated:** {{DATE_TIME_AND_TIME_ZONE}}

## Snapshot

| Field | Current value |
| --- | --- |
| Active milestone | {{MILESTONE}} |
| Active slice/branch | `{{BRANCH}}` |
| Validated implementation commit | `{{COMMIT_OR_NOT_YET_VALIDATED}}` |
| Default branch/remote state | {{SYNC_STATE}} |
| Worktree state | {{CLEAN_OR_EXACT_CHANGES}} |
| Build/test environment | {{VERSIONS_AND_IDENTIFIERS}} |
| Deployed/installed version | {{VERSION_COMMIT_OR_NOT_APPLICABLE}} |
| Last validated environment | {{ENVIRONMENT_AND_TIMESTAMP}} |
| Physical intervention required | {{YES_NO_NOT_APPLICABLE_AND_REASON}} |

Do not record passwords, tokens, private keys, production data, or other
secrets. Treat addresses, device paths, sessions, and external service state as
timestamped observations, not permanent configuration.

## Current objective

{{ONE_PARAGRAPH_DESCRIPTION_OF_THE_ACTIVE_SLICE_AND_ITS_MERGE_BOUNDARY}}

## Last completed work

- {{COMPLETED_CHANGE_WITH_COMMIT_PR_OR_EVIDENCE}}
- {{COMPLETED_CHANGE_WITH_COMMIT_PR_OR_EVIDENCE}}

## Validation evidence

### Observed

- {{DIRECTLY_OBSERVED_RESULT_AND_SOURCE}}

### Inferred

- {{INFERENCE_AND_SUPPORTING_EVIDENCE_OR_NONE}}

### Not tested

- {{IMPORTANT_UNTESTED_BEHAVIOR}}

## Remaining in this slice

- [ ] {{TASK_OR_ACCEPTANCE_CRITERION}}
- [ ] {{TASK_OR_ACCEPTANCE_CRITERION}}

## Blockers and authorization needs

- {{NONE_OR_SPECIFIC_BLOCKER_ACTION_OWNER}}

## Exact next action

1. {{ONE_CONCRETE_FIRST_STEP}}

## Diagnostic artifacts

- `{{PATH}}`: {{PURPOSE_TIMESTAMP_AND_RETENTION_POLICY}}

## Session closeout

- [ ] Branch, HEAD, remote, and worktree state recorded
- [ ] Deployed version/environment provenance recorded if applicable
- [ ] Validation and important failures summarized
- [ ] Background processes, locks, devices, and temporary access released
- [ ] Exact next action recorded
