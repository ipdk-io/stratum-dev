xargs -a .github/dpdk-tests.txt \
    bazel test --define target=dpdk --test_tag_filters=-broken,-flaky
