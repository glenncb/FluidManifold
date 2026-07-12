#!/usr/bin/env python
"""
PlatformIO post-build script: patches the esp_app_desc_t fields (version,
project name, build date/time) directly into the compiled firmware.bin.

PlatformIO's `framework = arduino` build does not populate esp_app_desc_t
from this project's own source the way `framework = espidf` does - every
build links against a precompiled Arduino core, so esp_app_desc_t.version
otherwise ends up permanently fixed to that prebuilt library's own build
metadata (observed as version "2f061cb", project_name "arduino-lib-builder"),
never changing no matter what this project's own REVISION is. Both novafwupd
(NOVAFWVER-tag confirmation) and the ESP-IDF OTA rollback-protection
(esp_ota_get_last_invalid_partition() version comparison in otalib.cpp) rely
on esp_app_desc_t.version being real and distinct per build, so this patches
it in using the same REVISION value that VERSION/NOVAFWVER are derived from.

Adapted from the technique described at:
https://community.platformio.org/t/how-to-engrave-application-name-version-in-esp32-partitions-with-arduino-framework/28516
(original script: https://github.com/ankostis/Freematics/blob/dev/firmware_v5/telelogger/patchappinfos.py)
"""
Import("env")

import functools
import hashlib
import operator
import re
import struct
from datetime import datetime
from pathlib import Path

# offsets within the final image - see esp_app_desc_t in esp_app_format.h
ESP_APP_DESC_OFFSET = 0x30  # version(32) name(32) time(16) date(16) ...
FIRST_SEGMENT_OFFSET = 0x18
ESP_IMAGE_HEADER_MAGIC = 0xE9
ESP_APP_DESC_MAGIC_WORD = 0xABCD5432
ESP_CHECKSUM_MAGIC = 0xEF
FILE_HASH_LENGTH = 32  # trailing SHA256, if the image has one appended

PROJECT_NAME = "FluidManifold"


def read_project_version(env):
    globals_h = Path(env["PROJECT_DIR"]) / "include" / "globals.h"
    text = globals_h.read_text()
    m = re.search(r"#define\s+REVISION\s+([0-9.]+)", text)
    if not m:
        raise RuntimeError(f"patch_appinfo: could not find REVISION in {globals_h}")
    return m.group(1)


def checksum_image(fd, nsegments, patch=False):
    fd.seek(FIRST_SEGMENT_OFFSET)
    state = ESP_CHECKSUM_MAGIC
    for _ in range(nsegments):
        size = struct.unpack("<4xI", fd.read(8))[0]
        data = fd.read(size)
        state = functools.reduce(operator.xor, data, state)
    # checksum byte lives at the next 16-byte-aligned offset
    align = (16 - 1) - (fd.tell() % 16)
    fd.seek(align, 1)
    if patch:
        fd.write(struct.pack("B", state))
    return state


def hash_image(fd, patch=False):
    fd.seek(0)
    size = Path(fd.name).stat().st_size - FILE_HASH_LENGTH
    digest = hashlib.sha256(fd.read(size)).digest()
    # fd is now positioned right before the trailing hash bytes
    if patch:
        fd.write(digest)


def patch_field(fd, offset, length, value):
    fd.seek(offset)
    fd.write(struct.pack(f"{length}s", value.encode("utf-8")))


def patch_app_infos(source, target, env):
    # SCons's target/source argument order for a post-action isn't reliable
    # to assume positionally (observed source[0] actually being the .elf, not
    # the .bin, in practice) - search both lists for the real .bin path
    # instead of trusting which slot it landed in.
    img_path = None
    for node_list in (target, source):
        for node in node_list:
            if str(node.path).lower().endswith(".bin"):
                img_path = node.path
                break
        if img_path:
            break
    if img_path is None:
        print("patch_appinfo: could not find a .bin path in target/source, skipping")
        return

    version = read_project_version(env)
    now = datetime.now()
    build_time = now.strftime("%H:%M:%S")
    build_date = now.strftime("%b %d %Y")

    with open(img_path, "r+b") as fd:
        header_magic, nsegments, is_hashed = struct.unpack("BB21xB", fd.read(24))
        if header_magic != ESP_IMAGE_HEADER_MAGIC:
            print(f"patch_appinfo: unexpected image header magic 0x{header_magic:02x}, skipping")
            return

        fd.seek(0x20)
        app_magic = struct.unpack("<I", fd.read(4))[0]
        if app_magic != ESP_APP_DESC_MAGIC_WORD:
            print(f"patch_appinfo: no esp_app_desc_t found (magic 0x{app_magic:08x}), skipping")
            return

        patch_field(fd, ESP_APP_DESC_OFFSET, 32, version)
        patch_field(fd, ESP_APP_DESC_OFFSET + 32, 32, PROJECT_NAME)
        patch_field(fd, ESP_APP_DESC_OFFSET + 64, 16, build_time)
        patch_field(fd, ESP_APP_DESC_OFFSET + 80, 16, build_date)

        checksum_image(fd, nsegments, patch=True)
        if is_hashed:
            hash_image(fd, patch=True)

    print(f"patch_appinfo: patched {img_path} - version={version!r} name={PROJECT_NAME!r}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", patch_app_infos)
