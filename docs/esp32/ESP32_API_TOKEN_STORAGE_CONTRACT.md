# API-token storage-contract refactor

## Scope

This review-loop slice is limited to removing duplicated token-store lifecycle
logic in `src/api_tokens.c`. It may extract an internal generic store helper,
but it must not add token capabilities or alter routes, payloads, scopes, or
browser/session authorization.

## Locked compatibility contract

- Keep NVS namespace `management` and the independent keys `api-tokens` and
  `diag-tokens`, including blob version, record layout, and capacities of four
  OTA and two diagnostic records.
- Keep OTA scope `ota.install` and diagnostic scope `diagnostics.nut`; neither
  store may authorize the other family's routes.
- Keep verifier-only persistence, strict token syntax, salted SHA-256
  verification, constant-time comparison, zeroization, and one-time plaintext
  disclosure.
- Keep per-store case-insensitive name/identifier validation, corrupt-storage
  errors, ADMIN/CSRF route policy, and existing payloads.

## Acceptance

Verify create/list/delete/revocation and authorization isolation for both
stores, corrupt-store handling, bounded storage, clean ESP-IDF v6.0.2 build,
and the unchanged HTTPS 443/NUT 3493/refused-8080 boundaries before release.
