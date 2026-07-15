# First Codex message template

Copy, customize, and send this as the first message for a new project.

```text
I want you to establish a safe, durable development workflow before implementing
product changes.

First read:
- AGENTS.md at the workspace root
- docs/project/PROJECT_BRIEF.md
- docs/project/DEVELOPMENT_ROLES.md
- docs/project/MY_CODEX_COLLABORATION_PROFILE.md, if present
- docs/project/PROJECT_INTAKE.md, if present
- any files under docs/project/addenda/ that apply to this project

Phase 1 — Discovery only:
1. Inspect the repository, Git state, build/test configuration, existing docs,
   generated files, and relevant environment state.
2. Do not change product behavior, install dependencies, access hardware,
   mutate external services, push, merge, or release during discovery unless I
   explicitly authorize it.
3. Distinguish observed facts, inferences, unknowns, and stale documentation.
4. Identify secrets or sensitive data that must not enter chat or tracked files.
5. Report conflicts between my intake, repository reality, and AGENTS.md.

Phase 2 — Project setup:
1. Customize or create the project brief, development plan, roles, preflight,
   decision log, current status, and only the optional documents this project
   actually needs.
2. Divide milestones into small, independently reviewable branches with an
   explicit definition of done and proportionate validation for each slice.
3. Propose the first implementation slice and exact next action.
4. Ask only blocking questions whose answers would materially change scope,
   security, architecture, or acceptance criteria. Make and label safe
   assumptions for everything else.

Before implementation, show me:
- your evidence-based understanding of the current state
- proposed guardrails and exclusions
- unresolved decisions and recommendations
- the small-branch development plan
- the validation strategy
- the exact first implementation slice

Keep status updates concise. Preserve my existing changes. Never record secrets.
Do not claim completion without evidence from the environments that matter.
```

For a tiny project, tell the Agent which documents may be combined. Do not drop
`AGENTS.md`, the project brief, or current status solely because the project is
small.
