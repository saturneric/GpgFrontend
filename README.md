# GpgFrontend

![Language](https://img.shields.io/badge/language-C%2B%2B-green)
![License](https://img.shields.io/badge/License-GPL--3.0-orange)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d1750e052a85430a8f1f84e58a0fceda)](https://www.codacy.com/gh/saturneric/GpgFrontend/dashboard?utm_source=github.com&utm_medium=referral&utm_content=saturneric/GpgFrontend&utm_campaign=Badge_Grade)
[![Build Release](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml)
[![Build Nightly](https://github.com/saturneric/GpgFrontend/actions/workflows/testing-nightly.yml/badge.svg?branch=develop)](https://github.com/saturneric/GpgFrontend/actions/workflows/testing-nightly.yml)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/saturneric/GpgFrontend)

A modern "Enigma" built on [GnuPG](https://www.gnupg.org/). It brings easy and
trustworthy to your privacy life.

<img width="100" height="100" align="right" style="position: absolute;right: 0;padding: 12px;top:12px;z-index: 1000;" src="https://image.cdn.bktus.com/i/2024/02/24/248b2e18-a120-692e-e6bc-42ca30be9011.webp" alt="GpgFrontend"/>

**Key Features**

- Easy: Encrypt and sign your texts, files and more.
- Cross-platform: Native support for Windows, macOS and Linux.
- Portable: Run directly from a USB drive and carry your keys.
- Flexible: Manage keys separately through multiple key databases.
- Privacy-focused: Using multiple approaches to enhance safety.

> If you like GpgFrontend, you can give it a ⭐ on GitHub as donation. :)

## Table of Contents

- [GpgFrontend](#gpgfrontend)
  - [Table of Contents](#table-of-contents)
  - [User Manual](#user-manual)
  - [Supported Languages](#supported-languages)
  - [Modules](#modules)
  - [Contributing \& Bugs Report](#contributing--bugs-report)
    - [For Developers](#for-developers)
  - [Project's Logo](#projects-logo)
  - [Mission and Origins](#mission-and-origins)
  - [Project Maintainer](#project-maintainer)
  - [LICENSES](#licenses)

## User Manual

For detailed instructions on installation, usage, and troubleshooting, please
refer to the [User Manual](https://www.gpgfrontend.bktus.com/overview/glance).
The User Manual is the primary and most up-to-date resource for all users. It
provides guidance beyond what is found in the README, ensuring you have the best
practices.

The source code for the user manual is maintained in this
[repository](https://github.com/saturneric/GpgFrontend-Manual.git).

## Supported Languages

GpgFrontend currently supports an array of languages including:

- English
- Chinese
- French
- German
- Italian
- Spanish

Contributors: [SHOW](TRANSLATORS)

If you find an error in any of the translations or need to add a new one, we
welcome you to [join our translation
work](https://www.gpgfrontend.bktus.com/appendix/translate-interface).

## Modules

GpgFrontend supports extensive module development, allowing users to customize
their experience. Modules can encapsulate functionality, enabling users to
enable or disable features as needed. Users can refer to existing module code
for guidance [Module
Repository](https://github.com/saturneric/GpgFrontend-Modules.git) to reach a
broader audience.

## Contributing & Bugs Report

Feel free to dive in! [Open an
issue](https://github.com/saturneric/GpgFrontend/issues/new) or submit PRs if
you prefer to use GitHub. For anonymous users, Git patches can be delivered by
[mail](mailto:eric@bktus.com). If you don't have a GitHub account or prefer not
to register, you are welcome to communicate with me via email.

[Contributing Guide](https://www.gpgfrontend.bktus.com/appendix/contribute)

### For Developers

GpgFrontend's architecture and design are not easy for beginners, especially for
developers who are not familiar with C++, Qt, and multithreading. I was
struggling hard for these in the past few years as well. [An AI-assisted
Wiki](https://deepwiki.com/saturneric/GpgFrontend) has been created through
analysis of the GpgFrontend source code. This Wiki offers a comprehensive
overview of the project’s technical architecture, design principles, and major
components. I'd checked it and I would say that it can be of great help.

For setting up the development environment, please refer to the [Development
Environment Setup Guide](https://gpgfrontend.bktus.com/appendix/setup-dev-env).

## Project's Logo

<img width="256" height="256" src="https://image.cdn.bktus.com/i/2024/02/24/f3f2f26a-96b4-65eb-960f-7ac3397a0a40.webp" alt="Logo"/>

## Mission and Origins

GpgFrontend inherits the codebase from the discontinued but easy-to-use
**[gpg4usb](https://git.bktus.com/gpgfrontend/gpg4usb/)**. As described in my
blog post _[“The Past and Present of GpgFrontend: My Journey with an Open-Source
Encryption Tool”](https://blog.bktus.com/en/archives/u8hywl/)_, the project
began with a simple question:

> “What if everyone could have a small, reliable, and secure ‘crypto machine’. A
> tool that makes encryption as tangible and trustworthy as turning a key in a
> lock?”

## Project Maintainer

[@Saturneric](https://github.com/saturneric)

You can refer to [HERE](https://www.gpgfrontend.bktus.com/overview/contact) for
my contact details.

## LICENSES

GpgFrontend itself is licensed under the [GPLv3](COPYING).

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_large)
