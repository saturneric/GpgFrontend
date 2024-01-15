# Overview of GpgFrontend

---

![Language](https://img.shields.io/badge/language-C%2B%2B-green)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/saturneric/gpgfrontend)
![License](https://img.shields.io/badge/License-GPL--3.0-orange)
![CodeSize](https://img.shields.io/github/languages/code-size/saturneric/GpgFrontend)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d1750e052a85430a8f1f84e58a0fceda)](https://www.codacy.com/gh/saturneric/GpgFrontend/dashboard?utm_source=github.com&utm_medium=referral&utm_content=saturneric/GpgFrontend&utm_campaign=Badge_Grade)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_shield)

**GpgFrontend** is a Powerful, Easy-to-Use, Compact, Cross-Platform, and
Installation-Free [OpenPGP](https://www.openpgp.org/) Crypto Tool.

By using GpgFrontend, you can quickly encrypt and decrypt text or files. Or at
the same time as the above operations, you can add your own signature to let
others know that this document or this paragraph of text was issued by you.

Furthermore, it visualizes most of the common operations of gpg commands. It
aims to allow ordinary users to quickly use gpg and make professional users more
convenient. GpgFrontend supports new features of GnuPG 2.x.

**The corresponding release version for this
Document: [v2.0.4](https://github.com/saturneric/GpgFrontend/releases/tag/v2.0.4)**

## Interface

Note: For different operating system and system style settings, GpgFrontend may
look different.Documentation can also vary widely from release to release.
Please check the corresponding program release version for the document.

![image-20220109192100901](https://www.bktus.com/wp-content/uploads/2023/08/image-20220109192100901.png)

## System Requirement

Before proceeding with the installation and usage of GpgFrontend, it's crucial
to understand the system requirements that ensure optimal performance. This
section provides comprehensive details about the necessary software
dependencies, hardware specifications, and the compatible operating systems.
Meeting these requirements will ensure a smooth, efficient experience while
using GpgFrontend.

Please read the following subsections carefully to confirm that your system
aligns with the recommended configurations.

### Operating System

GpgFrontend is compatible with major operating systems including Linux, macOS,
and Windows. Specifically, it recommends Windows 10 and later, macOS 11 and
later, and Ubuntu 20.04 LTS or other equivalent Linux distributions.

### Software Dependencies

Qt6 or Later: GpgFrontend is developed using the Qt framework. Therefore, a
runtime of Qt6 or later is required for the software to function correctly.
Please note, the Qt6 runtime is integrated into the Release Package for Linux,
macOS, and Windows, so it generally does not need to be considered separately.
This allows for easier setup and ensures compatibility across different systems.

GnuPG: As the frontend for GnuPG, GpgFrontend requires GnuPG (version 2.2.0 or
higher) to be pre-installed in your system. Please note, GpgFrontend is not
compatible with GnuPG 1.x versions. Users employing earlier versions of GnuPG 2
may encounter some unforeseen issues. We strongly recommend updating to the
supported versions to ensure a seamless experience.

### Hardware

While the specific hardware requirements largely depend on the size and
complexity of the data you're working with, we generally recommend:

A computer with at least 1 GB of RAM. However, 2 GB or more is preferable for
smoother performance. The majority of these resources are allocated to your
operating system, but around 100 MB of memory is needed to ensure the smooth
running of GpgFrontend. At least 200 MB of free disk space for software
installation. Additional space will be needed for ongoing work.

Please note, these requirements are intended to be guidelines rather than strict
rules. It's possible that GpgFrontend will work on lower-spec hardware, but for
optimal performance, the above specifications are recommended.

### Network

Although not necessary for basic operation, an active Internet connection may be
required for software updates and accessing online help resources.

Please note that these are the minimal requirements that we tested, and actual
requirements for your use case could be higher, especially for large datasets.

## Origin

The GpgFrontend project inherit from a relatively mature but not maintained
[gpg4usb](https://www.gpg4usb.org/) project. It inherits the stable,
easy-to-use, compact, and installation-free features of gpg4usb, and plans to
support some new features of OpenPGP based on it and continue to improve it.

## Purpose

GpgFrontend is committed to empowering people globally, especially those without
command-line or programming expertise, to securely transmit information. While
free software represents a shared asset for all of humanity, many individuals
remain unable to leverage it due to limited knowledge or ingrained usage habits.
Our mission is to break this cycle.

We are dedicated to improving the user-friendliness and intuitive nature of the
free software GnuPG, with the primary objective of expanding its reach to a
wider audience.

As an open-source project, GpgFrontend not only advocates for transparency and
community participation, but also adheres to using open-source codes and
libraries.

### Free forever

GpgFrontend is committed to remaining free of charge indefinitely. You can rest
assured that you will never be asked to pay a fee to use the software.

## Source Code

The primary code repository for GpgFrontend resides on the GitHub server. Every
update gets committed here first. You can visit the [code
repository](https://github.com/saturneric/GpgFrontend) to
follow our development progress and stay updated on the latest changes.

### License

GpgFrontend's source code is licensed under the GPL-3.0 license, affirming its
status as open-source software. You are entitled to exercise the rights outlined
in the license, as long as you adhere to its terms and conditions.

### Contributing

We believe that adding new features shouldn't compromise existing core
functionality. However, as an individual developer, my abilities are limited.
Therefore, GpgFrontend warmly welcomes contributors. You can report issues or
submit pull requests through GitHub. Additionally, questions and code
contributions can be submitted via email. Feel free to send me your bug reports
and patches.

## Privacy Assurance

GpgFrontend operates serverlessly, it doesn't require any server to function.
Although it utilizes the OpenPGP protocol for public key transfers, no
additional information is collected or uploaded.

For users with heightened security needs, a version of GpgFrontend will be made
available in the future that lacks internet access capabilities.
