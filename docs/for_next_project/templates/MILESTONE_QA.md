# {{MILESTONE_NAME}} milestone questions and answers

**Status:** {{DRAFT_UNDER_REVIEW_LOCKED_FOR_IMPLEMENTATION}}

**Scope:** {{PROJECT_MILESTONE_RELEASE_OR_BRANCH_BOUNDARY}}

Material scope or security changes after this document is locked require a new
review and decision-log entry.

## Questions requiring decisions

### 1. {{TOPIC}}

**Question:** {{PRECISE_QUESTION}}

**Why it matters:** {{SCOPE_SECURITY_UX_COMPATIBILITY_OR_OPERATIONS_IMPACT}}

**Options:**

| Option | Benefits | Costs/risks |
| --- | --- | --- |
| {{OPTION}} | {{BENEFITS}} | {{COSTS}} |
| {{OPTION}} | {{BENEFITS}} | {{COSTS}} |

**Recommendation:** {{RECOMMENDATION_AND_REASON}}

**Recorded answer:** {{ANSWER_OR_PENDING}}

### 2. {{TOPIC}}

**Question:** {{QUESTION}}

**Recorded answer:** {{ANSWER_OR_PENDING}}

## Security and recovery boundary

- Trust boundary: {{BOUNDARY}}
- Authentication/authorization: {{MODEL}}
- Sensitive data: {{DATA_AND_STORAGE}}
- Destructive actions: {{CONFIRMATION_AND_AUTHORITY}}
- Backup/rollback/recovery: {{RECOVERY_PATH}}
- Explicitly prohibited behavior: {{PROHIBITIONS}}

## Deferred decisions

| Decision | Deferred until | Reason/default until then |
| --- | --- | --- |
| {{DECISION}} | {{MILESTONE_OR_TRIGGER}} | {{REASON}} |

## Definition of done

From each required supported environment, the appropriate user can:

- [ ] {{USER_VISIBLE_ACCEPTANCE_CRITERION}}
- [ ] {{FAILURE_OR_RECOVERY_CRITERION}}
- [ ] {{SECURITY_OR_PERMISSION_CRITERION}}
- [ ] {{COMPATIBILITY_CRITERION}}

## Lock checklist

- [ ] All material questions have recorded answers.
- [ ] Deferred items have safe temporary behavior.
- [ ] Definition of done is observable and testable.
- [ ] Project Maintainer approved the scope and risk boundary.
- [ ] Decisions are copied to `DECISION_LOG.md` when they affect later work.
