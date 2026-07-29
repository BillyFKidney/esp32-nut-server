# Agent startup context audit — 2026-07-29

This audit measures the documentation set shown in the Project Maintainer's
pre-optimization startup prompt and the fast-start set established by the
2026-07-29 documentation rewrite.

Exact byte and line counts come from the repository files. Token counts are
conservative estimates using `UTF-8 bytes ÷ 4`; the exact count varies by model
and tokenizer. Byte reduction is therefore the primary reproducible measure.

## Before optimization

The startup prompt required all six files below.

| File | Lines | Bytes | Estimated tokens |
| --- | ---: | ---: | ---: |
| `AGENTS.md` | 123 | 5,585 | 1,396 |
| `docs/ESP32_CURRENT_STATUS.md` | 1,595 | 101,910 | 25,478 |
| `docs/ESP32_DEVELOPMENT_PLAN.md` | 438 | 29,121 | 7,280 |
| `docs/ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md` | 446 | 21,381 | 5,345 |
| `docs/ESP32_DEVELOPMENT_ROLES.md` | 157 | 7,847 | 1,962 |
| `docs/ESP32_PREFLIGHT.md` | 191 | 8,716 | 2,179 |
| **Total** | **2,950** | **174,560** | **43,640** |

## After optimization

### Default fast start

Normal development startup now requires only `AGENTS.md` and
`docs/ESP32_CURRENT_STATUS.md`. Other documents are loaded only when the task
routing table calls for them.

| File | Lines | Bytes | Estimated tokens |
| --- | ---: | ---: | ---: |
| `AGENTS.md` | 91 | 5,422 | 1,356 |
| `docs/ESP32_CURRENT_STATUS.md` | 106 | 5,863 | 1,466 |
| **Total** | **197** | **11,285** | **2,821** |

Compared with the former six-file startup prompt, the default context is
**163,275 bytes smaller (93.5%)** and saves approximately **40,819 tokens**.

The status file itself fell from **101,910 bytes to 5,863 bytes**, a reduction
of **96,047 bytes (94.2%)**. Its line count fell from **1,595 to 106**.

### Same six files, after slimming

This comparison isolates file-size improvements even if an older prompt still
loads all six documents.

| File | Before bytes | After bytes | Reduction |
| --- | ---: | ---: | ---: |
| `AGENTS.md` | 5,585 | 5,422 | 163 (2.9%) |
| `docs/ESP32_CURRENT_STATUS.md` | 101,910 | 5,863 | 96,047 (94.2%) |
| `docs/ESP32_DEVELOPMENT_PLAN.md` | 29,121 | 19,187 | 9,934 (34.1%) |
| `docs/ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md` | 21,381 | 17,726 | 3,655 (17.1%) |
| `docs/ESP32_DEVELOPMENT_ROLES.md` | 7,847 | 7,847 | Unchanged; now conditional |
| `docs/ESP32_PREFLIGHT.md` | 8,716 | 8,716 | Unchanged; now conditional |
| **Total** | **174,560** | **64,761** | **109,799 (62.9%)** |

The same-six-file token estimate falls from approximately **43,640 to 16,190**.

## Preservation result

No project information was discarded to obtain these reductions. Historical
status, completed roadmap entries, completed QA evidence, the former AGENTS
instructions, and the README changelog are retained in this archive. Archive
bytes do not consume startup context unless an agent deliberately opens those
reference files.
