# `src/` code-complete review plan

## Purpose and boundary

This is a fresh review of every one of the 108 tracked files below `src/`, starting from
`main` commit `8a84e65a4f89cc159d6a727726aefb60d578c309` on the local branch
`review/src-code-complete`. The completed v2.8.2 tracker was preserved as
`.code-review-tracker.20260909-142413.bak`; it is historical evidence, not
input to this review.

The review is a quality-maintenance gate, not authorization to redesign the
inherited NUT daemon/driver architecture, change external contracts, loosen
authorization, or modify live devices. It preserves LAN-only HTTPS `443`,
read-only NUT `3493`, refusal of `8080`, and ADMIN/CSRF/bearer boundaries.

## Method

1. A scanner inventories all 108 tracked files under `src/` against the supplied 68
   checks. Each finding must be real, actionable, and have a file and line.
2. Language-specific rules are assessed in their applicable context. A C or
   C-header file is not reported merely for lacking Python or TypeScript
   annotations, `pathlib`, async/await, or class-only constructs.
3. Findings are persisted as `TODO` in the root tracker. The scanner does not
   edit product code.
4. The Project Maintainer reviews the scan and explicitly approves the fixer
   phase. No product-code change happens before that approval.
5. If approved, the fixer makes only surgical, behavior-preserving changes,
   records `DONE` or `SKIP`, and runs proportionate non-device validation.
6. An independent reviewer verifies every claimed fix and rescans the modified
   files. The loop has at most three iterations. Failed or new findings remain
   explicit; a maximum-iteration stop is `PARTIAL`, never silently clean.

## Evidence and acceptance

- The tracker records finding IDs, source locations, severity, disposition,
  iteration, and final counts.
- The review report distinguishes observed results from inference and items not
  tested. Generated ESP-IDF state remains untracked.
- Any later implementation slice needs a clean ESP-IDF v6.0.2 build before it
  can be proposed for commit. Hardware, OTA, flashing, push, merge, tag, and
  release remain separately authorized.

## Current stop point

The only authorized work before Maintainer approval is branch setup,
documentation, archival of the old tracker, scanning, and tracker creation.
The exact next action after the scan is: Maintainer reviews the tracker and
authorizes or rejects the fixer phase.
