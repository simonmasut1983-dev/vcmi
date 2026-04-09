#!/bin/sh

echo DEVELOPER_DIR=/Applications/Xcode_16.2.app >> $GITHUB_ENV

# see GitHub Actions public runners
echo "CI_CMAKE_VERSION=4.2.3" >> $GITHUB_ENV
