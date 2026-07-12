# GpgFrontend

![Languages](https://img.shields.io/badge/languages-C%2B%2B%20%7C%20Rust-blue)
![License](https://img.shields.io/badge/License-GPL--3.0-orange)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d1750e052a85430a8f1f84e58a0fceda)](https://www.codacy.com/gh/saturneric/GpgFrontend/dashboard?utm_source=github.com&utm_medium=referral&utm_content=saturneric/GpgFrontend&utm_campaign=Badge_Grade)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/saturneric/GpgFrontend)

A modern, cross-platform OpenPGP tool with a unique dual-engine core: switch
freely between the battle-tested **GnuPG** and a modern, memory-safe Rust
**rPGP** backend. It makes encryption, signing, and key management easier and
more trustworthy in everyday privacy workflows.

**Key Features**

- Easy: Encrypt and sign your texts, files and more in just a few clicks.
- Cross-platform: Native support for Windows, macOS and Linux.
- Portable: Run directly from a USB drive and carry your keys.
- Flexible: Manage keys separately through multiple isolated key databases.
- Dual-engine: Switch freely between the mature GnuPG and the Rust rPGP backend.
- Future-proof: Explore OpenPGP v6 (RFC 9580) and post-quantum algorithms.
- Memory-safe: Sensitive data is guarded in secure memory while in use.
- Privacy-focused: Everything runs locally, with no telemetry or tracking.

> If you like GpgFrontend, you can give it a ⭐ on GitHub as donation. :)

## Table of Contents

- [GpgFrontend](#gpgfrontend)
  - [Table of Contents](#table-of-contents)
  - [User Manual](#user-manual)
  - [Release Channels](#release-channels)
  - [Supported Languages](#supported-languages)
  - [Modules](#modules)
  - [Contributing \& Bug Reports](#contributing--bug-reports)
    - [For Developers](#for-developers)
  - [Project's Logo](#projects-logo)
  - [Mission and Origins](#mission-and-origins)
  - [Project Maintainer](#project-maintainer)
  - [LICENSES](#licenses)

## User Manual

For detailed instructions on installation, usage, and troubleshooting, please
refer to the [User Manual](https://www.gpgfrontend.bktus.com/overview/glance).
The User Manual is the primary and most up-to-date resource for all users who
want to use GpgFrontend. It provides guidance on basic concepts, common
workflows, and recommended practices for using GpgFrontend effectively.

> The source code for the user manual is maintained in this
> [repository](https://github.com/saturneric/GpgFrontend-Manual.git).

## Release Channels

- **v2.2.x (Stable, recommended):** The current stable series and the
  recommended choice for daily use. It introduces the dual-engine core, adding
  the Rust **rPGP** backend for OpenPGP v6 (RFC 9580) and post-quantum
  algorithms alongside **GnuPG**, which remains the default and most
  interoperable backend. rPGP-related features continue to evolve over time.

- **v2.1.x (Legacy):** The previous GnuPG-only stable series. It remains in
  maintenance mode, focusing on bug fixes and security updates, with no new
  major features or breaking changes planned. Choose this if you need a pure
  GnuPG build or are not ready to adopt the multi-engine 2.2 line.

## Supported Languages

GpgFrontend currently supports an array of languages including:

- English
- Chinese (Simplified)
- Chinese (Traditional)
- French
- German
- Italian
- Spanish
- Russian

And these translations are contributed by our community: [SHOW](TRANSLATORS)

If you find an error in any of the translations or need to add a new one, we
welcome you to [join our translation
work](https://www.gpgfrontend.bktus.com/appendix/translate-interface).

## Modules

GpgFrontend supports extensive module development, allowing users to customize
their experience and enable only the features they need. Modules can encapsulate
functionality, enabling users to enable or disable features as needed. Users can
refer to existing module code for guidance [Module
Repository](https://github.com/saturneric/GpgFrontend-Modules.git) to reach a
broader audience.

## Contributing & Bug Reports

Contributions, bug reports, and suggestions are welcome. You can [open an
issue](https://github.com/saturneric/GpgFrontend/issues/new) or submit PRs if
you prefer to use GitHub. For anonymous users, Git patches can be delivered by
[mail](mailto:eric@bktus.com). If you don't have a GitHub account or prefer not
to register, you are welcome to communicate with me via email.

[Contributing Guide](https://www.gpgfrontend.bktus.com/appendix/contribute)

### For Developers

Although GpgFrontend's UI looks simple, its architecture is not trivial,
especially for developers unfamiliar with C++, Qt, asynchronous workflows, and
multithreading. An [AI-assisted
Wiki](https://deepwiki.com/saturneric/GpgFrontend) is available based on
source-code analysis. It can be helpful for navigation and high-level
understanding, but it may not be fully accurate in all implementation details.
Please use it together with the source code rather than as a replacement for it.

For setting up the development environment, please refer to the [Development
Environment Setup Guide](https://gpgfrontend.bktus.com/appendix/setup-dev-env).

## Project's Logo

<img width="256" height="256" src="https://image.cdn.bktus.com/i/2024/02/24/f3f2f26a-96b4-65eb-960f-7ac3397a0a40.webp" alt="Logo"/>

## Mission and Origins

GpgFrontend inherits the codebase from the discontinued but easy-to-use
**[gpg4usb](https://git.bktus.com/gpgfrontend/gpg4usb/)**. As described in my
blog post _["The Past and Present of GpgFrontend: My Journey with an Open-Source
Encryption Tool"](https://blog.bktus.com/en/archives/u8hywl/)_, the project
began with a simple question:

> "What if everyone could have a small, reliable, and secure 'crypto machine'. A
> tool that makes encryption as tangible and trustworthy as turning a key in a
> lock?"

## Project Maintainer

[@Saturneric](https://github.com/saturneric)

You can refer to [HERE](https://www.gpgfrontend.bktus.com/overview/contact) for
my contact details.

## LICENSES

GpgFrontend itself is licensed under the [GPLv3](COPYING).

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_large)
