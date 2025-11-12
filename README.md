# GpgFrontend

![Language](https://img.shields.io/badge/language-C%2B%2B-green)
![License](https://img.shields.io/badge/License-GPL--3.0-orange)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d1750e052a85430a8f1f84e58a0fceda)](https://www.codacy.com/gh/saturneric/GpgFrontend/dashboard?utm_source=github.com&utm_medium=referral&utm_content=saturneric/GpgFrontend&utm_campaign=Badge_Grade)
[![Build Release](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml)
[![Build Nightly](https://github.com/saturneric/GpgFrontend/actions/workflows/testing-nightly.yml/badge.svg?branch=develop)](https://github.com/saturneric/GpgFrontend/actions/workflows/testing-nightly.yml)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/saturneric/GpgFrontend)

A modern Enigma built on [GnuPG](https://www.gnupg.org/). It brings clarity,
security, and trust to everyday encryption.

<img width="100" height="100" align="right" style="position: absolute;right: 0;padding: 12px;top:12px;z-index: 1000;" src="https://image.cdn.bktus.com/i/2024/02/24/248b2e18-a120-692e-e6bc-42ca30be9011.webp" alt="GpgFrontend"/>

**Key Features:**

- One-click encryption and signing: Quickly encrypt, decrypt and digitally sign
  texts, files and emails
- Easy installation: Available via Homebrew, Flatpak, GitHub Releases, winget,
  AUR, Microsoft Store and more
- True cross-platform experience: Native support for Windows, macOS and Linux;
  can also be built on FreeBSD
- Fully portable: Run directly from a USB drive and move keys and settings
  between Windows and Linux without hassle
- Secure key transfer: Safely migrate your keys and configurations across
  devices
- Multiple independent key databases: Manage keys and identities separately for
  different roles, projects or teams
- Comprehensive algorithm support: Includes RSA, DSA, EdDSA, ECDSA (NIST,
  Brainpool), ECDH and other algorithms via the latest GnuPG

> If you appreciate GpgFrontend, just give it a ⭐ on GitHub: it’s like adopting
> your own little digital Enigma.

## Table of Contents

- [GpgFrontend](#gpgfrontend)
  - [Table of Contents](#table-of-contents)
  - [User Manual](#user-manual)
  - [Developer Wiki](#developer-wiki)
  - [Language Support](#language-support)
    - [Supported Languages](#supported-languages)
  - [Modules](#modules)
    - [Mission and Origins](#mission-and-origins)
  - [Contact](#contact)
  - [Contributing \& Bugs Report](#contributing--bugs-report)
    - [Quick Start with GitHub Codespaces](#quick-start-with-github-codespaces)
  - [Project Maintainer](#project-maintainer)
  - [Project's Logo](#projects-logo)
  - [LICENSES](#licenses)

## User Manual

For detailed instructions on installation, usage, and troubleshooting, please
refer to the official [User
Manual](https://www.gpgfrontend.bktus.com/overview/glance). The User Manual is
the primary and most up-to-date resource for all users. It provides
comprehensive guidance beyond what is found in the README, ensuring you have the
latest information and best practices.

The source code for the user manual is maintained in this
[repository](https://github.com/saturneric/GpgFrontend-Manual.git).

## Developer Wiki

[An AI-assisted Wiki](https://deepwiki.com/saturneric/GpgFrontend) has been
generated through automated analysis of the GpgFrontend source code. This Wiki
offers a comprehensive overview of the project’s technical architecture, design
principles, and major components. It serves as a valuable resource for developers
looking to understand the inner workings of GpgFrontend.

## Language Support

If you find an error in any of the translations or need to add a new one, we
welcome you to [join our translation
work](https://www.gpgfrontend.bktus.com/appendix/translate-interface).

### Supported Languages

GpgFrontend currently supports an array of languages including:

- English
- Chinese
- French
- German
- Italian

Contributors: [SHOW](TRANSLATORS)

## Modules

GpgFrontend supports extensive module development, allowing users to customize
their experience. Modules can encapsulate functionality, enabling users to
enable or disable features as needed. Users can refer to existing module code
for guidance [Module
Repository](https://github.com/saturneric/GpgFrontend-Modules.git) to reach a
broader audience.

### Mission and Origins

GpgFrontend draws its inspiration and core philosophy from the stable, portable,
and cross-platform **[gpg4usb](https://git.bktus.com/gpgfrontend/gpg4usb/)**
project. Inheriting the spirit of true portability and platform independence,
GpgFrontend builds on this foundation to deliver advanced features, modern
cryptographic standards, and a clean, intuitive user experience.

As described in the article _[“The Past and Present of GpgFrontend: My Journey
with an Open-Source Encryption
Tool”](https://blog.bktus.com/en/archives/u8hywl/)_, the project began with a
simple question:

> “What if everyone could have a small, reliable, and secure ‘crypto machine’—a
> tool that makes encryption as tangible and trustworthy as turning a key in a
> lock?”

That idea became the cornerstone of GpgFrontend’s mission: **to make OpenPGP
encryption truly accessible—simple, robust, and convenient for everyone, on any
major operating system.**

## Contact

Please refer to [HERE](https://www.gpgfrontend.bktus.com/overview/contact) for
my contact details.

## Contributing & Bugs Report

Feel free to dive in! [Open an
issue](https://github.com/saturneric/GpgFrontend/issues/new) or submit PRs if
you prefer to use GitHub. For anonymous users, Git patches can be delivered by
[mail](mailto:eric@bktus.com). If you don't have a GitHub account or prefer not
to register, you are welcome to communicate with me via email.

[Contributing Guide](https://www.gpgfrontend.bktus.com/appendix/contribute)

### Quick Start with GitHub Codespaces

[![Open in GitHub
Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/saturneric/GpgFrontend)

With just one click, you can quickly launch a fully configured development
environment in GitHub Codespaces.

## Project Maintainer

[@Saturneric](https://github.com/saturneric)

I’m always eager to hear your suggestions for improvement. My goal is to
continuously enhance the usability of GpgFrontend and address critical issues
effectively. Your feedback is highly valued and has a great chance of being
implemented in the next release. Feel free to share your ideas and insights via
Issues, email, or any other convenient method. Together, we can make GpgFrontend
even better!

## Project's Logo

<img width="256" height="256" src="https://image.cdn.bktus.com/i/2024/02/24/f3f2f26a-96b4-65eb-960f-7ac3397a0a40.webp" alt="Logo"/>

## LICENSES

GpgFrontend itself is licensed under the [GPLv3](COPYING).

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_large)
