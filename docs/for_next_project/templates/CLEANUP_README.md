# {{PROJECT_NAME}} cleanup holding area

Nothing under this directory is an active build or documentation input unless
explicitly stated. Material is preserved for review instead of deleted.

## Categories

| Path | Purpose |
| --- | --- |
| `artifacts/` | Local diagnostics retained outside normal source status |
| `review/orphan/` | No active purpose or reference identified |
| `review/redundant/` | Duplicate material retained pending review |
| `delete_recommended/outdated/` | Reviewed as obsolete but awaiting deletion authority |

Moving something here is not permission to delete it.

## Classification procedure

Before moving material:

1. Inspect Git tracking and ignore state.
2. Search source, build, documentation, deployment, and automation references.
3. Determine whether it is generated and where its tool expects it.
4. Compare suspected duplicates byte-for-byte when possible.
5. Preserve relative paths and record evidence.
6. Move only after active use is ruled out or the Maintainer accepts the risk.

## Inventory

### {{DATE}} — {{SHORT_DESCRIPTION}}

**Classification:** {{OBSERVED_OR_INFERRED}} / {{ORPHAN_REDUNDANT_OUTDATED_ARTIFACT}}

**Original path:** `{{PATH}}`

**New path:** `{{PATH}}`

**Reason/evidence:** {{GIT_REFERENCE_SEARCH_COMPARISON_OR_TOOL_EVIDENCE}}

**Deletion recommendation:** {{NO_REVIEW_AFTER_DATE_OR_CONDITION}}

## Structure reviewed but retained

- `{{PATH}}`: retained because {{BUILD_HISTORY_UPSTREAM_TOOL_OR_ACTIVE_USE}}.

## Proposed organization requiring review

Describe structural changes here before moving tracked active files. Include
benefits, costs, references to update, migration plan, and rollback plan.
