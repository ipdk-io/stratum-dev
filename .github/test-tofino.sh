xargs -a .github/tofino-tests.txt \
    bazel test --define target=tofino \
        --test_tag_filters=-broken,-flaky --jobs=6
