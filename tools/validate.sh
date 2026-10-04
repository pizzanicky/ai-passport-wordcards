#!/usr/bin/env bash
set -euo pipefail

mode="${1:---all}"
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
    echo "Usage: $0 [--all|--static|--firmware]" >&2
}

run_static_checks() {
    local test_dir
    test_dir="$(mktemp -d /tmp/wordcards-host.XXXXXX)"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain tests/test_wc_model.c main/wc_model.c -o "${test_dir}/model"
    "${test_dir}/model"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain -Itests/store_stubs tests/test_wc_store.c main/wc_store.c main/wc_model.c -o "${test_dir}/store"
    "${test_dir}/store"
    codec=managed_components/espressif__esp_audio_codec/include
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Imain -Itests/store_stubs -I"${codec}" -I"${codec}/decoder" -I"${codec}/decoder/impl" -I"${codec}/simple_dec" -I"${codec}/simple_dec/impl" tests/test_wc_decoder.c main/wc_decoder.c -o "${test_dir}/decoder"
    "${test_dir}/decoder"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Icomponents/bsp/src tests/test_bsp_display_rounding.c components/bsp/src/bsp_display_rounding.c -o "${test_dir}/rounding"
    "${test_dir}/rounding"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Icomponents/bsp/src tests/test_bsp_es8311_sleep_check.c components/bsp/src/bsp_es8311_sleep_check.c -o "${test_dir}/sleep"
    "${test_dir}/sleep"
    python3 tests/test_verify_firmware.py
    python3 tests/test_archive_firmware.py
    python3 tests/test_install_ranges.py
    python3 tests/check_assets.py
    if [ -d managed_components/lvgl__lvgl ]; then
        cmake -S tests/ui_host -B .cache/ui-host
        cmake --build .cache/ui-host -j 6
        mkdir -p build/ui-preview
        .cache/ui-host/ui_host build/ui-preview
    else
        echo "UI host: dependencies unavailable; run firmware configure first" >&2
        return 1
    fi
    rm -r "${test_dir}"
    echo "Host tests: PASS"
}

run_firmware_checks() (
    local validation_build_dir

    if ! command -v idf.py >/dev/null 2>&1; then
        echo "ERROR: idf.py is not available; activate ESP-IDF 5.5.3 first." >&2
        return 1
    fi

    validation_build_dir="$(mktemp -d /tmp/ai-passport-firmware.XXXXXX)"
    trap 'case "${validation_build_dir}" in /tmp/ai-passport-firmware.*) rm -rf -- "${validation_build_dir}" ;; esac' EXIT

    SDKCONFIG_DEFAULTS="${repo_root}/sdkconfig.defaults" \
        idf.py -B "${validation_build_dir}" \
        -D "SDKCONFIG=${validation_build_dir}/sdkconfig" build
    idf.py -B "${validation_build_dir}" merge-bin \
        -o "${validation_build_dir}/FoloToy-AI-Passport-full.bin"
    python3 tools/verify_firmware.py "${validation_build_dir}"
    python3 tools/package_install.py "${validation_build_dir}"
    python3 tools/check_flash_budget.py "${validation_build_dir}/FoloToy-AI-Passport.bin" "${validation_build_dir}/sdkconfig"
    PYTHONDONTWRITEBYTECODE=1 python3 tools/archive_firmware.py create \
        "${validation_build_dir}" --archive-root "${repo_root}/build/firmware"
    mkdir -p "${repo_root}/build"
    install -m 0644 \
        "${validation_build_dir}/FoloToy-AI-Passport-full.bin" \
        "${repo_root}/build/FoloToy-AI-Passport-full.bin"
    echo "Firmware build: PASS"
)

cd "${repo_root}"
case "${mode}" in
    --all)
        run_static_checks
        run_firmware_checks
        ;;
    --static)
        run_static_checks
        ;;
    --firmware)
        run_firmware_checks
        ;;
    *)
        usage
        exit 2
        ;;
esac
