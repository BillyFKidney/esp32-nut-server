# {{MAINTAINER_NAME}} Codex collaboration profile

Use this for preferences that remain stable across projects. Put project access,
scope, and technical guardrails in the project-specific documents.

## Communication

- Desired level of detail: {{CONCISE_BALANCED_DETAILED}}
- Preferred update style: {{OUTCOME_FIRST_STEPS_FIRST_OTHER}}
- Technical comfort: {{BACKGROUND_AND_TERMINOLOGY}}
- When to explain rationale: {{PREFERENCES}}
- How to present disagreement or risk: {{PREFERENCES}}

## Autonomy

- Actions the Agent should take without asking: {{SAFE_REVERSIBLE_ACTIONS}}
- Actions that require confirmation: {{EXTERNAL_DESTRUCTIVE_SENSITIVE_ACTIONS}}
- Assumptions the Agent may make: {{ASSUMPTION_BOUNDARY}}
- Questions worth interrupting work for: {{BLOCKING_QUESTION_THRESHOLD}}

## Planning and implementation

- Preferred planning depth: {{PREFERENCE}}
- Preferred branch/PR size: {{PREFERENCE}}
- Preferred merge method: {{METHOD}}
- Versioning model and milestone/slice/release mapping: {{VERSION_POLICY}}
- When to surface a missing or deferred release target: {{RELEASE_GAP_POLICY}}
- Documentation expected from the start: {{DOCUMENTS}}
- Definition of done expectations: {{EXPECTATIONS}}

## Service and workflow continuity

- Services that require explicit named approval before retirement:
  {{SERVICE_RETIREMENT_POLICY}}
- Agent access, automation, deployment/update capability, or recurring work that
  must not be removed or transferred without explicit approval:
  {{WORKFLOW_CONTINUITY_POLICY}}
- Required replacement-before-retirement and rollback expectations:
  {{CUTOVER_POLICY}}

## Testing

- Human testing role: {{ROLE}}
- Devices/environments normally available: {{ENVIRONMENTS}}
- Evidence the human can provide: {{EVIDENCE}}
- Accessibility/UX priorities: {{PRIORITIES}}

## File and Git safety

- Existing-change policy: {{POLICY}}
- Cleanup/deletion policy: {{POLICY}}
- Commit/push/merge preferences: {{POLICY}}
- Tag/release/artifact publication preferences: {{POLICY}}

## Handoff

- Required status fields: {{FIELDS}}
- Desired end-of-session behavior: {{BEHAVIOR}}
- How to identify the exact next action: {{PREFERENCE}}

This profile does not grant standing authority for destructive, secret-bearing,
production, external, or unrelated work.
