# ESP32-NUT v2.8.18 release evidence

## Scope

This release closes the bounded OTA reboot-scheduling failure path. A verified
image that has been selected for the next boot is no longer left pending when
FreeRTOS cannot allocate the normal delayed reboot task.

## Candidate acceptance

- Scanner, fixer, and independent reviewer completed cleanly.
- The clean ESP-IDF v6.0.2 candidate build embedded
  `v2.8.17-10-gd04243886`, had 59% app-slot headroom, and SHA-256
  `3a69e2b096fb013e2508d0fa80a64823f41efa9693367f120449f2bf25a183fb`.
- Certificate-pinned scoped OTA installed that candidate on 3Dprinter. The
  post-reboot device ran from `app0`, reported update `installed`, healthy NUT,
  and UPS `OL`; HTTPS `443` and NUT `3493` responded, while `8080` was refused.

Final tag-specific build, artifact checksum, and release publication details
are added after the annotated `v2.8.18` tag is built and accepted.
