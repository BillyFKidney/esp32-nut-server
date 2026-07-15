# {{PROJECT_NAME}} release checklist

**Release:** {{VERSION_OR_IDENTIFIER}}

**Target date:** {{DATE}}

**Release owner:** {{PROJECT_MAINTAINER}}

**Source commit:** {{COMMIT}}

## Scope

- [ ] Included milestones/slices are listed.
- [ ] Deferred or excluded work is explicit.
- [ ] Changelog/release notes describe user impact and breaking changes.
- [ ] Versioning follows `{{VERSION_POLICY}}`.

## Repository

- [ ] Default branch is synchronized and worktree is clean.
- [ ] Required PRs are reviewed and merged with the intended history.
- [ ] Release commit/tag points to the reviewed source.
- [ ] Generated or local-only files are not included accidentally.

## Validation

- [ ] Clean build/package succeeds in the supported toolchain.
- [ ] Automated tests, linting, and static checks pass.
- [ ] Manual acceptance matrix passes.
- [ ] Upgrade, rollback, restart, and recovery paths pass.
- [ ] Supported environments and minimum versions are exercised.
- [ ] Known limitations and untested behavior are documented.

## Security and data

- [ ] Security checklist and dependency review are complete.
- [ ] No secrets, private data, debug credentials, or unsafe endpoints ship.
- [ ] Migration, backup, retention, and destructive-action behavior is reviewed.
- [ ] Signing/notarization/checksums are complete when applicable.

## Documentation and operations

- [ ] README, setup, configuration, troubleshooting, and current status agree.
- [ ] Operator runbook and support/escalation path are current.
- [ ] Monitoring, logs, alerting, and health checks are ready.
- [ ] Rollback owner and procedure are known.

## Publish/deploy

- [ ] Project Maintainer explicitly authorized the release.
- [ ] Artifact provenance and destination are recorded.
- [ ] Deployment/publishing is performed once from the approved commit.
- [ ] Release/tag/assets are verified from the consumer perspective.

## Post-release

- [ ] Primary workflow and health indicators are verified.
- [ ] Errors, regressions, and support channels are monitored.
- [ ] `CURRENT_STATUS.md` records the release and next action.
- [ ] Release outcome and rollback decision are recorded.
