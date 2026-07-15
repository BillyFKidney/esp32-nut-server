# Apple-platform and automation addendum

Use for iOS, iPadOS, macOS, watchOS, visionOS, App Intents, Shortcuts,
AppleScript, JXA, and GUI automation.

## Record the platform contract

- Product type: `{{APP_SCRIPT_SHORTCUT_SERVICE_EXTENSION}}`
- Xcode/Swift or scripting runtime versions: `{{VERSIONS}}`
- Deployment targets and required OS versions: `{{TARGETS}}`
- Bundle identifiers, targets, schemes, and configurations:
  `{{IDENTIFIERS_WITHOUT_SECRETS}}`
- Entitlements/capabilities: `{{ENTITLEMENTS}}`
- Signing/notarization/distribution model: `{{MODEL}}`
- Required devices, Simulators, accounts, or cloud containers: `{{DEPENDENCIES}}`

## Apple-specific guardrails

- Treat signing certificates, provisioning profiles, App Store Connect keys,
  team identifiers, and private container data as sensitive.
- Do not change bundle identifiers, entitlements, deployment targets, privacy
  manifests, signing, or data containers as incidental fixes.
- Prefer semantic accessibility identifiers over visual coordinates for UI
  automation.
- Keep UI work on the main actor and preserve existing state ownership patterns.
- Validate on the minimum supported OS and at least one current OS.
- Separate Simulator proof from physical-device proof when hardware,
  permissions, notifications, background execution, or performance matters.

## AppleScript and macOS automation

Record:

- Target applications and supported versions
- Required Accessibility, Automation, Full Disk Access, Files and Folders, or
  Screen Recording permissions
- Whether automation uses application dictionaries, System Events/UI scripting,
  shell commands, or Shortcuts
- Locale, display, window, menu, and focus assumptions
- Idempotency and safe behavior when the target app is closed or UI changes

GUI scripting is brittle. Prefer application APIs/dictionaries, Shortcuts, or
structured file/service interfaces before coordinate-based interaction.

## Preflight additions

- [ ] Record macOS/iOS, Xcode, SDK, Simulator/device, scheme, and configuration.
- [ ] Check for an already running build, Simulator, preview, test, or automation
      process before starting another.
- [ ] Confirm the target app/account/data container and whether it is disposable.
- [ ] Confirm required privacy permissions without asking for secret values.
- [ ] Preserve user documents and avoid resetting Simulator/device data unless
      explicitly authorized.

## Validation additions

- Build and tests for intended schemes/configurations
- Simulator launch and UI interaction
- Physical-device validation where platform behavior differs
- Dynamic Type, VoiceOver labels, contrast, localization, light/dark mode, and
  relevant orientations/window sizes
- App lifecycle, background/foreground, permission denied/revoked, offline, and
  account/session changes
- AppleScript repeatability, timeout, focus loss, missing UI, and permission
  failure behavior
- Signing/archive/notarization or distribution checks only at the release gate
