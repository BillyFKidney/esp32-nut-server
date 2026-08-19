#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/** Handle the read-only ADMIN status response without refreshing session activity. */
esp_err_t management_status_handler(httpd_req_t *request);

/** Handle the bearer-authorized Agent status response without session activity. */
esp_err_t management_agent_status_handler(httpd_req_t *request);
