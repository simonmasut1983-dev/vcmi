# VCMI Dependencies

This repository contains prebuilt Conan dependencies for vcmi. These dependencies are primary used by Github CI and can be used to avoid building dependencies locally by developers.

Current flow to update dependencies:
1. Open PR with changes
2. Make sure that CI build succeeds
3. Merge the PR
4. Run workflow to create new release from the CI run
5. (TBD) Update dependencies submodule and prebuilts URL in VCMI repo to point to the new commit / release, update VCMI code if needed

# TODO List

- Find a way to set up an artifactory on our server or find somebody willing to host it for us and deprecate this repository. Potential options:
  - Artifactory Community Edition (requires more high-spec server than our current one)
  - https://docs.conan.io/2/reference/conan_server.html (not recommended by Conan)
  - https://jfrog.com/

- Run CI with full package rebuild on schedule (weekly? monthly?) to detect any regressions or breaking changes in CI or in used recipes
