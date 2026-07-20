# Project intake questionnaire

Complete what you know before the first Codex message. `Not decided` is a valid
answer. Do not include secrets.

## Identity

- Project name: `{{PROJECT_NAME}}`
- Repository or workspace location: `{{LOCATION}}`
- New project, inherited project, or fork: `{{PROJECT_ORIGIN}}`
- Project Maintainer: `{{MAINTAINER}}`
- Other Device Operators/testers: `{{OPERATORS}}`

## Problem and outcome

- Problem to solve: `{{PROBLEM}}`
- Who experiences the problem: `{{USERS}}`
- Desired outcome: `{{OUTCOME}}`
- How success will be measured: `{{SUCCESS_MEASURES}}`
- Explicitly out of scope: `{{EXCLUSIONS}}`

## Product and environment

- Project type: `{{WEB_IOS_AUTOMATION_EMBEDDED_HOME_ASSISTANT_OTHER}}`
- Languages/frameworks/tools: `{{STACK}}`
- Supported platforms and versions: `{{PLATFORMS}}`
- Development environments: `{{DEVELOPMENT_ENVIRONMENTS}}`
- Test/staging environments: `{{TEST_ENVIRONMENTS}}`
- Production/deployed environments: `{{PRODUCTION_ENVIRONMENTS}}`
- Hardware, devices, or external services: `{{DEPENDENCIES}}`

## Existing state

- What already works: `{{WORKING_STATE}}`
- Known failures or pain points: `{{KNOWN_PROBLEMS}}`
- Existing documentation: `{{EXISTING_DOCS}}`
- Existing tests/CI: `{{TESTS_AND_CI}}`
- Current branch/version/deployment if known: `{{CURRENT_STATE}}`
- Existing tags/releases and any expected-but-missing versions:
  `{{RELEASE_HISTORY_AND_GAPS}}`
- Important prior decisions: `{{PRIOR_DECISIONS}}`
- Existing services, listeners, deployment paths, automation, and Agent access
  that must remain available until explicitly retired: `{{CONTINUITY_INVENTORY}}`

## Access and authority

- What the Device Operator can physically or visually access:
  `{{OPERATOR_ACCESS}}`
- What the Project Maintainer can administer: `{{MAINTAINER_ACCESS}}`
- What Codex can reach through the workspace/tools: `{{AGENT_ACCESS}}`
- Actions requiring explicit approval: `{{APPROVAL_BOUNDARIES}}`
- Services that may not be retired without named explicit approval:
  `{{SERVICE_RETIREMENT_BOUNDARIES}}`
- Workflow changes that would remove Agent capabilities or transfer recurring
  work to a human: `{{WORKFLOW_OWNERSHIP_BOUNDARIES}}`
- Systems that are view-only: `{{VIEW_ONLY_SYSTEMS}}`

## Risk and security

- Credentials, personal data, production data, or signing material involved:
  `{{SENSITIVE_ASSETS_WITHOUT_VALUES}}`
- Network or trust boundaries: `{{TRUST_BOUNDARIES}}`
- Destructive or hard-to-reverse operations: `{{HIGH_RISK_ACTIONS}}`
- Required backup/recovery path: `{{RECOVERY_REQUIREMENTS}}`
- Legal, privacy, accessibility, or compliance requirements:
  `{{COMPLIANCE_REQUIREMENTS}}`

## Development preferences

- Preferred branch/PR size: `{{BRANCH_PREFERENCE}}`
- Preferred merge method: `{{MERGE_METHOD}}`
- Versioning model—strict Semantic Versioning, milestone-oriented, date-based,
  or another policy: `{{VERSIONING_MODEL}}`
- Mapping between milestones, implementation slices, and version numbers:
  `{{VERSION_MAPPING}}`
- When an accepted slice should be proposed for tagging/release, and when a
  release may intentionally be deferred: `{{RELEASE_DECISION_POLICY}}`
- Release/deployment preference: `{{RELEASE_PREFERENCE}}`
- Desired communication style: `{{COMMUNICATION_STYLE}}`
- When the Agent should stop and ask: `{{QUESTION_THRESHOLD}}`
- What “done” means to you: `{{DEFINITION_OF_DONE}}`

## Manual testing

- Primary GUI/UX tester: `{{TESTER}}`
- Devices/browsers/clients that must be tested: `{{TEST_MATRIX}}`
- Evidence the tester can provide: `{{SCREENSHOTS_LOGS_OBSERVATIONS}}`
- Accessibility or usability priorities: `{{UX_PRIORITIES}}`

## First requested milestone

- Milestone name: `{{MILESTONE}}`
- Desired capabilities: `{{CAPABILITIES}}`
- Constraints: `{{CONSTRAINTS}}`
- Known questions: `{{QUESTIONS}}`
- Desired stopping point for the first session: `{{FIRST_SESSION_GOAL}}`
