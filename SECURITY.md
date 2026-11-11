# Security Policy

## Supported Versions

Only versions with a white check mark are currently being supported. Versions
without it will no longer receive updates, including security fixes. This is
primarily due to my existing commitments to work and learning, leaving me with
limited time to dedicate to this project. I sincerely apologize for any
inconvenience this may cause. The situation may improve in the future. Hence,
it's recommended to always use the latest version of the application.

The table below is generated from `release/policy.toml`; edit that file and
run `scripts/release/policy.py render-security`, never the table itself.

<!-- policy:versions:begin -->
| Version | Status | Supported | Until |
| ------- | ------ | --------- | ----- |
| 2.3.x | In development: nightly builds | :white_check_mark: |  |
| 2.2.x | Current release: bug and security fixes | :white_check_mark: |  |
| 2.1.x | Security fixes only (the last series that builds with Qt5); move to 2.2.x | :white_check_mark: | 2027-12-31 |
| 2.0.x | End of life; move to 2.2.x | :x: |  |
| 1.x.x | End of life; move to 2.2.x | :x: |  |
<!-- policy:versions:end -->

## Verifying Downloads

Which key signs which releases (generated from `release/policy.toml` and
`release/product.json`):

<!-- policy:keys:begin -->
Releases from 2.1.5 on can be checked file by file:

| Releases | Signed with | Verify with |
| -------- | ----------- | ----------- |
| Stable (vX.Y.Z) | OpenPGP, detached `.sig` per file, by the maintainer | `12F7E8858CF15BEC9975FF3C5CA3DA246843FD03` |
| Nightly (main) and beta (release/X.Y) | Sigstore, on the build's manifest (`build/SHA256SUMS.txt.sigstore.json`) | the build workflow's identity, `https://github.com/saturneric/GpgFrontend/.github/workflows/build.yml@<ref>` |

Check a stable download by hand with `gpg --verify FILE.sig FILE`, a nightly's or beta's manifest with `cosign verify-blob` and its files against that manifest. The application relies on the signed update index, which names each release's descriptor by its hash, and the descriptor each file's; a stable release is listed there only once its OpenPGP signatures have been checked. Windows installers of stable releases are also Authenticode-signed.
<!-- policy:keys:end -->

## Platform & Library Support

In light of security considerations and limited time available for contribution,
GpgFrontend will gradually cease to support certain older platforms.
Furthermore, GpgFrontend will aim to utilize newer and more secure libraries
whenever possible.

As of version v1.0.0, we no longer support x32 operating systems. This decision
stems from time constraints and the dwindling usage of x32 machines in the PC
market. Consequently, I plan to cease dealing with complex issues associated
with this system. x64 should suffice for most users, or alternatively, gpg4usb
can be used.

As of version v2.0.1, we no longer support Ubuntu 16.04, as the LTS life cycle
of this version had ended by then. Continuing to use this version of Ubuntu is
therefore not safe.

From version v2.1.0 onwards, we no longer support macOS 10.15, due to its
impending discontinuation by GitHub Action. Considering its age, and the
likelihood of Apple discontinuing support soon, it seems prudent to drop it.

As of version v2.1.1, we no longer support Ubuntu 18.04, as it isn't supported
by GitHub Action since April 3rd. Thus, all builds will transition to Qt6, since
Ubuntu 20.04 is compatible with Qt6.

From version v2.1.4 onwards, we no longer support macOS 11, as it will be
discontinued soon by GitHub Action. Given its age and the anticipated
discontinuation of support by Apple, it is wise to drop it.

From version v2.1.6 onwards, we no longer support macOS 12, as it will be
discontinued soon by GitHub Action. Given its age and the anticipated
discontinuation of support by Apple, it is wise to drop it.

From version v2.1.8 onwards, we no longer support Ubuntu 20.04, as the LTS life
cycle of this version had ended by then. Continuing to use this version of
Ubuntu is therefore not safe.

From version v2.1.11 onwards, we no longer support macOS 13, as it discontinued
by GitHub Action. Given its age and the anticipated discontinuation of support
by Apple, it is wise to drop it.

From version v2.2.2 onwards, we no longer support macOS 14, as it will be soon
discontinued by GitHub Action. Given its age and the anticipated discontinuation
of support by Apple, it is wise to drop it.

From version v2.2.3 onwards, we no longer support Qt5, as it consumes more time to maintain code for both Qt5 and Qt6, especially with the increasing adoption of Qt6. But version 2.1.x will still support Qt5, so if you want to use Qt5, please use version 2.1.x.

## Reporting a Vulnerability

In case you wish to report a vulnerability, please avoid raising an issue
publicly. Instead, contact me directly via email at <eric@bktus.com>. In your
email, please describe the vulnerability you've discovered and request a hot-fix
if necessary. Alternatively, you could create a git patch to resolve the issue
and send it to me via email. This approach would expedite the resolution of any
vulnerabilities.
