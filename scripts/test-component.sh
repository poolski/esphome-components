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

for extra in "${component_dir}/tests"/*.yaml; do
  if [[ -f "${extra}" ]]; then
    if [[ "${extra}" == *"invalid"* ]]; then
      echo "==> [${component_dir}] esphome config ${extra} (expect fail)"
      if esphome config "${extra}"; then
        echo "Expected validation failure for ${extra}, but config passed" >&2
        exit 1
      fi
    else
      echo "==> [${component_dir}] esphome config ${extra}"
      esphome config "${extra}"
    fi
  fi
done

if [[ -x "${component_dir}/tests_host/run_host_tests.sh" ]]; then
  echo "==> [${component_dir}] host unit tests"
  "${component_dir}/tests_host/run_host_tests.sh"
fi

echo "==> [${component_dir}] esphome compile"
esphome compile "${validate_file}"

echo "==> [${component_dir}] PASS"
