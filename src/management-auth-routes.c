#include "management-auth-routes.h"

#include "management-credentials.h"
#include "management-http.h"
#include "management-pages.h"
#include "management-session.h"

#include <stdbool.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "mbedtls/platform_util.h"

#define TAG "nut-management"

esp_err_t management_auth_setup_handler(httpd_req_t *request)
{
    if (management_admin_password_is_configured())
    {
        return management_send_redirect(request, "/");
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char password[129] = {0};
    char confirmation[129] = {0};
    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1] = {0};
    const esp_err_t form_result = management_read_form_body(request, body, sizeof(body));
    const bool fields_present = form_result == ESP_OK &&
                                management_form_value(body, "password", password, sizeof(password)) &&
                                management_form_value(body, "confirm", confirmation, sizeof(confirmation)) &&
                                management_form_value(body, "csrf", csrf, sizeof(csrf));
    const bool csrf_valid = fields_present &&
                            management_session_setup_csrf_is_valid(request, csrf);
    const bool matches = csrf_valid && strcmp(password, confirmation) == 0;
    const esp_err_t password_result = matches ?
        management_credentials_set_admin_password(password) : ESP_ERR_INVALID_ARG;
    mbedtls_platform_zeroize(body, sizeof(body));
    mbedtls_platform_zeroize(password, sizeof(password));
    mbedtls_platform_zeroize(confirmation, sizeof(confirmation));
    mbedtls_platform_zeroize(csrf, sizeof(csrf));
    if (!csrf_valid)
    {
        return management_send_html_status(request, "403 Forbidden",
                                           "<h1>ESP32-NUT setup</h1><p>The setup form expired or was invalid. <a href='/'>Start again</a>.</p>");
    }
    if (!matches)
    {
        return management_send_html_status(request, "400 Bad Request",
                                           "<h1>ESP32-NUT setup</h1><p>Passwords must match and contain 12 to 128 characters. <a href='/'>Try again</a>.</p>");
    }
    if (password_result != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to store ADMIN password: %s", esp_err_to_name(password_result));
        return management_send_html_status(request, "500 Internal Server Error",
                                           "<h1>ESP32-NUT setup</h1><p>Unable to save the ADMIN password. <a href='/'>Try again</a>.</p>");
    }

    management_session_start();
    management_session_record_login_success();
    char session_header[176];
    management_session_set_cookie(request, session_header, sizeof(session_header));
    return management_send_redirect(request, "/");
}
esp_err_t management_auth_login_page_handler(httpd_req_t *request)
{
    return management_send_redirect(request, "/");
}

esp_err_t management_auth_login_handler(httpd_req_t *request)
{
    const int64_t now = esp_timer_get_time();
    const int retry_after = management_session_login_retry_after_seconds(now);
    if (retry_after > 0)
    {
        return management_pages_send_login_throttled(request, retry_after);
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char password[129] = {0};
    bool needs_migration = false;
    const bool valid = management_read_form_body(request, body, sizeof(body)) == ESP_OK &&
                       management_form_value(body, "password", password, sizeof(password)) &&
                       management_credentials_verify_admin_password(password,
                                                                    &needs_migration);
    if (valid && needs_migration)
    {
        const esp_err_t migration_result =
            management_credentials_set_admin_password(password);
        if (migration_result != ESP_OK)
        {
            ESP_LOGE(TAG, "Unable to migrate ADMIN password credential: %s",
                     esp_err_to_name(migration_result));
        }
    }
    mbedtls_platform_zeroize(body, sizeof(body));
    mbedtls_platform_zeroize(password, sizeof(password));
    if (!valid)
    {
        if (management_session_record_login_failure(now))
        {
            return management_pages_send_login_throttled(
                request, MANAGEMENT_LOGIN_COOLDOWN_SECONDS);
        }
        return management_send_html_status(request, "401 Unauthorized",
                                           "<h1>ESP32-NUT sign in</h1><p>Invalid password. <a href='/'>Try again</a>.</p>");
    }

    management_session_record_login_success();
    management_session_start();
    char session_header[176];
    management_session_set_cookie(request, session_header, sizeof(session_header));
    return management_send_redirect(request, "/");
}

esp_err_t management_auth_password_change_handler(httpd_req_t *request)
{
    if (!management_session_csrf_is_valid(request))
    {
        return management_send_json(request, "403 Forbidden",
                                    "{\"error\":\"Invalid session or CSRF token.\"}");
    }

    char body[MANAGEMENT_FORM_BODY_LIMIT + 1];
    char current_password[129] = {0};
    char new_password[129] = {0};
    char confirmation[129] = {0};
    const esp_err_t form_result = management_read_form_body(request, body, sizeof(body));
    const bool fields_present = form_result == ESP_OK &&
                                management_form_value(body, "current", current_password,
                                                      sizeof(current_password)) &&
                                management_form_value(body, "password", new_password,
                                                      sizeof(new_password)) &&
                                management_form_value(body, "confirm", confirmation,
                                                      sizeof(confirmation));
    mbedtls_platform_zeroize(body, sizeof(body));

    const bool new_password_valid = fields_present &&
                                    strlen(new_password) >= 12 &&
                                    strlen(new_password) <= 128 &&
                                    strcmp(new_password, confirmation) == 0;
    const bool current_password_valid = new_password_valid &&
                                        management_credentials_verify_admin_password(
                                            current_password, NULL);
    const bool password_changed = current_password_valid &&
                                  strcmp(current_password, new_password) != 0;
    esp_err_t password_result = ESP_ERR_INVALID_ARG;
    if (password_changed)
    {
        password_result = management_credentials_set_admin_password(new_password);
    }

    mbedtls_platform_zeroize(current_password, sizeof(current_password));
    mbedtls_platform_zeroize(new_password, sizeof(new_password));
    mbedtls_platform_zeroize(confirmation, sizeof(confirmation));

    if (!fields_present || !new_password_valid)
    {
        return management_send_json(
            request, "400 Bad Request",
            "{\"error\":\"New passwords must match and contain 12 to 128 characters.\"}");
    }
    if (!current_password_valid)
    {
        return management_send_json(request, "403 Forbidden",
                                    "{\"error\":\"The current ADMIN password is incorrect.\"}");
    }
    if (!password_changed)
    {
        return management_send_json(request, "400 Bad Request",
                                    "{\"error\":\"Choose a new password that differs from the current password.\"}");
    }
    if (password_result != ESP_OK)
    {
        return management_send_json(request, "500 Internal Server Error",
                                    "{\"error\":\"Unable to save the new ADMIN password.\"}");
    }

    management_session_start();
    char session_header[176];
    management_session_set_cookie(request, session_header, sizeof(session_header));
    return management_send_json(request, "200 OK",
                                "{\"message\":\"ADMIN password changed. The browser session was refreshed.\"}");
}
