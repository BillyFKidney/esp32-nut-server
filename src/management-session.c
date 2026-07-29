#include "management-session.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "mbedtls/platform_util.h"

#define MANAGEMENT_SESSION_BYTES 32U
#define MANAGEMENT_SESSION_IDLE_US ((int64_t)MANAGEMENT_SESSION_IDLE_SECONDS * 1000000LL)
#define MANAGEMENT_LOGIN_MAX_FAILURES 5U
#define MANAGEMENT_LOGIN_COOLDOWN_US ((int64_t)MANAGEMENT_LOGIN_COOLDOWN_SECONDS * 1000000LL)

_Static_assert(MANAGEMENT_SESSION_HEX_LENGTH == MANAGEMENT_SESSION_BYTES * 2U,
               "Session token length must match the random token size");

typedef struct
{
    bool active;
    char cookie[MANAGEMENT_SESSION_HEX_LENGTH + 1U];
    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1U];
    int64_t last_activity_us;
} ManagementSession;

static ManagementSession management_session;
static portMUX_TYPE management_session_lock = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE management_login_lock = portMUX_INITIALIZER_UNLOCKED;
static unsigned int management_login_failures;
static int64_t management_login_cooldown_until_us;

static void management_session_bytes_to_hex(const uint8_t *source,
                                            size_t source_length,
                                            char *destination,
                                            size_t destination_length)
{
    static const char hexadecimal[] = "0123456789abcdef";
    if (destination_length < source_length * 2U + 1U)
    {
        if (destination_length > 0)
        {
            destination[0] = '\0';
        }
        return;
    }

    for (size_t index = 0; index < source_length; index++)
    {
        destination[index * 2U] = hexadecimal[source[index] >> 4U];
        destination[index * 2U + 1U] = hexadecimal[source[index] & 0x0fU];
    }
    destination[source_length * 2U] = '\0';
}

static bool management_session_constant_time_equal(const uint8_t *left,
                                                   const uint8_t *right,
                                                   size_t length)
{
    uint8_t difference = 0;
    for (size_t index = 0; index < length; index++)
    {
        difference |= left[index] ^ right[index];
    }
    return difference == 0;
}

static bool management_session_cookie_value(httpd_req_t *request,
                                            const char *name,
                                            char *destination,
                                            size_t destination_size)
{
    const size_t cookie_length = httpd_req_get_hdr_value_len(request, "Cookie");
    if (cookie_length == 0 || cookie_length > 256U || destination_size == 0)
    {
        return false;
    }

    char cookie_header[257];
    if (httpd_req_get_hdr_value_str(request, "Cookie", cookie_header,
                                    sizeof(cookie_header)) != ESP_OK)
    {
        return false;
    }

    const size_t name_length = strlen(name);
    char *entry = cookie_header;
    while (*entry != '\0')
    {
        while (*entry == ' ' || *entry == ';')
        {
            entry++;
        }
        char *separator = strchr(entry, '=');
        if (separator == NULL)
        {
            break;
        }
        char *end = strchr(separator + 1, ';');
        if (end == NULL)
        {
            end = entry + strlen(entry);
        }
        const size_t entry_name_length = (size_t)(separator - entry);
        const size_t value_length = (size_t)(end - separator - 1);
        if (entry_name_length == name_length && strncmp(entry, name, name_length) == 0 &&
            value_length + 1U <= destination_size)
        {
            memcpy(destination, separator + 1, value_length);
            destination[value_length] = '\0';
            return true;
        }
        entry = end;
    }
    return false;
}

static bool management_session_is_hex_token(const char *token)
{
    if (token == NULL || strlen(token) != MANAGEMENT_SESSION_HEX_LENGTH)
    {
        return false;
    }
    for (size_t index = 0; index < MANAGEMENT_SESSION_HEX_LENGTH; index++)
    {
        const char character = token[index];
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f')))
        {
            return false;
        }
    }
    return true;
}

