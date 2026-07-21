#!/usr/bin/env python3
"""Safely probe ESP32-NUT API-token authorization boundaries."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import http.client
import os
import re
import ssl
import sys


TOKEN_ENVIRONMENT_VARIABLE = "ESP32_NUT_OTA_TOKEN"
TOKEN_PATTERN = re.compile(r"esp32nut_v1_[0-9a-f]{64}\Z")
TOKEN_REDACTION_PATTERN = re.compile(r"esp32nut_v1_[0-9a-f]{64}")
MAX_RESPONSE_SIZE = 16 * 1024
ALLOWED_PATHS = {
    "/api/v1/agent/ota/install",
    "/api/v1/admin/tokens",
    "/api/v1/status",
    "/api/v1/admin/time",
    "/api/v1/admin/password",
}


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Send a small fingerprint-pinned HTTPS request using an API token "
            f"read only from {TOKEN_ENVIRONMENT_VARIABLE}."
        )
    )
    parser.add_argument("--device", required=True, help="ESP32 hostname or IP address")
    parser.add_argument(
        "--certificate-sha256",
        required=True,
        help="trusted peer-certificate SHA-256 fingerprint",
    )
    parser.add_argument("--port", type=int, default=443, help="HTTPS port")
    parser.add_argument("--method", choices=("GET", "POST", "DELETE"), default="POST")
    parser.add_argument("--path", choices=sorted(ALLOWED_PATHS), required=True)
    parser.add_argument("--content-type", help="optional Content-Type header")
    parser.add_argument(
        "--body-text",
        default="",
        help="small non-secret diagnostic body; never place a token here",
    )
    parser.add_argument("--timeout", type=float, default=15.0)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if "://" in arguments.device or "/" in arguments.device:
        print("--device must contain only a hostname or IP address.", file=sys.stderr)
        return 2
    if arguments.port < 1 or arguments.port > 65535:
        print("--port must be from 1 through 65535.", file=sys.stderr)
        return 2

    expected_fingerprint = arguments.certificate_sha256.replace(":", "").lower()
    if re.fullmatch(r"[0-9a-f]{64}", expected_fingerprint) is None:
        print("The certificate SHA-256 fingerprint is invalid.", file=sys.stderr)
        return 2

    token = os.environ.pop(TOKEN_ENVIRONMENT_VARIABLE, "")
    if TOKEN_PATTERN.fullmatch(token) is None:
        print(
            f"Set {TOKEN_ENVIRONMENT_VARIABLE} privately to a complete v1 API token.",
            file=sys.stderr,
        )
        return 2

    body = arguments.body_text.encode("utf-8")
    connection = http.client.HTTPSConnection(
        arguments.device,
        arguments.port,
        context=ssl._create_unverified_context(),  # noqa: SLF001
        timeout=arguments.timeout,
    )
    try:
        connection.connect()
        peer_certificate = connection.sock.getpeercert(binary_form=True)
        actual_fingerprint = hashlib.sha256(peer_certificate).hexdigest()
        if not hmac.compare_digest(actual_fingerprint, expected_fingerprint):
            print("The device certificate fingerprint does not match.", file=sys.stderr)
            return 1

        connection.putrequest(arguments.method, arguments.path, skip_accept_encoding=True)
        connection.putheader("Authorization", f"Bearer {token}")
        if arguments.content_type:
            connection.putheader("Content-Type", arguments.content_type)
        if arguments.method != "GET" or body:
            connection.putheader("Content-Length", str(len(body)))
        connection.putheader("User-Agent", "ESP32-NUT-API-Token-Probe/1")
        connection.putheader("Connection", "close")
        connection.endheaders(body)

        response = connection.getresponse()
        response_body = response.read(MAX_RESPONSE_SIZE + 1)
        if len(response_body) > MAX_RESPONSE_SIZE:
            print("The device response exceeded the safety limit.", file=sys.stderr)
            return 1
        print(f"HTTP {response.status} {response.reason}")
        if response_body:
            response_text = response_body.decode("utf-8", errors="replace")
            print(TOKEN_REDACTION_PATTERN.sub("[REDACTED API TOKEN]", response_text))
        return 0
    except (OSError, ssl.SSLError, http.client.HTTPException) as error:
        print(f"Probe failed: {error}", file=sys.stderr)
        return 1
    finally:
        connection.close()


if __name__ == "__main__":
    raise SystemExit(main())
