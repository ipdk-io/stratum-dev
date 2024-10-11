// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// This is the main entry for p4 module tests.
#include <stdlib.h>

#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "stratum/glue/init_google.h"
#include "stratum/glue/logging.h"

static char kDefaultTmpDir[] = "/tmp/stratum_hal_lib_p4_test.XXXXXX";

DEFINE_string(test_tmpdir, "", "Temp directory to be used for tests.");

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  InitGoogle(argv[0], &argc, &argv, true);
  ::stratum::InitStratumLogging();

  bool tmpdir_created = false;
  if (FLAGS_test_tmpdir.empty()) {
    CHECK(mkdtemp(kDefaultTmpDir));
    FLAGS_test_tmpdir = kDefaultTmpDir;
    tmpdir_created = true;
    LOG(INFO) << "Created FLAGS_test_tmpdir " << FLAGS_test_tmpdir;
  }

  int result = RUN_ALL_TESTS();

  if (tmpdir_created) {
    const std::string cleanup("rm -rf " + FLAGS_test_tmpdir);
    system(cleanup.c_str());
    LOG(INFO) << "Cleaned up FLAGS_test_tmpdir " << FLAGS_test_tmpdir;
  }

  return result;
}