void management_session_start(void)
{
    uint8_t cookie[MANAGEMENT_SESSION_BYTES];
    uint8_t csrf[MANAGEMENT_SESSION_BYTES];
    esp_fill_random(cookie, sizeof(cookie));
    esp_fill_random(csrf, sizeof(csrf));

    taskENTER_CRITICAL(&management_session_lock);
    management_session_bytes_to_hex(cookie, sizeof(cookie), management_session.cookie,
                                    sizeof(management_session.cookie));
    management_session_bytes_to_hex(csrf, sizeof(csrf), management_session.csrf,
                                    sizeof(management_session.csrf));
    management_session.last_activity_us = esp_timer_get_time();
    management_session.active = true;
    taskEXIT_CRITICAL(&management_session_lock);
    mbedtls_platform_zeroize(cookie, sizeof(cookie));
    mbedtls_platform_zeroize(csrf, sizeof(csrf));
}

void management_session_set_cookie(httpd_req_t *request, char *session_header,
                                   size_t session_header_size)
{
    taskENTER_CRITICAL(&management_session_lock);
    snprintf(session_header, session_header_size,
             "ESP32NUT_SESSION=%s; Path=/; Secure; HttpOnly; SameSite=Strict",
             management_session.cookie);
    taskEXIT_CRITICAL(&management_session_lock);
    httpd_resp_set_hdr(request, "Set-Cookie", session_header);
}

void management_session_expire_cookie(httpd_req_t *request)
{
    httpd_resp_set_hdr(request, "Set-Cookie",
                       "ESP32NUT_SESSION=; Path=/; Max-Age=0; Secure; HttpOnly; SameSite=Strict");
}

void management_session_start_setup(httpd_req_t *request, char *csrf,
                                    size_t csrf_size, char *setup_header,
                                    size_t setup_header_size)
{
    char cookie[MANAGEMENT_SESSION_HEX_LENGTH + 1U];
    if (!management_session_cookie_value(request, "ESP32NUT_SETUP", cookie,
                                         sizeof(cookie)) ||
        !management_session_is_hex_token(cookie))
    {
        uint8_t cookie_bytes[MANAGEMENT_SESSION_BYTES];
        esp_fill_random(cookie_bytes, sizeof(cookie_bytes));
        management_session_bytes_to_hex(cookie_bytes, sizeof(cookie_bytes), cookie,
                                        sizeof(cookie));
        mbedtls_platform_zeroize(cookie_bytes, sizeof(cookie_bytes));
    }
    snprintf(csrf, csrf_size, "%s", cookie);
    snprintf(setup_header, setup_header_size,
             "ESP32NUT_SETUP=%s; Path=/; Max-Age=300; Secure; HttpOnly; SameSite=Strict",
             cookie);
    httpd_resp_set_hdr(request, "Set-Cookie", setup_header);
    mbedtls_platform_zeroize(cookie, sizeof(cookie));
}

bool management_session_setup_csrf_is_valid(httpd_req_t *request,
                                            const char *csrf)
{
    char cookie[MANAGEMENT_SESSION_HEX_LENGTH + 1U];
    if (!management_session_is_hex_token(csrf) ||
        !management_session_cookie_value(request, "ESP32NUT_SETUP", cookie,
                                         sizeof(cookie)) ||
        !management_session_is_hex_token(cookie))
    {
        return false;
    }

    const bool valid = management_session_constant_time_equal(
        (const uint8_t *)cookie, (const uint8_t *)csrf,
        MANAGEMENT_SESSION_HEX_LENGTH);
    mbedtls_platform_zeroize(cookie, sizeof(cookie));
    return valid;
}

int management_session_login_retry_after_seconds(int64_t now)
{
    int retry_after = 0;
    taskENTER_CRITICAL(&management_login_lock);
    if (now < management_login_cooldown_until_us)
    {
        retry_after = (int)((management_login_cooldown_until_us - now + 999999LL) /
                            1000000LL);
    }
    taskEXIT_CRITICAL(&management_login_lock);
    return retry_after;
}

