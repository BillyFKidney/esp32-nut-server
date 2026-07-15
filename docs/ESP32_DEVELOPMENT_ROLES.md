# ESP32-NUT development team roles

This document defines who does what during ESP32-NUT development, testing, and
recovery. Roles describe capabilities and authority; one person may hold more
than one role in a session.

These names intentionally differ from the product's `ADMIN` and future `USER`
authorization roles. In product documentation, those uppercase names refer only
to accounts in the ESP32-NUT management interface.

## Device Operator

**Draft mapping:** the people previously described as `USER`, currently the
project owner and Russ.

The Device Operator has physical access and performs normal user-facing tests.
This is the primary Level 1 support and diagnostic role.

### Capabilities

- Physically inspect, connect, disconnect, power, and operate the ESP32 board,
  Mac mini, UPS, and network equipment
- Use UniFi to view device presence and the current IP address
- Use Safari, Google Chrome, Visual Studio Code outside its embedded terminal,
  and native macOS applications
- Use Home Assistant and Synology NAS with user/view access
- Observe AdGuard DNS behavior
- View Git state and history without changing the repository
- Test the browser GUI and judge overall usability

### Responsibilities

- Run the Device Operator section of
  [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) before a development session
- Report observations with timestamps instead of guessing at causes
- Perform physical actions only when the Maintainer or Agent gives a precise,
  scoped instruction or when a documented recovery step clearly applies
- Confirm expected warnings separately from unexpected failures
- Lead manual GUI and end-to-end UX acceptance testing
- Never paste passwords, Wi-Fi credentials, session cookies, API tokens, or
  private keys into chat or tracked documentation

A dedicated manual test document is still needed. The expected future file is
`ESP32_MANUAL_TEST_PLAN.md`, organized as short tasks with prerequisites,
actions, expected results, evidence, and pass/fail status.

## Project Maintainer

**Draft mapping:** the person previously described as human `ADMIN`, currently
the project owner.

The Project Maintainer includes all Device Operator capabilities and has
administrative and development authority. This is the primary Level 2 support
and diagnostic role and the human decision-maker for the project.

### Additional capabilities

- Use the Visual Studio Code embedded terminal and native Terminal comfortably
- Administer UniFi and other in-scope devices or services
- Change the Git repository, branches, commits, and remotes
- Authorize builds, flashes, OTA installation, destructive recovery, commits,
  pushes, pull requests, and other external changes
- Provide the Agent with screenshots, logs, current IP addresses, and physical
  observations that are not available from the workspace

### Responsibilities

- Decide scope, priorities, security policy, and acceptance criteria
- Review Agent recommendations and approve actions with meaningful physical,
  destructive, security, or external effects
- Ensure credentials and private material remain outside tracked files and chat
- Resolve conflicts between requested behavior and documented safety guardrails
- Confirm whether a result is ready to commit, push, merge, or release
- Escalate physical or third-party administration tasks that the Agent cannot
  perform directly

## Codex Agent

The Codex Agent is the software-engineering and diagnostic collaborator. It can
inspect and edit the shared workspace, run approved commands, build firmware,
analyze logs, and interact with connected development hardware when the local
environment permits.

### Capabilities

- Inspect repository history, source, documentation, generated build metadata,
  and diagnostic artifacts
- Implement and document changes within the requested scope
- Build and proportionally verify firmware and software changes
- Discover the current serial device, identify its owner, monitor logs, and use
  approved flash commands
- Test reachable LAN services when sandbox and network permissions allow
- Preserve user changes and explain observed, inferred, and untested findings

### Limits

- Has no physical hands and cannot inspect LEDs, cables, buttons, or screens
  without human-provided evidence
- Does not inherently have access to UniFi, Home Assistant, Synology, AdGuard,
  browsers, passwords, or other third-party systems
- Cannot assume an old IP address, COM-device suffix, firmware image, or external
  service state is still current
- Must not infer authority for destructive recovery, credential disclosure,
  pushes, releases, or materially broader external changes
- May need the Maintainer to approve commands outside the workspace sandbox or
  actions that alter connected hardware or remote systems

### Responsibilities

- Read the current status, roles, and preflight documents before hardware work
- Prefer network checks before claiming the serial port or changing cables
- Give physical instructions one action at a time, including expected evidence
  and when to stop
- Keep one known owner for the serial port and release it at session end
- Update [ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md) when repository,
  firmware, hardware, validation, or next-action state changes
- Record enough evidence that another chat or human can resume without
  reconstructing the previous session
- Stop and ask the Maintainer when a choice changes scope, security posture, or
  authorization

## Responsibility summary

| Activity | Device Operator | Project Maintainer | Codex Agent |
| --- | --- | --- | --- |
| Physical connections and buttons | Performs | Performs or authorizes | Gives scoped instructions; cannot perform |
| GUI and UX testing | Leads | Reviews and accepts | Defines tasks, diagnoses, and implements fixes |
| L1 observation and triage | Leads | Assists | Interprets evidence |
| L2 debugging | Supplies evidence | Leads human side | Leads technical investigation |
| Source implementation | Views only | Reviews or performs | Leads within authorized scope |
| Build and automated checks | Observes results | Authorizes or performs | Runs and interprets |
| Flash or OTA operation | Assists physically | Authorizes | Runs when authorized and available |
| Commit, push, merge, release | No | Authorizes and owns | Performs only when requested |
| Credentials and secrets | Keeps private | Administers and keeps private | Must not request or record unless strictly necessary and safely handled |
| Final acceptance | Provides UX result | Owns decision | Provides evidence and recommendation |

## Handoff vocabulary

Use these terms consistently:

- **Observed:** directly seen in a log, command result, GUI, or physical check
- **Inferred:** concluded from evidence but not directly verified
- **Not tested:** implemented or expected but not yet exercised
- **Blocked:** progress requires a specific human action, authorization, or
  external-state change
- **Next action:** the single concrete step that should happen first when work
  resumes
