#include "management.h"
#include "management-certificates.h"
#include "management-credentials.h"
#include "management-pages.h"
#include "management-session.h"
#include "management-status.h"
#include "management-routes.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "mbedtls/platform_util.h"
#include "nvs.h"

#define TAG "nut-management"

#define MANAGEMENT_NAMESPACE "management"
#define MANAGEMENT_DEVICE_NAME_KEY "device-name"

#define MANAGEMENT_HTTPS_PORT 443
#define MANAGEMENT_HTTPS_REQUEST_HEADER_LIMIT 4096U

_Static_assert(sizeof(MANAGEMENT_NAMESPACE) <= NVS_NS_NAME_MAX_SIZE,
               "Management NVS namespace exceeds the ESP-IDF limit");
_Static_assert(sizeof(MANAGEMENT_DEVICE_NAME_KEY) <= NVS_KEY_NAME_MAX_SIZE,
               "Device-name NVS key exceeds the ESP-IDF limit");

static httpd_handle_t management_https_server;

static esp_err_t management_open_nvs(nvs_open_mode_t mode, nvs_handle_t *handle)
{
    return nvs_open(MANAGEMENT_NAMESPACE, mode, handle);
}


static esp_err_t management_root_handler(httpd_req_t *request)
{
    if (!management_admin_password_is_configured())
    {
        char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
        char setup_header[192];
        management_session_start_setup(request, csrf, sizeof(csrf), setup_header,
                                       sizeof(setup_header));
        const esp_err_t send_result =
            management_pages_send_setup(request, csrf);
        mbedtls_platform_zeroize(csrf, sizeof(csrf));
        return send_result;
    }
    if (!management_session_is_authorized(request, true))
    {
        management_session_expire_cookie(request);
        const int retry_after =
            management_session_login_retry_after_seconds(esp_timer_get_time());
        if (retry_after > 0)
        {
            return management_pages_send_login_throttled(request, retry_after);
        }
        return management_pages_send_login(request);
    }

    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1];
    management_session_copy_csrf(csrf, sizeof(csrf));
    const esp_err_t send_result = management_pages_send_admin(request, csrf);
    mbedtls_platform_zeroize(csrf, sizeof(csrf));
    return send_result;
}

esp_err_t management_factory_reset(void)
{
    nvs_handle_t handle = 0;
    esp_err_t result = management_open_nvs(NVS_READWRITE, &handle);
    if (result != ESP_OK)
    {
        return result;
    }
    result = nvs_erase_all(handle);
    if (result == ESP_OK)
    {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    management_session_clear();
    management_session_record_login_success();
    return result;
}

esp_err_t management_server_start(void)
{
    if (management_https_server != NULL)
    {
        return ESP_OK;
    }

    management_status_initialize_hardware_diagnostics();

    ESP_RETURN_ON_ERROR(management_certificates_load_or_create(), TAG,
                        "Unable to prepare HTTPS certificate");

    const ManagementCertificateMaterial *certificate_material =
        management_certificates_get_material();

    httpd_ssl_config_t configuration = HTTPD_SSL_CONFIG_DEFAULT();
    configuration.httpd.server_port = MANAGEMENT_HTTPS_PORT;
    configuration.httpd.stack_size = 12288;
    /*
     * Chrome plus a trusted reverse proxy can exceed ESP-IDF's 1024-byte
     * default before a setup or authentication handler receives the request.
     * This remains a bounded, management-server-only limit; the HTTP captive
     * portal retains its smaller default.
     */
    configuration.httpd.max_req_hdr_len = MANAGEMENT_HTTPS_REQUEST_HEADER_LIMIT;
    /* Allow a bounded idle interval while a browser streams a large OTA image. */
    configuration.httpd.recv_wait_timeout = 15;
    configuration.httpd.max_open_sockets = 4;
    configuration.httpd.max_uri_handlers = MANAGEMENT_HTTPS_ROUTE_CAPACITY;
    configuration.httpd.lru_purge_enable = true;
    configuration.servercert = certificate_material->certificate;
    configuration.servercert_len = certificate_material->certificate_length;
    configuration.prvtkey_pem = certificate_material->private_key;
    configuration.prvtkey_len = certificate_material->private_key_length;

    esp_err_t result = httpd_ssl_start(&management_https_server, &configuration);
    if (result != ESP_OK)
    {
        management_https_server = NULL;
        ESP_LOGE(TAG, "Unable to start the HTTPS management server: %s", esp_err_to_name(result));
        return result;
    }

    result = management_routes_register(management_https_server,
                                        management_root_handler);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to register HTTPS management route: %s", esp_err_to_name(result));
        httpd_ssl_stop(management_https_server);
        management_https_server = NULL;
        return result;
    }

    ESP_LOGI(TAG, "LAN-only HTTPS administration is listening on TCP port %d", MANAGEMENT_HTTPS_PORT);
    return ESP_OK;
}
