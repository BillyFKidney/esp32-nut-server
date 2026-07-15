# {{PROJECT_NAME}} development plan

This document tracks stable milestones and implementation slices. Put changing
branch, environment, and deployment facts in `CURRENT_STATUS.md`.

## Guardrails

- {{ARCHITECTURE_GUARDRAIL}}
- {{SECURITY_OR_DATA_GUARDRAIL}}
- {{COMPATIBILITY_GUARDRAIL}}
- {{VALIDATION_GUARDRAIL}}
- {{DEFERRED_OR_PROHIBITED_SCOPE}}

## Target environments

| Item | Target |
| --- | --- |
| Runtime/platform | {{RUNTIME}} |
| Language/framework | {{STACK}} |
| Minimum supported version | {{MINIMUM_VERSION}} |
| Development environment | {{DEVELOPMENT_ENVIRONMENT}} |
| Test environments | {{TEST_ENVIRONMENTS}} |
| Production/deployed environment | {{PRODUCTION_ENVIRONMENT}} |

## Completed milestones

| Milestone | Status | Evidence/outcome |
| --- | --- | --- |
| {{MILESTONE}} | Complete | {{COMMIT_PR_RELEASE_OR_VALIDATION}} |

## Active milestone: {{MILESTONE_NAME}}

**Status:** {{PROPOSED_REQUIREMENTS_REVIEW_LOCKED_IMPLEMENTING_ACCEPTANCE_COMPLETE}}

**Requirements:** [{{MILESTONE_DOCUMENT}}]({{MILESTONE_DOCUMENT}})

**Definition of done:**

- [ ] {{USER_VISIBLE_CRITERION}}
- [ ] {{TECHNICAL_CRITERION}}
- [ ] {{SECURITY_RECOVERY_CRITERION}}
- [ ] {{TARGET_ENVIRONMENT_CRITERION}}

### Implementation slices

Each branch begins at the latest default branch and contains one coherent risk,
review, and acceptance boundary.

| Order | Branch | Scope | Required validation | Status |
| --- | --- | --- | --- | --- |
| 1 | `feature/{{SLICE_NAME}}` | {{ONE_COHERENT_CAPABILITY}} | {{BUILD_TEST_RUNTIME_MANUAL}} | Planned |
| 2 | `feature/{{SLICE_NAME}}` | {{ONE_COHERENT_CAPABILITY}} | {{BUILD_TEST_RUNTIME_MANUAL}} | Planned |
| Final | `feature/{{MILESTONE}}-acceptance` | Combined definition of done and release preparation | Full supported-environment matrix | Planned |

Cross-cutting design rules should be applied to each slice rather than becoming
an indefinite umbrella branch. Split a slice further when it contains multiple
independent failure or rollback boundaries.

## Later milestones

### {{MILESTONE}}

- {{CAPABILITY}}
- {{CAPABILITY}}

Success criterion: {{MEASURABLE_OUTCOME}}.

## Deferred ideas

- {{IDEA}} — revisit after {{TRIGGER_OR_MILESTONE}}.

## Change tracking conventions

- Use one branch and PR per independently reviewable slice.
- Prefer `{{MERGE_METHOD}}` merges.
- Record validation in the PR and relevant project documentation.
- Merge validated foundations without declaring the umbrella milestone complete.
- Start the next branch from synchronized default branch after merge.
- Update this plan when scope, sequence, guardrails, or completion status changes.
