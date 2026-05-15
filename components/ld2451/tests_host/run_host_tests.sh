#!/usr/bin/env bash
set -euo pipefail

clang++ -std=c++17 -Wall -Wextra -pedantic \
  components/ld2451/tests_host/config_state_test.cpp \
  components/ld2451/runtime/config_state.cpp \
  -Icomponents/ld2451 -o /tmp/ld2451-config-state-test
/tmp/ld2451-config-state-test
