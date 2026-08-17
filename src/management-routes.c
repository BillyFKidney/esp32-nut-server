#include "management-routes.h"

#include "management-auth-routes.h"
#include "management-ota-routes.h"
#include "management-session-routes.h"
#include "management-status-routes.h"
#include "management-time-routes.h"
#include "management-token-routes.h"
#include "management-wifi-routes.h"

esp_err_t management_routes_register(
    httpd_handle_t server,
    esp_err_t (*root_handler)(httpd_req_t *request))
{
    const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_handler};
    const httpd_uri_t setup = {.uri = "/setup", .method = HTTP_POST, .handler = management_auth_setup_handler};
    const httpd_uri_t login_page = {.uri = "/login", .method = HTTP_GET, .handler = management_auth_login_page_handler};
    const httpd_uri_t login = {.uri = "/login", .method = HTTP_POST, .handler = management_auth_login_handler};
    const httpd_uri_t password = {.uri = "/api/v1/admin/password", .method = HTTP_POST, .handler = management_auth_password_change_handler};
    const httpd_uri_t logout = {.uri = "/logout", .method = HTTP_POST, .handler = management_session_logout_handler};
    const httpd_uri_t status = {.uri = "/api/v1/status", .method = HTTP_GET, .handler = management_status_handler};
    const httpd_uri_t session_activity = {.uri = "/api/v1/admin/session/activity", .method = HTTP_POST, .handler = management_session_activity_handler};
    const httpd_uri_t time_configuration = {.uri = "/api/v1/admin/time", .method = HTTP_POST, .handler = management_time_config_handler};
    const httpd_uri_t ota_check = {.uri = "/api/v1/ota/check", .method = HTTP_POST, .handler = management_ota_check_handler};
    const httpd_uri_t ota = {.uri = "/api/v1/ota/install", .method = HTTP_POST, .handler = management_ota_install_handler};
    const httpd_uri_t token_list = {.uri = "/api/v1/admin/tokens", .method = HTTP_GET, .handler = management_token_list_handler};
    const httpd_uri_t token_create = {.uri = "/api/v1/admin/tokens", .method = HTTP_POST, .handler = management_token_create_handler};
    const httpd_uri_t token_delete = {.uri = "/api/v1/admin/tokens", .method = HTTP_DELETE, .handler = management_token_delete_handler};
    const httpd_uri_t wifi_scan = {.uri = "/api/v1/admin/wifi/scan", .method = HTTP_GET, .handler = management_wifi_scan_handler};
    const httpd_uri_t wifi_configuration = {.uri = "/api/v1/admin/wifi", .method = HTTP_POST, .handler = management_wifi_configure_handler};
    const httpd_uri_t agent_ota = {.uri = "/api/v1/agent/ota/install", .method = HTTP_POST, .handler = management_agent_ota_install_handler};
    const httpd_uri_t *routes[] = {
        &root, &setup, &login_page, &login, &password, &logout, &status,
        &session_activity, &time_configuration, &ota_check, &ota, &token_list,
        &token_create, &token_delete, &wifi_scan, &wifi_configuration, &agent_ota};
    _Static_assert(sizeof(routes) / sizeof(routes[0]) <=
                       MANAGEMENT_HTTPS_ROUTE_CAPACITY,
                   "HTTPS route count exceeds configured handler capacity");

    for (size_t index = 0; index < sizeof(routes) / sizeof(routes[0]); index++)
    {
        const esp_err_t result = httpd_register_uri_handler(server, routes[index]);
        if (result != ESP_OK)
        {
            return result;
        }
    }
    return ESP_OK;
}
