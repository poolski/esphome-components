#!/usr/bin/env bash
set -euo pipefail

clang++ -std=c++17 -Wall -Wextra -pedantic \
  components/ld2451/tests_host/config_state_test.cpp \
  components/ld2451/config_state.cpp \
  -Icomponents/ld2451 -o /tmp/ld2451-config-state-test
/tmp/ld2451-config-state-test

clang++ -std=c++17 -Wall -Wextra -pedantic \
  components/ld2451/tests_host/frame_parser_test.cpp \
  components/ld2451/frame_parser.cpp \
  -Icomponents/ld2451 -o /tmp/ld2451-frame-parser-test
/tmp/ld2451-frame-parser-test

clang++ -std=c++17 -Wall -Wextra -pedantic \
  components/ld2451/tests_host/ack_codec_test.cpp \
  components/ld2451/ack_codec.cpp \
  -Icomponents/ld2451 -o /tmp/ld2451-ack-codec-test
/tmp/ld2451-ack-codec-test
