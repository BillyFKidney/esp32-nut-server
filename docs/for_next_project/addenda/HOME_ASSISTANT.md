# Home Assistant addendum

Use for Home Assistant configuration, automations, scripts, dashboards,
integrations, add-ons, blueprints, and custom components.

## Record the environment

- Installation type: `{{HA_OS_CONTAINER_CORE_SUPERVISED}}`
- Core/Supervisor/OS versions: `{{VERSIONS}}`
- Production versus test instance: `{{ENVIRONMENTS}}`
- Configuration method: `{{YAML_UI_PACKAGES_GIT_CUSTOM_COMPONENT}}`
- Relevant integrations/add-ons: `{{INTEGRATIONS}}`
- Required devices/entities/areas: `{{IDENTIFIERS}}`
- Backup and restore method: `{{BACKUP_METHOD}}`

## Guardrails

- Treat the live Home Assistant instance as production unless explicitly
  identified as disposable.
- Never place tokens, webhook secrets, passwords, coordinates, or private data
  in Git or chat. Use `secrets.yaml` or the integration's approved secret store.
- Do not rename entity IDs, device IDs, areas, helpers, labels, or unique IDs as
  incidental cleanup; they may have hidden consumers.
- Prefer targeted reloads over full restarts and preserve availability of
  safety, climate, alarm, and power automations.
- Do not change recorder/history retention or high-frequency update behavior
  without considering database and storage impact.

## Preflight additions

- [ ] Create/verify a recent backup before broad configuration or migration.
- [ ] Record current versions and update availability without updating.
- [ ] Identify the exact instance and confirm production/test status.
- [ ] Record current entity state and automation mode for affected workflows.
- [ ] Check configuration and logs before reload/restart.
- [ ] Identify whether the Maintainer has admin access or only user/view access.

## Implementation slices

Useful boundaries include:

- Integration/device connectivity
- Helpers and data model
- One automation workflow
- Dashboard/UX
- Notifications
- Energy/history/recorder behavior
- Backup/recovery and migration
- Acceptance across required users and devices

## Validation additions

- Run Home Assistant configuration validation before reload/restart.
- Test trigger, conditions, actions, concurrency mode, timeout, and failure path.
- Exercise unavailable/unknown entities, restart recovery, duplicate events,
  offline devices, and delayed state changes.
- Confirm dashboards on required desktop/mobile clients and user permissions.
- Inspect logs and traces, but redact secrets and private state.
- Verify rollback by restoring the previous YAML/configuration or documented
  backup—not by improvising on the live instance.
