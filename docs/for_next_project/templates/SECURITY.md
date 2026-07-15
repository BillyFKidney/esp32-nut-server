# {{PROJECT_NAME}} security model

This document records project-specific assets, trust boundaries, controls,
limitations, and response procedures. Do not include secret values.

## Scope and assumptions

- Deployment model: {{LOCAL_DESKTOP_LAN_CLOUD_DEVICE_OTHER}}
- Trusted actors/systems: {{TRUSTED}}
- Untrusted actors/systems: {{UNTRUSTED}}
- Security assumptions: {{ASSUMPTIONS}}
- Explicit non-goals: {{NON_GOALS}}

## Assets

| Asset | Sensitivity | Location | Owner | Backup/recovery |
| --- | --- | --- | --- | --- |
| {{ASSET}} | {{PUBLIC_INTERNAL_CONFIDENTIAL_CRITICAL}} | {{LOCATION}} | {{OWNER}} | {{RECOVERY}} |

## Trust boundaries and data flows

| Flow | Source → destination | Authentication | Encryption | Validation/logging |
| --- | --- | --- | --- | --- |
| {{FLOW}} | {{SOURCE_DESTINATION}} | {{AUTH}} | {{TRANSPORT_STORAGE}} | {{CONTROLS}} |

## Authentication and authorization

- Identities: {{IDENTITIES}}
- Roles/permissions: {{ROLES}}
- Session/token lifecycle: {{LIFECYCLE}}
- Recovery/revocation: {{RECOVERY}}
- Rate limiting/abuse controls: {{CONTROLS}}

## Secrets

- Secret types: {{NAMES_NOT_VALUES}}
- Approved storage: {{KEYCHAIN_VAULT_ENVIRONMENT_SECRET_STORE}}
- Prohibited locations: source, chat, logs, screenshots, fixtures, and tracked
  configuration unless encrypted by an approved system
- Rotation/revocation: {{PROCESS}}

## Threats and controls

| Threat | Impact | Control | Residual risk | Validation |
| --- | --- | --- | --- | --- |
| {{THREAT}} | {{IMPACT}} | {{CONTROL}} | {{RISK}} | {{TEST}} |

## Destructive and privileged operations

- Operations: {{DELETE_RESET_MIGRATE_DEPLOY_CONTROL_OTHER}}
- Required role/authority: {{AUTHORITY}}
- Confirmation design: {{CONFIRMATION}}
- Audit evidence: {{LOG_OR_RECORD}}
- Rollback/recovery: {{RECOVERY}}

## Dependency and update policy

- Dependency sources: {{SOURCES}}
- Version/update policy: {{POLICY}}
- Artifact/signature verification: {{VERIFICATION}}
- Vulnerability response: {{PROCESS}}

## Security validation checklist

- [ ] Inputs and outputs are validated and safely encoded.
- [ ] Authorization is checked server-side or at the true control boundary.
- [ ] Secrets do not appear in source, logs, errors, screenshots, or artifacts.
- [ ] Failure and rate-limit behavior is tested.
- [ ] Backup, recovery, revocation, and rollback are tested.
- [ ] Network, storage, and platform-specific controls are reviewed.

## Incident response

1. Preserve evidence without spreading secrets.
2. Contain the affected account, device, service, or environment.
3. Revoke/rotate affected credentials and block unsafe paths.
4. Restore from a known-good state.
5. Validate before reconnecting or redeploying.
6. Record root cause, impact, corrective action, and follow-up decision.

Security reports go to: {{PRIVATE_REPORTING_CHANNEL}}.