bool management_session_record_login_failure(int64_t now)
{
    bool cooldown_started = false;
    taskENTER_CRITICAL(&management_login_lock);
    management_login_failures++;
    if (management_login_failures >= MANAGEMENT_LOGIN_MAX_FAILURES)
    {
        management_login_failures = 0;
        management_login_cooldown_until_us = now + MANAGEMENT_LOGIN_COOLDOWN_US;
        cooldown_started = true;
    }
    taskEXIT_CRITICAL(&management_login_lock);
    return cooldown_started;
}

void management_session_record_login_success(void)
{
    taskENTER_CRITICAL(&management_login_lock);
    management_login_failures = 0;
    management_login_cooldown_until_us = 0;
    taskEXIT_CRITICAL(&management_login_lock);
}

void management_session_clear(void)
{
    taskENTER_CRITICAL(&management_session_lock);
    mbedtls_platform_zeroize(&management_session, sizeof(management_session));
    taskEXIT_CRITICAL(&management_session_lock);
}

uint32_t management_session_remaining_seconds(void)
{
    uint32_t remaining_seconds = 0;
    taskENTER_CRITICAL(&management_session_lock);
    if (management_session.active)
    {
        const int64_t elapsed_us =
            esp_timer_get_time() - management_session.last_activity_us;
        if (elapsed_us < MANAGEMENT_SESSION_IDLE_US)
        {
            const int64_t remaining_us = MANAGEMENT_SESSION_IDLE_US - elapsed_us;
            remaining_seconds = remaining_us > 0
                                    ? (uint32_t)(remaining_us / 1000000LL)
                                    : 0;
        }
    }
    taskEXIT_CRITICAL(&management_session_lock);
    return remaining_seconds;
}

void management_session_copy_csrf(char *csrf, size_t csrf_size)
{
    taskENTER_CRITICAL(&management_session_lock);
    snprintf(csrf, csrf_size, "%s", management_session.csrf);
    taskEXIT_CRITICAL(&management_session_lock);
}

bool management_session_is_authorized(httpd_req_t *request,
                                      bool refresh_activity)
{
    char value[MANAGEMENT_SESSION_HEX_LENGTH + 1U];
    if (!management_session_cookie_value(request, "ESP32NUT_SESSION", value,
                                         sizeof(value)) ||
        strlen(value) != MANAGEMENT_SESSION_HEX_LENGTH)
    {
        return false;
    }

    bool authorized = false;
    taskENTER_CRITICAL(&management_session_lock);
    const int64_t now = esp_timer_get_time();
    if (management_session.active &&
        now - management_session.last_activity_us <= MANAGEMENT_SESSION_IDLE_US &&
        management_session_constant_time_equal(
            (const uint8_t *)value,
            (const uint8_t *)management_session.cookie,
            MANAGEMENT_SESSION_HEX_LENGTH))
    {
        if (refresh_activity)
        {
            management_session.last_activity_us = now;
        }
        authorized = true;
    }
    else if (management_session.active &&
             now - management_session.last_activity_us > MANAGEMENT_SESSION_IDLE_US)
    {
        mbedtls_platform_zeroize(&management_session, sizeof(management_session));
    }
    taskEXIT_CRITICAL(&management_session_lock);
    mbedtls_platform_zeroize(value, sizeof(value));
    return authorized;
}

bool management_session_csrf_is_valid(httpd_req_t *request)
{
    if (!management_session_is_authorized(request, true))
    {
        return false;
    }

    char csrf[MANAGEMENT_SESSION_HEX_LENGTH + 1U] = {0};
    if (httpd_req_get_hdr_value_str(request, "X-ESP32-NUT-CSRF", csrf,
                                    sizeof(csrf)) != ESP_OK ||
        strlen(csrf) != MANAGEMENT_SESSION_HEX_LENGTH)
    {
        return false;
    }

    bool matches;
    taskENTER_CRITICAL(&management_session_lock);
    matches = management_session_constant_time_equal(
        (const uint8_t *)csrf, (const uint8_t *)management_session.csrf,
        MANAGEMENT_SESSION_HEX_LENGTH);
    taskEXIT_CRITICAL(&management_session_lock);
    return matches;
}
