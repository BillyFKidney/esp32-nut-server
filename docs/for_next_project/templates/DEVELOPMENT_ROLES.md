# {{PROJECT_NAME}} development roles

Roles describe capabilities and authority. One person may hold multiple roles.
Choose names that do not collide with product account roles.

## Device Operator

**People:** {{OPERATORS_OR_TESTERS}}

The Device Operator performs user-facing, GUI, physical, or environment
observations and leads Level 1 diagnostics.

### Capabilities

- {{PHYSICAL_OR_GUI_ACCESS}}
- {{VIEW_ONLY_SYSTEMS}}
- {{TEST_DEVICES_AND_CLIENTS}}

### Responsibilities

- Run the human section of `PREFLIGHT.md`.
- Report observations and timestamps rather than guessing causes.
- Perform physical or live-system changes only when clearly authorized.
- Lead manual GUI, UX, and end-to-end acceptance tasks.
- Keep secrets out of chat and tracked files.

## Project Maintainer

**People:** {{MAINTAINERS}}

The Project Maintainer owns scope, architecture, priority, security policy,
acceptance, and authorization for consequential actions.

### Capabilities

- {{ADMINISTRATIVE_AND_DEVELOPMENT_ACCESS}}
- {{GIT_AND_RELEASE_AUTHORITY}}
- {{THIRD_PARTY_SYSTEM_AUTHORITY}}

### Responsibilities

- Resolve scope, risk, and security decisions.
- Approve destructive, external, production, release, and credential-sensitive
  actions.
- Confirm whether work is ready to commit, merge, deploy, or release.
- Provide evidence or access the Agent cannot obtain directly.

## Codex Agent

The Codex Agent is the implementation and diagnostic collaborator.

### Capabilities

- Inspect and edit the shared workspace.
- Run available tools and approved commands.
- Build, test, analyze, document, and publish within authorized scope.
- Interpret logs and evidence supplied by humans or connected systems.

### Limits

- Cannot physically inspect or manipulate devices.
- Does not inherently have access to GUI applications, third-party services,
  credentials, or current external state.
- Cannot assume old addresses, device identifiers, versions, sessions, or
  deployments are current.
- Must not infer authority for destructive or materially broader actions.

### Responsibilities

- Inspect evidence before acting.
- Preserve human-owned and unrelated changes.
- Explain tradeoffs and label assumptions.
- Give physical/GUI instructions one action at a time with expected evidence.
- Keep status and decisions durable across chats.
- Stop for Maintainer direction when a choice changes scope, security,
  architecture, acceptance, or authorization.

## Responsibility matrix

| Activity | Device Operator | Project Maintainer | Codex Agent |
| --- | --- | --- | --- |
| GUI/UX testing | Leads | Accepts | Defines tasks and implements fixes |
| Physical actions | Performs | Authorizes | Gives scoped instructions |
| Source changes | Observes | Reviews/authorizes | Implements |
| Build/test | Supplies environment evidence | Accepts strategy | Runs/interprets |
| External service changes | Assists | Authorizes | Performs only when authorized and accessible |
| Commit/push/merge/release | No | Owns authority | Performs only when requested |
| Secrets | Keeps private | Administers | Avoids requesting/recording |
| Final acceptance | Provides results | Owns decision | Provides evidence/recommendation |

## Shared vocabulary

- **Observed:** directly evidenced now
- **Inferred:** concluded from evidence but not directly verified
- **Not tested:** expected or implemented but not exercised
- **Blocked:** requires a specific authority, human action, or external change
- **Next action:** the single first step when work resumes
