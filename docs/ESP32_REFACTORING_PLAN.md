# ESP32-NUT refactoring status

The management/Wi-Fi modular refactoring was published and target-tested in
`v2.7.1`. Its full extraction plan, module inventory, route inventory, and
validation history are preserved in
[archive/ESP32_REFACTORING_PLAN_V2_7_1.md](archive/ESP32_REFACTORING_PLAN_V2_7_1.md).

## Result

`management.c` is the deliberate orchestration boundary for root-page policy,
HTTPS lifecycle, and factory reset. Focused modules own credentials, sessions,
certificates, HTTP helpers, status/log snapshots, pages, auth/session routes,
status/time/token/Wi-Fi/OTA routes, and route registration.

The refactor preserves HTTPS `443`, NUT `3493`, refused `8080`, ADMIN/CSRF,
bearer scope and Authorization-header zeroization, Wi-Fi recovery, and
read-only UPS behavior. It does not add UPS controls or change the
factory-reset scope.

No management refactor is active. Begin future work from the compact active
roadmap, not this completed inventory.
