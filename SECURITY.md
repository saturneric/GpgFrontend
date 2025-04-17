# Security Policy

## Supported Versions

Only versions with a white check mark are currently being supported. Versions
without it will no longer receive updates, including security fixes. This is
primarily due to my existing commitments to work and learning, leaving me with
limited time to dedicate to this project. I sincerely apologize for any
inconvenience this may cause. The situation may improve in the future. Hence,
it's recommended to always use the latest version of the application.

| Version | Supported          |
| ------- | ------------------ |
| 1.x.x   | :x:                |
| 2.x.x   | :x:                |
| 2.1.x   | :white_check_mark: |

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

## Reporting a Vulnerability

In case you wish to report a vulnerability, please avoid raising an issue
publicly. Instead, contact me directly via email at eric@bktus.com. In your
email, please describe the vulnerability you've discovered and request a hot-fix
if necessary. Alternatively, you could create a git patch to resolve the issue
and send it to me via email. This approach would expedite the resolution of any
vulnerabilities.
