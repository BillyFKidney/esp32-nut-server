# Codex software-project starter kit

This directory is a reusable project operating system for starting software
work with Codex. It captures the practices that made the ESP32-NUT project
effective: explicit authority, discovery before implementation, small branches,
locked decisions, evidence-based validation, current status, and reliable
handoffs between chats.

The kit is intentionally generic. Use only the templates justified by the
project's size and risk, then add the relevant project-type addenda.

## The most important distinction

`AGENTS.md` is not merely an attachment. Copy and customize
`templates/AGENTS.md` as **`AGENTS.md` at the root of the new workspace** before
the first implementation message. Codex automatically discovers repository
instructions there.

The other templates normally belong under `docs/project/` or an equivalent
project-owned documentation directory.

## Before the first message

1. Create or select the project workspace and initialize Git if appropriate.
2. Complete [PROJECT_INTAKE.md](PROJECT_INTAKE.md) as far as practical.
3. Copy [MY_CODEX_COLLABORATION_PROFILE.md](MY_CODEX_COLLABORATION_PROFILE.md)
   into the new project documentation and adjust any project-specific details.
4. Copy `templates/AGENTS.md` to the workspace root as `AGENTS.md` and replace
   obvious placeholders.
5. Copy these core templates into `docs/project/`:
   - `PROJECT_BRIEF.md`
   - `DEVELOPMENT_ROLES.md`
   - `DEVELOPMENT_PLAN.md`
   - `CURRENT_STATUS.md`
   - `PREFLIGHT.md`
   - `DECISION_LOG.md`
6. Copy only the relevant files from `addenda/`.
7. Send the customized text from
   [FIRST_MESSAGE_TEMPLATE.md](FIRST_MESSAGE_TEMPLATE.md).

Do not put passwords, tokens, private keys, signing material, production data,
or other secrets in any starter document or chat message.

## Template catalog

### Core for most projects

| Template | Purpose |
| --- | --- |
| `AGENTS.md` | Durable repository-wide instructions, safety rules, commands, and handoff expectations |
| `COLLABORATION_PROFILE.md` | Cross-project preferences for communication, autonomy, Git, safety, testing, and handoff |
| `PROJECT_BRIEF.md` | Stable description of the problem, users, outcomes, constraints, and exclusions |
| `DEVELOPMENT_ROLES.md` | Human and Agent capabilities, limits, authority, and responsibility boundaries |
| `DEVELOPMENT_PLAN.md` | Milestones divided into small, independently reviewable branches |
| `CURRENT_STATUS.md` | Changing branch, environment, deployment, evidence, blockers, and exact next action |
| `PREFLIGHT.md` | Short human and Agent startup/closeout checks |
| `DECISION_LOG.md` | Durable choices, alternatives, consequences, and revisit triggers |

### Add when justified

| Template | Use when |
| --- | --- |
| `MILESTONE_QA.md` | Requirements or security decisions must be resolved and locked before implementation |
| `MANUAL_TEST_PLAN.md` | A person must validate GUI, UX, hardware, integrations, or acceptance criteria |
| `SECURITY.md` | The project handles credentials, personal data, network services, automation privileges, or updates |
| `RELEASE_CHECKLIST.md` | Software is distributed, deployed, tagged, published, or installed on other systems |
| `CLEANUP_README.md` | Imported or inherited repositories need non-destructive file triage |
| `README.md` | The repository needs user/developer orientation and setup instructions |

## Recommended lifecycle

1. **Discovery gate:** inspect the repository and environment without changing
   product behavior. Separate observed facts, inferences, and unknowns.
2. **Planning gate:** agree on guardrails, definition of done, major decisions,
   small implementation slices, and the version/release target assigned to each
   slice when the project publishes versioned releases.
3. **Implementation slice:** create one branch for one coherent risk and review
   boundary.
4. **Validation gate:** run proportionate automated, integration, target,
   manual, security, and recovery checks.
5. **Handoff:** update current status with evidence and one exact next action.
6. **Publish code:** commit intentionally, push, use a draft PR while
   incomplete, merge validated slices, then start the next branch from updated
   `main`.
7. **Slice release gate:** after acceptance, explicitly decide whether to
   publish the slice's assigned version. Verify the tag, artifacts, checksums,
   and release record rather than assuming the merge created a release.
8. **Milestone acceptance:** declare the umbrella milestone complete only when
   its combined definition of done passes—not merely because its last branch
   merged or an early version in its release family was published.

## Versioning and release continuity

Choose and document a versioning model before assigning release numbers. One
useful milestone-oriented model is:

- `MAJOR`: the umbrella milestone or release family
- `MINOR`: one independently validated and published implementation slice
- `PATCH`: a compatible fix to an already published slice

This model is not necessarily strict Semantic Versioning while a public API is
still emerging, so say that explicitly. The first foundation slice in a family
may use `MAJOR.0.0`, later slices may use `MAJOR.MINOR.0`, and the combined
milestone acceptance can remain a later planned minor release.

A merge and a release are separate events. A version target is published only
after its slice is validated, accepted, and explicitly authorized for release.
After every accepted or merged slice, audit the planned version against existing
tags and releases. Publish it, or record an intentional deferral and exact next
action; do not leave a completed release target missing by accident.

## Behaviors this kit is designed to prevent

- Implementing before the actual problem and exclusions are understood
- One long-running branch for an entire milestone
- Treating a build as proof that runtime behavior works
- Leaving critical decisions only in chat history
- Hard-coding IP addresses, device paths, ports, environment names, or tool
  locations that can change
- Starting multiple processes that compete for one device or resource
- Confusing lack of tool access with failure of the target system
- Moving or deleting unfamiliar files before checking Git and build references
- Losing the relationship between a deployed artifact and its source commit
- Ending a session without a clean handoff and exact next action
- Merging or releasing without explicit human authority
- Leaving an accepted, release-targeted slice untagged because merge and release
  were treated as the same event
- Retiring a service because the Agent recommends it without obtaining explicit
  approval naming that service
- Removing Agent access or automation and silently making a previously
  Agent-owned task the human's responsibility

## Working well with the Project Maintainer

The goal is not to make the human write perfect specifications. The Agent
should convert incomplete ideas into explicit choices, explain material
tradeoffs, make safe assumptions when possible, and ask only questions whose
answers materially change the result.

Good collaboration means:

- Lead with concrete evidence and outcomes.
- Use plain language and short, actionable updates.
- Ask one focused question at a time when human input is truly required.
- Give physical or GUI instructions one action at a time and state what result
  to report.
- Preserve human-owned changes and avoid surprise external actions.
- Record durable conclusions in project files so the next chat does not have to
  reconstruct them.

## Placeholder convention

Templates use `{{UPPER_SNAKE_CASE}}` placeholders. Replace every applicable
placeholder and remove irrelevant sections. Write `Not applicable` or
`Not decided` when that distinction matters; do not silently leave ambiguous
blanks.

The first Agent should tailor the templates to the repository rather than
blindly preserving generic language.
