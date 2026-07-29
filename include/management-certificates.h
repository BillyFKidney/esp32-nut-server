#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * Device-hosted HTTPS certificate material owned by the certificate module.
 * The pointers remain valid after a successful load until this module reloads
 * the material or the device restarts.
 */
typedef struct
{
    const uint8_t *certificate;
    size_t certificate_length;
    const uint8_t *private_key;
    size_t private_key_length;
} ManagementCertificateMaterial;

/**
 * Load the persisted self-signed certificate/key pair, or generate and persist
 * a replacement when either NVS blob is absent or incomplete.
 */
esp_err_t management_certificates_load_or_create(void);

/** Return the material owned by this module after a successful load. */
const ManagementCertificateMaterial *management_certificates_get_material(void);
