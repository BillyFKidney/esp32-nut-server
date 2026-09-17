#!/usr/bin/env python3
"""Validate the live ESP32-NUT ADMIN console without exposing credentials."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import http.client
import json
import os
import re
import ssl
import sys
from pathlib import Path
from urllib.parse import urlencode


PASSWORD_VARIABLE = "ESP32_NUT_ADMIN_CONSOLE_PASSWORD"
MAX_RESPONSE_SIZE = 96 * 1024


def request(
    device: str,
    fingerprint: str,
    method: str,
    path: str,
    *,
    headers: dict[str, str] | None = None,
    body: bytes = b"",
) -> tuple[int, list[tuple[str, str]], bytes]:
    connection = http.client.HTTPSConnection(
        device, 443, context=ssl._create_unverified_context(), timeout=30  # noqa: SLF001
    )
    try:
        connection.connect()
        certificate = connection.sock.getpeercert(binary_form=True)
        actual = hashlib.sha256(certificate).hexdigest()
        if not hmac.compare_digest(actual, fingerprint):
            raise RuntimeError("Device certificate fingerprint does not match.")
        connection.request(method, path, body=body, headers=headers or {})
        response = connection.getresponse()
        response_body = response.read(MAX_RESPONSE_SIZE + 1)
        if len(response_body) > MAX_RESPONSE_SIZE:
            raise RuntimeError(f"{path} exceeded the response-size safety limit.")
        return response.status, response.getheaders(), response_body
    finally:
        connection.close()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--device", required=True)
    parser.add_argument("--certificate-sha256", required=True)
    parser.add_argument(
        "--staged-ota-firmware",
        help="optional firmware image to check through the PSRAM browser-stage route",
    )
    parser.add_argument(
        "--install-staged-ota",
        action="store_true",
        help="after a successful staged check, commit that image for installation",
    )
    parser.add_argument(
        "--reject-staged-ota-firmware",
        help="firmware image expected to be rejected by the PSRAM browser-stage route",
    )
    arguments = parser.parse_args()
    if arguments.install_staged_ota and not arguments.staged_ota_firmware:
        parser.error("--install-staged-ota requires --staged-ota-firmware")
    if arguments.install_staged_ota and arguments.reject_staged_ota_firmware:
        parser.error("--install-staged-ota cannot be combined with --reject-staged-ota-firmware")

    password = os.environ.pop(PASSWORD_VARIABLE, "")
    if not password:
        print(f"Set {PASSWORD_VARIABLE} through a private environment.", file=sys.stderr)
        return 2
    fingerprint = arguments.certificate_sha256.replace(":", "").lower()
    if re.fullmatch(r"[0-9a-f]{64}", fingerprint) is None:
        print("The certificate SHA-256 fingerprint is invalid.", file=sys.stderr)
        return 2

    try:
        status, favicon_headers, favicon_body = request(
            arguments.device, fingerprint, "GET", "/favicon.ico"
        )
        require(status == 200, f"NUT favicon returned HTTP {status}.")
        favicon_content_type = next(
            (
                value
                for name, value in favicon_headers
                if name.lower() == "content-type"
            ),
            "",
        )
        require(favicon_content_type == "image/png", "NUT favicon is not a PNG.")
        require(
            favicon_body.startswith(b"\x89PNG\r\n\x1a\n"),
            "NUT favicon has an invalid PNG signature.",
        )

        login_body = urlencode({"password": password}).encode()
        status, response_headers, _ = request(
            arguments.device,
            fingerprint,
            "POST",
            "/login",
            headers={
                "Content-Type": "application/x-www-form-urlencoded",
                "Content-Length": str(len(login_body)),
            },
            body=login_body,
        )
        require(status == 303, f"ADMIN login returned HTTP {status}, expected 303.")
        set_cookie = next(
            (value for name, value in response_headers if name.lower() == "set-cookie"), ""
        )
        cookie = set_cookie.split(";", 1)[0]
        require(cookie.startswith("ESP32NUT_SESSION="), "ADMIN session cookie was not issued.")

        session_headers = {"Cookie": cookie}
        status, _, page_body = request(
            arguments.device, fingerprint, "GET", "/", headers=session_headers
        )
        require(status == 200, f"ADMIN page returned HTTP {status}.")
        page = page_body.decode("utf-8")
        required_markers = (
            "class=app-header",
            "id=panel-dashboard",
            "id=panel-status",
            "id=panel-logs",
            "JSON.stringify(x,null,2)",
            "copyLogsButton",
            "downloadLogsButton",
            "new Blob([lastLogsTranscript]",
        )
        for marker in required_markers:
            require(marker in page, f"ADMIN page is missing {marker!r}.")
        require("dashboardLogs" not in page, "Dashboard still contains log rendering.")
        require(page.count("/api/v1/admin/logs") == 1, "Full-log request is duplicated.")
        require(
            "<link rel=icon href=/favicon.ico>" in page,
            "ADMIN page does not declare the NUT favicon.",
        )
        require("HTTPS is active" not in page, "Removed HTTPS notice remains visible.")
        require(
            "All management actions remain protected" not in page,
            "Removed session notice remains visible.",
        )
        csrf_match = re.search(r"const csrf='([0-9a-f]{64})'", page)
        require(csrf_match is not None, "Rendered ADMIN page has no valid CSRF token.")
        csrf = csrf_match.group(1)

        if arguments.staged_ota_firmware:
            firmware = Path(arguments.staged_ota_firmware).read_bytes()
            require(firmware, "Staged OTA firmware is empty.")
            stage_headers = {
                "Cookie": cookie,
                "Content-Type": "application/octet-stream",
                "Content-Length": str(len(firmware)),
                "X-ESP32-NUT-CSRF": csrf,
            }
            status, _, stage_body = request(
                arguments.device,
                fingerprint,
                "POST",
                "/api/v1/ota/check",
                headers=stage_headers,
                body=firmware,
            )
            require(status == 200, f"PSRAM firmware stage returned HTTP {status}.")
            stage_json = json.loads(stage_body)
            stage = stage_json.get("stage")
            require(
                isinstance(stage, str) and re.fullmatch(r"[0-9a-f]{32}", stage),
                "PSRAM firmware stage did not return a valid opaque identifier.",
            )
            require(
                stage_json.get("status") == "checked",
                "PSRAM firmware stage did not report checked status.",
            )
            if arguments.install_staged_ota:
                install_headers = {
                    "Cookie": cookie,
                    "Content-Length": "0",
                    "X-ESP32-NUT-CSRF": csrf,
                    "X-ESP32-NUT-OTA-Stage": stage,
                }
                status, _, install_body = request(
                    arguments.device,
                    fingerprint,
                    "POST",
                    "/api/v1/ota/install",
                    headers=install_headers,
                )
                require(status == 200, f"Checked firmware install returned HTTP {status}.")
                require(
                    json.loads(install_body).get("status") == "installed",
                    "Checked firmware install did not report installed status.",
                )
                print("PASS: checked PSRAM firmware was committed for installation")
                return 0

        if arguments.reject_staged_ota_firmware:
            rejected_firmware = Path(arguments.reject_staged_ota_firmware).read_bytes()
            require(rejected_firmware, "Rejected staged OTA firmware is empty.")
            rejected_headers = {
                "Cookie": cookie,
                "Content-Type": "application/octet-stream",
                "Content-Length": str(len(rejected_firmware)),
                "X-ESP32-NUT-CSRF": csrf,
            }
            status, _, _ = request(
                arguments.device,
                fingerprint,
                "POST",
                "/api/v1/ota/check",
                headers=rejected_headers,
                body=rejected_firmware,
            )
            require(status == 422, f"Malformed PSRAM firmware stage returned HTTP {status}.")

        status, favicon_headers, favicon_body = request(
            arguments.device, fingerprint, "GET", "/favicon.ico"
        )
        require(status == 200, f"NUT favicon returned HTTP {status}.")
        favicon_type = next(
            (value for name, value in favicon_headers if name.lower() == "content-type"), ""
        )
        require(favicon_type == "image/png", "NUT favicon is not served as image/png.")
        require(
            favicon_body.startswith(b"\x89PNG\r\n\x1a\n"),
            "NUT favicon response is not a PNG image.",
        )

        status, _, status_body = request(
            arguments.device,
            fingerprint,
            "GET",
            "/api/v1/status",
            headers=session_headers,
        )
        require(status == 200, f"Status route returned HTTP {status}.")
        status_json = json.loads(status_body)
        require(isinstance(status_json.get("logs"), list), "Status logs field is absent.")
        require(len(status_json["logs"]) <= 6, "Status log window exceeds six entries.")

        status, _, logs_body = request(
            arguments.device,
            fingerprint,
            "GET",
            "/api/v1/admin/logs",
            headers=session_headers,
        )
        require(status == 200, f"Full-log route returned HTTP {status}.")
        logs_json = json.loads(logs_body)
        require(isinstance(logs_json.get("logs"), list), "Full-log array is absent.")
        require(logs_json.get("retained_capacity") == 24, "Retained capacity is not 24.")
        require(logs_json.get("status_window") == 6, "Status window is not 6.")
        require(len(logs_json["logs"]) <= 24, "Full-log response exceeds 24 entries.")

        status, _, wifi_scan_body = request(
            arguments.device,
            fingerprint,
            "GET",
            "/api/v1/admin/wifi/scan",
            headers=session_headers,
        )
        require(status == 200, f"Wi-Fi scan returned HTTP {status}.")
        wifi_scan_json = json.loads(wifi_scan_body)
        networks = wifi_scan_json.get("networks")
        require(isinstance(networks, list), "Wi-Fi scan network list is absent.")
        require(wifi_scan_json.get("maximum") == 20, "Wi-Fi scan limit is not 20.")
        require(len(networks) <= 20, "Wi-Fi scan returned more than 20 networks.")

        status, _, _ = request(
            arguments.device, fingerprint, "GET", "/api/v1/admin/logs"
        )
        require(status == 401, f"Unauthenticated full-log route returned HTTP {status}.")

        invalid_csrf_headers = {
            "Cookie": cookie,
            "X-ESP32-NUT-CSRF": "0" * 64,
            "Content-Length": "0",
        }
        status, _, _ = request(
            arguments.device,
            fingerprint,
            "POST",
            "/api/v1/admin/session/activity",
            headers=invalid_csrf_headers,
        )
        require(status == 403, f"Invalid CSRF request returned HTTP {status}.")

        print(
            "PASS: authenticated streamed page, NUT favicon, removed notices, unchanged "
            "six-entry status logs, 24-entry ADMIN logs, bounded Wi-Fi scan, "
            "unauthenticated rejection, and CSRF rejection."
        )
        return 0
    except (OSError, ssl.SSLError, http.client.HTTPException, ValueError, RuntimeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
