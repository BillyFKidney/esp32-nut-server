# {{PROJECT_NAME}} development preflight

Use this at session start and closeout. Keep the human path short; place detailed
recovery steps below it.

## Device Operator: start here

- [ ] Preserve a healthy running environment; do not restart or reconnect
      components merely to create a “clean slate.”
- [ ] Stop any known monitor, debugger, dev server, editor task, or GUI session
      that would compete for the same resource—unless it must be preserved.
- [ ] Record the current time and observable environment state:
      `{{HUMAN_OBSERVATIONS}}`.
- [ ] Check the primary user-facing path: `{{GUI_URL_APP_AUTOMATION_OR_DEVICE}}`.
- [ ] Report what was checked, what failed, and what was not checked.
- [ ] Tell the Agent about physical, GUI, service, network, or credential state
      changed since the previous session.

Copyable handoff:

```text
Observed at:
Environment/version:
Primary user path: responds / fails / not checked
Required service/device: responds / fails / not checked
Competing monitor/process intentionally running: yes / no
Physical or external changes since last session:
```

## Project Maintainer preflight

- [ ] Session goal and acceptable stopping point are clear.
- [ ] Current status is current or explicitly marked stale.
- [ ] Backup, recovery, and rollback needs are understood.
- [ ] Consequential external actions are explicitly authorized before use.
- [ ] No secrets will enter tracked files, chat, screenshots, logs, or echoed
      command output.

## Codex Agent preflight

1. Read `AGENTS.md`, project brief, current status, roles, active milestone, and
   applicable addenda.
2. Inspect repository and environment state before editing:

```bash
git status --short --branch
{{READ_ONLY_ENVIRONMENT_CHECKS}}
```

3. Resolve drift-prone identifiers now: versions, addresses, device paths,
   environment names, sessions, branch/PR state, and service ownership.
4. Identify processes or tools that could compete for the same resource.
5. Prefer the cheapest read-only health check before restarts, cable changes,
   resets, migrations, or deployments.
6. Record unavailable tools or permissions as `not checked`, not as target
   failure.

## Recovery ladder

Stop as soon as a step restores progress.

1. Refresh facts and identifiers.
2. Run read-only health and ownership checks.
3. Release only the known competing owner/process.
4. Restart the smallest affected component.
5. Reconnect or reset only the affected device/service.
6. Use backup, rollback, reinstall, reflash, or restore only when simpler
   recovery is insufficient and authority is explicit.
7. Preserve evidence before destructive recovery.

## Rules during the session

- Keep one known owner for exclusive resources.
- Give human instructions one action at a time with expected evidence.
- Record the source commit/configuration of deployed artifacts.
- Do not confuse build success with runtime or acceptance proof.
- Keep large diagnostics outside normal source paths and summarize conclusions.

## End-of-session checklist

- [ ] Update `CURRENT_STATUS.md`.
- [ ] Record branch, commit, remote, environment, and deployment state.
- [ ] Record validation, failures, and untested behavior.
- [ ] Stop or intentionally record background processes and resource owners.
- [ ] Release temporary access and confirm recovery state.
- [ ] Record one exact next action.
