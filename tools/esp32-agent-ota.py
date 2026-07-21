#!/usr/bin/env python3
"""Install ESP32-NUT firmware through the scoped API-token OTA route."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import http.client
import os
from pathlib import Path
import re
import ssl
import sys


TOKEN_ENVIRONMENT_VARIABLE = "ESP32_NUT_OTA_TOKEN"
TOKEN_PATTERN = re.compile(r"esp32nut_v1_[0-9a-f]{64}\Z")
TOKEN_REDACTION_PATTERN = re.compile(r"esp32nut_v1_[0-9a-f]{64}")
OTA_ROUTE = "/api/v1/agent/ota/install"
UPLOAD_CHUNK_SIZE = 64 * 1024
MAX_RESPONSE_SIZE = 16 * 1024


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Upload one ESP32-NUT application image over the scoped HTTPS "
            "Agent OTA route. The API token is read only from the "
            f"{TOKEN_ENVIRONMENT_VARIABLE} environment variable."
        )
    )
    parser.add_argument("--device", required=True, help="ESP32 hostname or IP address")
    parser.add_argument("--firmware", required=True, type=Path, help="application .bin path")
    parser.add_argument("--port", type=int, default=443, help="HTTPS port (default: 443)")
    trust = parser.add_mutually_exclusive_group(required=True)
    trust.add_argument(
        "--ca-file",
        type=Path,
        help="trusted CA/certificate PEM",
    )
    trust.add_argument(
        "--certificate-sha256",
        help="trusted peer-certificate SHA-256 fingerprint for the v2.x self-signed certificate",
    )
    trust.add_argument(
        "--insecure-self-signed",
        action="store_true",
        help="disable peer authentication explicitly (unsafe; temporary diagnostics only)",
    )
    parser.add_argument(
        "--timeout", type=float, default=120.0, help="connection timeout in seconds"
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if "://" in arguments.device or "/" in arguments.device:
        print("--device must contain only a hostname or IP address.", file=sys.stderr)
        return 2
    if arguments.port < 1 or arguments.port > 65535:
        print("--port must be from 1 through 65535.", file=sys.stderr)
        return 2
    if not arguments.firmware.is_file():
        print("The firmware path is not a readable file.", file=sys.stderr)
        return 2
    firmware_size = arguments.firmware.stat().st_size
    if firmware_size <= 0:
        print("The firmware image is empty.", file=sys.stderr)
        return 2

    token = os.environ.pop(TOKEN_ENVIRONMENT_VARIABLE, "")
    if TOKEN_PATTERN.fullmatch(token) is None:
        print(
            f"Set {TOKEN_ENVIRONMENT_VARIABLE} privately to a complete v1 API token.",
            file=sys.stderr,
        )
        return 2

    expected_fingerprint = ""
    if arguments.certificate_sha256 is not None:
        expected_fingerprint = arguments.certificate_sha256.replace(":", "").lower()
        if re.fullmatch(r"[0-9a-f]{64}", expected_fingerprint) is None:
            print("The certificate SHA-256 fingerprint is invalid.", file=sys.stderr)
            return 2

    if arguments.ca_file is not None:
        if not arguments.ca_file.is_file():
            print("The CA file is not readable.", file=sys.stderr)
            return 2
        tls_context = ssl.create_default_context(cafile=str(arguments.ca_file))
    else:
        tls_context = ssl._create_unverified_context()  # noqa: SLF001
        if arguments.insecure_self_signed:
            print(
                "Warning: TLS peer authentication is disabled for this request.",
                file=sys.stderr,
            )

    connection = http.client.HTTPSConnection(
        arguments.device,
        arguments.port,
        context=tls_context,
        timeout=arguments.timeout,
    )
    try:
        if expected_fingerprint:
            connection.connect()
            peer_certificate = connection.sock.getpeercert(binary_form=True)
            actual_fingerprint = hashlib.sha256(peer_certificate).hexdigest()
            if not hmac.compare_digest(actual_fingerprint, expected_fingerprint):
                print("The device certificate fingerprint does not match.", file=sys.stderr)
                return 1
        connection.putrequest("POST", OTA_ROUTE, skip_accept_encoding=True)
        connection.putheader("Authorization", f"Bearer {token}")
        connection.putheader("Content-Type", "application/octet-stream")
        connection.putheader("Content-Length", str(firmware_size))
        connection.putheader("User-Agent", "ESP32-NUT-Agent-OTA/1")
        connection.putheader("Connection", "close")
        connection.endheaders()
        with arguments.firmware.open("rb") as firmware:
            while chunk := firmware.read(UPLOAD_CHUNK_SIZE):
                connection.send(chunk)

        response = connection.getresponse()
        response_body = response.read(MAX_RESPONSE_SIZE + 1)
        if len(response_body) > MAX_RESPONSE_SIZE:
            print("The device response exceeded the safety limit.", file=sys.stderr)
            return 1
        print(f"HTTP {response.status} {response.reason}")
        if response_body:
            response_text = response_body.decode("utf-8", errors="replace")
            print(TOKEN_REDACTION_PATTERN.sub("[REDACTED API TOKEN]", response_text))
        return 0 if 200 <= response.status < 300 else 1
    except (OSError, ssl.SSLError, http.client.HTTPException) as error:
        print(f"OTA request failed: {error}", file=sys.stderr)
        return 1
    finally:
        connection.close()


if __name__ == "__main__":
    raise SystemExit(main())
