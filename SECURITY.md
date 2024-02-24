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
| 2.1.x   | :x:                |
| 2.2.x   | :white_check_mark: |

## Platform & Library Support

In light of security considerations and the limited time available for
contribution, GpgFrontend will gradually cease to support certain older
platforms. Furthermore, GpgFrontend aims to utilize newer and more secure
libraries whenever possible.

As of version v1.0.0, we have discontinued support for x32 operating systems due
to time constraints and the declining usage of x32 machines in the PC market.
Therefore, we plan to stop addressing complex issues associated with this
system. For most users, x64 should suffice, or alternatively, gpg4usb can be
used.

As of version v2.0.1, we have ceased to support Ubuntu 16.04, as the LTS life
cycle for this version had ended by that time. Continuing to use this version of
Ubuntu is therefore not safe.

From version v2.1.0 onwards, we no longer support macOS 10.15, due to its
impending discontinuation by GitHub Action. Considering its age and the
likelihood of Apple discontinuing support soon, it seems prudent to drop it.

As of version v2.1.1, we have stopped supporting Ubuntu 18.04, as it is no
longer supported by GitHub Action since April 3rd. Consequently, all builds will
transition to Qt6, since Ubuntu 20.04 is compatible with Qt6.

Starting from version v2.1.2, access to the Qt5 API in the source code will be
controlled through macro definitions, allowing for a conditional compilation of
Qt5 code. This change indicates that while the project will focus on utilizing
the Qt6 API, it will still provide a means to support Qt5 compilation,
especially on the Windows platform where a special Qt5 build will be maintained.
However, the Linux and macOS platforms will primarily move forward with Qt6,
with ongoing consideration to discontinue the deb package build.

## Reporting a Vulnerability

In case you wish to report a vulnerability, please avoid raising an issue
publicly. Instead, contact me directly via email at eric@bktus.com. In your
email, please describe the vulnerability you've discovered and request a hot-fix
if necessary. Alternatively, you could create a git patch to resolve the issue
and send it to me via email. This approach would expedite the resolution of any
vulnerabilities.
