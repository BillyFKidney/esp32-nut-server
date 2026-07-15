# Web application and service addendum

Use for browser applications, APIs, databases, background workers, and hosted
services.

## Record the system contract

- Frontend/backend/runtime/framework versions: `{{STACK}}`
- Environments and URLs: `{{LOCAL_TEST_STAGING_PRODUCTION}}`
- Authentication/authorization provider: `{{AUTH_MODEL}}`
- Databases, queues, caches, object stores, and third parties: `{{DEPENDENCIES}}`
- Deployment and rollback mechanism: `{{DEPLOYMENT_MODEL}}`
- Data classification and retention: `{{DATA_POLICY}}`

## Guardrails

- Keep local, staging, and production credentials/data clearly separated.
- Treat schema, migration, auth, billing, deletion, email, and webhook changes as
  distinct risk boundaries.
- Do not run migrations or seed/reset commands against an unidentified database.
- Make retries and webhooks idempotent; authenticate and validate callbacks.
- Enforce authorization at the backend/control boundary, not only in UI.
- Avoid logging secrets, tokens, personal data, payment data, or full request
  bodies by default.

## Preflight additions

- [ ] Identify environment, account/project, region, database, and branch.
- [ ] Confirm dev servers and ports are not already owned.
- [ ] Verify environment-variable names without printing secret values.
- [ ] Check migration status and backup/rollback before schema changes.
- [ ] Record current deployed version and health indicators.

## Validation additions

- Unit, integration, API-contract, and browser tests
- Authentication, authorization, CSRF/CORS, input validation, rate limits, and
  session lifecycle
- Responsive and accessible UI across required browsers
- Migration forward/rollback and mixed-version compatibility
- Retry, timeout, duplicate delivery, partial failure, and offline behavior
- Observability: health checks, structured logs, metrics, traces, and actionable
  errors without sensitive data
- Staging or preview validation before production deployment
