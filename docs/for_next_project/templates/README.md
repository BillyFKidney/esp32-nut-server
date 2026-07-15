# {{PROJECT_NAME}}

{{ONE_SENTENCE_DESCRIPTION_AND_PRIMARY_USER_OUTCOME}}

## Status

{{EXPERIMENTAL_ACTIVE_STABLE_MAINTENANCE_ARCHIVED}}. See
[`{{PROJECT_DOCS_PATH}}/CURRENT_STATUS.md`]({{PROJECT_DOCS_PATH}}/CURRENT_STATUS.md)
for the active slice and evidence.

## Features

- {{FEATURE}}
- {{FEATURE}}

## Supported environments

| Environment | Supported versions | Validation status |
| --- | --- | --- |
| {{ENVIRONMENT}} | {{VERSIONS}} | {{STATUS}} |

## Requirements

- {{TOOL_RUNTIME_DEVICE_SERVICE_OR_ACCOUNT}}

## Setup

```bash
{{SETUP_COMMANDS}}
```

Never commit local credentials. Configure secrets through
`{{APPROVED_SECRET_MECHANISM}}`.

## Build and run

```bash
{{BUILD_AND_RUN_COMMANDS}}
```

## Test

```bash
{{TEST_COMMANDS}}
```

Manual or target testing: [{{MANUAL_TEST_PATH}}]({{MANUAL_TEST_PATH}})

## Configuration

| Setting | Required | Default | Description |
| --- | --- | --- | --- |
| `{{SETTING}}` | {{YES_NO}} | `{{DEFAULT}}` | {{DESCRIPTION}} |

## Architecture

{{SHORT_ARCHITECTURE_DESCRIPTION_OR_DIAGRAM}}

## Security and privacy

See [{{SECURITY_PATH}}]({{SECURITY_PATH}}). Do not use the project outside its
documented trust and deployment boundaries.

## Troubleshooting

### {{SYMPTOM}}

1. {{READ_ONLY_CHECK}}
2. {{TARGETED_RECOVERY}}
3. Preserve {{EVIDENCE}} before destructive recovery.

## Development workflow

- Project plan: [{{PLAN_PATH}}]({{PLAN_PATH}})
- Current status: [{{STATUS_PATH}}]({{STATUS_PATH}})
- Roles: [{{ROLES_PATH}}]({{ROLES_PATH}})
- Preflight: [{{PREFLIGHT_PATH}}]({{PREFLIGHT_PATH}})
- Decisions: [{{DECISIONS_PATH}}]({{DECISIONS_PATH}})

## License

{{LICENSE_AND_THIRD_PARTY_NOTICES}}
