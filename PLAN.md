# Migration plan: `lilygo_t5_47_plus`

## Decisions

- The component and its repository use the name `lilygo_t5_47_plus`.
- The target GitHub repository is `hbast/lilygo_t5_47_plus`.
- The repository contains only the current component. The old `t547` component
  and all legacy examples are removed.
- The deleted PlatformIO board repository is not recreated or referenced.
- The ESPHome upstream pull request is closed; the project is maintained as an
  external component.
- Development, validation and compilation run in Docker. No Python, PlatformIO
  or compiler toolchain is installed on the host.

## Phase 1: Containerized development environment

Status: complete.

- Pin the default development image to the ESPHome `2026.6` release line.
- Store PlatformIO and ESPHome build caches in named Docker volumes.
- Provide one wrapper for image build, config validation, compilation, cleanup
  and an interactive container shell.
- Document version overrides so the component can be checked against more than
  one ESPHome release without sharing incompatible caches.

Acceptance:

- `./scripts/dev build` succeeds.
- `./scripts/dev version` runs ESPHome inside Docker.
- No compiler, Python environment or PlatformIO installation is created on the
  host.

## Phase 2: Establish the canonical repository

Status: complete.

1. Populate the local workspace from the current public repository history.
2. Rename the GitHub repository to `hbast/lilygo_t5_47_plus`.
3. Set the local `origin` to the renamed repository.
4. Create a migration branch before changing component contents.
5. Preserve attribution and license history, but remove operational links to
   `dabalroman`, `hbast/esphome` and the deleted PlatformIO repository.

Acceptance:

- The local checkout has the expected history and a clean baseline.
- GitHub and local `origin` both use `hbast/lilygo_t5_47_plus`.
- No install instructions point to another component repository.

## Phase 3: Replace the old implementation

Status: complete; hardware behavior remains to be smoke-tested in Phase 5.

1. Remove `components/t547` and the existing root-level YAML examples.
2. Import the current `lilygo_t5_47_plus` implementation from the ESPHome fork.
3. Keep display, touchscreen and battery support in
   `components/lilygo_t5_47_plus`.
4. Review the imported source for assumptions introduced by its former location
   inside the full ESPHome tree.
5. Correct known inconsistencies, including the battery pin documentation and
   the reported black/white inversion.

Acceptance:

- `components/t547` no longer exists.
- Only `platform: lilygo_t5_47_plus` is documented and tested.
- The component can be loaded as an ESPHome external component from this repo.

## Phase 4: One self-contained package and consistent examples

Status: complete.

1. Add `packages/lilygo_t5_47_plus.yaml` containing:
   - ESP32-S3 and Arduino framework selection;
   - 16 MB flash configuration;
   - octal PSRAM at 80 MHz;
   - the fixed GT911 I2C pins;
   - the external-component reference to `hbast/lilygo_t5_47_plus`.
2. Add focused examples for minimal display, greyscale, touchscreen, battery and
   deep sleep.
3. Keep credentials in `secrets.yaml` references or harmless placeholders.
4. Make the package the only recommended installation path in the README.

Acceptance:

- A user needs only one GitHub repository reference.
- Every example derives its board settings from the same package.
- No YAML contains 4 MB flash settings, copied SDK configuration blocks or
  references to removed repositories.

## Phase 5: Docker-only verification

Status: complete for V2.4.

1. Validate the local minimal, full-coverage and deep-sleep test configurations
   with `esphome config` in Docker.
2. Compile minimal, full-coverage and deep-sleep firmware in Docker.
3. Run the matrix against the minimum supported ESPHome release line and the
   current pinned release line, using separate Docker caches.
4. Add repository checks for stale links, old component names, inconsistent
   flash sizes and missing referenced files.
5. Document hardware smoke tests for V2.4 and the verified subset for V2.3.

Acceptance:

- All configuration and compile checks pass without host tooling.
- The generated firmware is ready for hardware validation.
- Display polarity, touch, battery GPIO14 and deep-sleep shutdown have explicit
  hardware test results before the first release.

Hardware results (V2.4):

- white paper background and black logical pixels confirmed;
- all 16 grayscale levels rendered;
- touch markers appeared at the correct transformed coordinates;
- no-battery state reported 0%; a connected 600 mAh LiPo read 4.20 V / 100%
  (see README's troubleshooting note — a reversed-polarity JST-PH connection
  reads identically to no battery at all, which cost real debugging time
  before the actual cause was found);
- timed deep sleep retained the E-Paper image and woke for the next refresh.

## Phase 6: Public cleanup and release

Status: complete.

1. Close ESPHome pull request `esphome/esphome#14403` with a short note that the
   component will continue as an external component.
2. Correct the stale feature-request instructions and link to the new package.
3. Remove or correct the older comments that reference the deleted
   `board_config.yaml` in `hbast/esphome`.
4. Create a tagged release after Docker and hardware verification.
5. Recommend the release tag, rather than `main`, in the public quick start.

Acceptance:

- The upstream PR no longer suggests that upstream integration is active.
- The feature request contains one current, tested installation example.
- The first release is reproducible from the Docker environment.

Completed:

- upstream pull request `esphome/esphome#14403` was closed;
- obsolete installation instructions in the feature request were replaced with
  the single-repository package example;
- hardware-verified release `v1.0.0` was published and its public package and
  examples were validated from GitHub.
