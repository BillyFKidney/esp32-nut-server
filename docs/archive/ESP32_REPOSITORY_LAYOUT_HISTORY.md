# ESP32-NUT repository-layout history

This historical record preserves the rationale removed from the compact active
[repository-layout policy](../ESP32_REPOSITORY_LAYOUT.md) on 2026-08-16. It
does not authorize a future move, retirement, or service change.

## Why the upstream documents moved

The four upstream narrative documents are useful reference material but are
not the ESP32-NUT application landing page. They now live under
`docs/upstream/` so the root presents the application first. Their references
in Autotools, documentation generation, packaging guidance, and internal links
were updated together.

The application README remains at the root because GitHub and source-tree
tools use that location. `README.adoc` is now the ESP32-NUT AsciiDoc guide and
`README.md` is its Markdown companion.

## Files intentionally not moved in this pass

The inherited CI definitions `Jenkinsfile-dynamatrix`, `.travis.yml`,
`appveyor.yml`, and `.lgtm.yml` remain at the root until their external service
consumers are audited. Moving one of these files can silently disable a
Jenkins, Travis, AppVeyor, or legacy analysis job even when the GitHub Actions
workflows remain green.

The root `ci_build.sh` and `ci_build.adoc` also remain in place because the
CircleCI configuration and repository guidance invoke the script by its root
path. A later CI-specific change may move or retire these files after the
replacement workflow is verified and the Project Maintainer authorizes the
service change.
