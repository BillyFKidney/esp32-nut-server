# {{PROJECT_NAME}} manual test plan

This plan is for behavior that automated tests cannot adequately prove. Keep
tasks short enough that a Device Operator can perform them without interpreting
implementation details.

## Test run metadata

| Field | Value |
| --- | --- |
| Test run ID | {{DATE_VERSION_ENVIRONMENT}} |
| Tester | {{DEVICE_OPERATOR}} |
| Build/commit | {{VERSION_AND_COMMIT}} |
| Environment | {{DEVICE_OS_BROWSER_SERVICE_OR_HARDWARE}} |
| Preconditions | {{BACKUP_NETWORK_ACCOUNT_FIXTURE_OR_PHYSICAL_STATE}} |

## Result vocabulary

- **Pass:** observed result matches expected result.
- **Fail:** observed result differs; record evidence and stop conditions.
- **Blocked:** a prerequisite, permission, or external dependency prevents the
  test.
- **Not run:** intentionally not attempted.

## Test matrix

| Environment | Required | Result | Notes/evidence |
| --- | --- | --- | --- |
| {{ENVIRONMENT}} | Yes | Not run | |

## Test cases

### {{AREA}}-001: {{USER_OUTCOME}}

**Purpose:** {{WHAT_THIS_PROVES}}

**Risk:** {{LOW_MEDIUM_HIGH_AND_WHY}}

**Prerequisites:**

- {{PRECONDITION}}

**Steps:**

1. {{ONE_USER_ACTION}}
2. Observe and record {{SPECIFIC_EVIDENCE}}.
3. {{NEXT_ACTION}}

**Expected result:**

- {{OBSERVABLE_RESULT}}
- {{EXPECTED_FAILURE_OR_SECURITY_BEHAVIOR}}

**Stop and report if:**

- {{UNEXPECTED_OR_UNSAFE_CONDITION}}

**Actual result:** {{PASS_FAIL_BLOCKED_NOT_RUN}}

**Evidence:** {{SCREENSHOT_LOG_TIMESTAMP_VALUE_OR_LINK}}

**Notes/issue:** {{NOTES_OR_ISSUE_LINK}}

## Regression checklist

- [ ] Previously supported primary workflow still works.
- [ ] Failure behavior remains safe and understandable.
- [ ] Restart/relaunch/reconnect preserves expected state.
- [ ] Upgrade and rollback paths were tested when applicable.
- [ ] Accessibility and supported screen/device sizes were checked.

## Acceptance summary

- Passed: {{COUNT}}
- Failed: {{COUNT}}
- Blocked/not run: {{COUNT}}
- Maintainer decision: {{ACCEPT_REJECT_MORE_EVIDENCE}}
