#!/usr/bin/env bash
set -euo pipefail

component_dir="${1:-}"

if [[ -z "${component_dir}" ]]; then
  echo "Usage: $0 <component-dir>" >&2
  exit 1
fi

if [[ ! -d "${component_dir}" ]]; then
  echo "Component directory not found: ${component_dir}" >&2
  exit 1
fi

validate_file="${component_dir}/validate.yaml"
if [[ ! -f "${validate_file}" ]]; then
  echo "Missing validate.yaml for component: ${component_dir}" >&2
  exit 1
fi

echo "==> [${component_dir}] esphome config"
esphome config "${validate_file}"

echo "==> [${component_dir}] esphome compile"
esphome compile "${validate_file}"

echo "==> [${component_dir}] PASS"
