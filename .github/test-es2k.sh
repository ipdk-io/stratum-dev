xargs -a .github/es2k-tests.txt \
    bazel test --define target=es2k --test_tag_filters=-broken,-flaky
