## Benefit

Management status and retained-log requests now use PSRAM for their bounded
bulk response storage, preserving scarce internal SRAM for timing-sensitive
firmware work.

## What changed

The 7,000-byte status JSON response and full-log snapshot explicitly request
byte-addressable PSRAM. ADMIN/CSRF and bearer authorization, response payloads,
error handling, zeroization, HTTPS `443`, read-only NUT `3493`, and refused
`8080` are preserved.

## Verification

Pending exact-tag build and acceptance.

## Artifact

Pending exact-tag artifact and SHA-256.
