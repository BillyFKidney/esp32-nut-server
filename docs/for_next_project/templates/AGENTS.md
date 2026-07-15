# {{PROJECT_NAME}} agent instructions

These instructions apply to the whole repository unless a more specific
`AGENTS.md` exists in a subdirectory.

## Repository identity

{{DESCRIBE_WHAT_THIS_REPOSITORY_IS_WHERE_IT_CAME_FROM_AND_WHAT_MUST_BE_PRESERVED}}

Before reorganizing or deleting unfamiliar files, determine whether they are
tracked, generated, imported, referenced by tooling, or owned by the human.

## Start here

Before implementation, read:

1. `{{PROJECT_DOCS_PATH}}/PROJECT_BRIEF.md`
2. `{{PROJECT_DOCS_PATH}}/CURRENT_STATUS.md`
3. `{{PROJECT_DOCS_PATH}}/DEVELOPMENT_ROLES.md`
4. `{{PROJECT_DOCS_PATH}}/PREFLIGHT.md`
5. The active milestone and applicable addenda linked from current status

## Working-tree safety

- Treat existing changes and untracked files as human-owned until their
  provenance is established.
- Inspect Git status and ignore rules before classifying files.
- Preserve unrelated changes and avoid broad mechanical rewrites.
- Do not delete files, discard changes, rewrite history, or use destructive
  commands without explicit Project Maintainer authority.
- Keep generated and machine-local output untracked:
  `{{GENERATED_PATHS_AND_FILES}}`.
- Never put credentials, tokens, private keys, signing material, production
  data, or other secrets in tracked files, chat, or echoed command output.

## Project guardrails

- {{ARCHITECTURE_OR_PRODUCT_GUARDRAIL}}
- {{SECURITY_GUARDRAIL}}
- {{COMPATIBILITY_GUARDRAIL}}
- {{SAFETY_OR_DATA_GUARDRAIL}}
- {{EXPLICITLY_DEFERRED_OR_PROHIBITED_WORK}}

If requested work conflicts with a guardrail, explain the conflict and ask for
a reviewed decision instead of silently changing the rule.

## Commands

Use the repository's documented environment. Do not guess commands when they
can be discovered locally.

```bash
# Setup
{{SETUP_COMMAND}}

# Build or static validation
{{BUILD_COMMAND}}

# Automated tests
{{TEST_COMMAND}}

# Formatting or linting
{{LINT_COMMAND}}
```

Record commands that require special devices, credentials, elevated access, or
external services in `PREFLIGHT.md`, not here as hidden assumptions.

## Branch and validation policy

- Begin each independently reviewable slice from the latest `main` or default
  branch.
- Keep one coherent risk and acceptance boundary per branch and PR.
- Prefer `{{MERGE_METHOD}}` merges.
- Run validation proportional to risk: `{{REQUIRED_VALIDATION_LEVELS}}`.
- A successful build is not runtime, integration, hardware, UX, security, or
  recovery proof.
- Do not mark a milestone complete until its combined definition of done has
  evidence, even if every implementation branch has merged.

## Authority and external actions

Follow `DEVELOPMENT_ROLES.md`. Builds and local reversible checks may be normal
implementation steps. Pushing, merging, releasing, deploying, modifying live
services, destructive recovery, or affecting other people requires authority
from the current request or an explicit approval.

## Documentation and handoff

- Keep `CURRENT_STATUS.md` factual whenever repository, environment,
  deployment, validation, blocker, or next-action state changes.
- Put stable requirements in the brief/plan, decisions in the decision log,
  reusable procedures in preflight, and changing facts in current status.
- Mark material claims as **observed**, **inferred**, or **not tested**.
- End each session with one exact next action.
- Record enough evidence that a new chat can resume without reconstructing the
  previous conversation.
