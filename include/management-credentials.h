#pragma once

#include <stdbool.h>

#include "esp_err.h"

/**
 * Return whether a valid current or legacy ADMIN credential is persisted.
 * This is also declared by management.h as part of the management service API.
 */
bool management_admin_password_is_configured(void);

/** Persist a new current-format ADMIN password credential. */
esp_err_t management_credentials_set_admin_password(const char *password);

/**
 * Verify an ADMIN password and report whether a successful legacy credential
 * login should be migrated to the current credential format.
 */
bool management_credentials_verify_admin_password(const char *password,
                                                  bool *needs_migration);
