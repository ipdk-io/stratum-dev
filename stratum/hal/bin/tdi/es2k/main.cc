// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/bin/tdi/es2k/es2k_main.h"

int main(int argc, char* argv[]) {
  return stratum::hal::tdi::Es2kMain(argc, argv).error_code();
}

