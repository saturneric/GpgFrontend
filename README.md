<img width="100" height="100" align="right" src="https://github.com/saturneric/Blob/blob/master/logos/icon.png?raw=true" alt="ICON"/>

# GpgFrontend

![Language](https://img.shields.io/badge/language-C%2B%2B-green)
![License](https://img.shields.io/badge/License-GPL--3.0-orange)
![CodeSize](https://img.shields.io/github/languages/code-size/saturneric/GpgFrontend)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d1750e052a85430a8f1f84e58a0fceda)](https://www.codacy.com/gh/saturneric/GpgFrontend/dashboard?utm_source=github.com&utm_medium=referral&utm_content=saturneric/GpgFrontend&utm_campaign=Badge_Grade)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/saturneric/gpgfrontend)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=small)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_small)

GpgFrontend is a Powerful, Easy-to-Use, Compact, Cross-Platform, and
Installation-Free [OpenPGP](https://www.openpgp.org/)
Crypto Tool.

By using GpgFrontend, you can quickly encrypt and decrypt text or files. Or at the same time as the above operations,
you can add your own signature to let others know that this document or this paragraph of text was issued by you.
Please **take the initiative to protect privacy in your own hands**, instead of relying on servers and algorithms
provided by large companies.

**Notice:** GpgFrontend does not provide an embedded [gnupg](https://gnupg.org/) binary library and needs to be
installed by the user. **This is to ensure safety and avoid code or binary files involved in encryption and decryption
being implanted in the backdoor during the delivery process.**

If you find this tool useful and promising, welcome to encourage me through STAR this project. Thanks!

[>> Quick Start <<](#quick-start) |
[>> Code & Binary Security <<](https://gpgfrontend.pub/#/about/code-binary-verify) |
[>> 简中README <<](https://github.com/saturneric/GpgFrontend/blob/main/README_CN.md)

<div align="center">
<img width="640" src="https://github.com/saturneric/Blob/blob/master/screenshots/main_mac.jpg?raw=true" alt="macOS Screenshot"/>
</div>
<div align="center">
<img width="320" src="https://github.com/saturneric/Blob/blob/master/screenshots/key_info.PNG?raw=true" alt="Windows Screenshot"/>
<img width="320" src="https://github.com/saturneric/Blob/blob/master/screenshots/keygen_ubuntu.png?raw=true" alt="Ubuntu Screenshot"/>
</div>

#### Workflows Status:

[![Build & Package](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml)

---

## Table of Contents

- [Features](#features)
- [Usage](#usage)
    - [Quick Start](#quick-start)
    - [How to Run it](#how-to-run-it)
        - [Windows](#windows)
        - [macOS](#macos)
        - [Debian/Ubuntu/CentOS](#debianubuntucentos)
- [Document](#document)
- [Purpose](#purpose)
- [Build](#build)
- [Languages Support](#languages-support)
- [Contributing & Bugs Report](#contributing--bugs-report)
    - [Contract](#contract)
    - [Maintainers](#maintainers)
- [Licenses](#LICENSES)

## Features

- Can run on **Windows, Linux and macOS**.
- Open source, free, no need to install.
- Just double-click, and then you can use it freely.
- Supports multiple [languages](#languages-support).

## Usage

### Quick Start

### Fast Text Encryption

Encryption can be done in just a few clicks.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encrypt.gif?raw=true)

### Fast Text Decryption

I want to see what you wrote right away.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt.gif?raw=true)

### Fast File Encryption & Sign

What about files?

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encr-sign-file.gif?raw=true)

### More Helpful Operation

Read the animated pictures in the [Document](https://gpgfrontend.pub/#/quick-start) to learn more awesome operations.

### How to Run it

#### Windows

0. If you haven't installed gnupg,
   please [Download](https://gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.3.1_20210420.exe) `gnupg-w32-******.exe` and
   install it.
1. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases)  Windows Edition from the latest release
2. Unzip `gpgfrontend-windows-latest-*******.zip`
3. Go into the directory and double click `GpgFrontend.exe`.

#### macOS

0. If command `gpg` is not available, please use homebrew to install it first.
1. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) macOS edition from the latest release
2. Double-Click GpgFrontend.dmg to load it
    - macOS will automatically decompress the zip file and then you will be able to see the dmg
3. Double click and run it
   (due to macOS security policy, you may need a little more step).
4. If it satisfies you, you can drag it into your Application folder.

#### Debian/Ubuntu/CentOS

[![!AppImage](https://docs.appimage.org/_images/download-appimage-banner.svg)](https://github.com/saturneric/GpgFrontend/releases/latest)

0. Install gnupg (If you have already followed please skip)
    - For Debian/Ubuntu
      ```shell
      $ sudo apt update
      $ sudo apt install gpg
      ```
    - For CentOS
       ```shell
       $ sudo yum install gnupg 
       ```
1. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) Linux edition from the latest release
2. Give `GpgFrontend-***.AppImage` permission to execute
    ```shell
    $ chmod u+x ./GpgFrontend-***.AppImage
    ```
3. Just double-click `GpgFrontend-***.AppImage` to run it.

## Document

for more usage information, please read [document](https://gpgfrontend.pub/#/overview).

## Purpose

The GpgFrontend project inherit from a relatively mature but not maintained [gpg4usb](https://www.gpg4usb.org/) project.
It inherits the stable, easy-to-use, compact, and installation-free features of gpg4usb, and plans to support some new
features of OpenPGP based on it and continue to improve it.

GpgFrontend will add more functions in the future to improve the ease of use of GPG in end-to-end transmission. At the
same time, the addition of new functions does not affect the old basic functions. My personal strength is always
limited. GpgFrontend welcomes volunteers to join. You can use the GitHub platform to file an issue, or submit a pull
request.

The GpgFrontend project is as open source, and it also insists on using open source codes and libraries.

## Build

The tutorial for building the software will be released shortly. Before the relevant documents are released, you can
refer to the project-related Github Action file if you know it.

## Languages Support

The supported languages are listed here. Some languages use machine translation and have not been verified. If you want
to join translation and verification work, please refer to
the [translate the interface](https://gpgfrontend.pub/#/translate-interface).

Notice: The verification may not be complete and rigorous.

|  Language |  English   | 简体中文  | français  | Французский  | Español  | 
|  ----  | ----  | ----  | ----  | ----  | ----  |
| Support  | Support | 支持 | Etre prêt | ожидать | apoyar |
| Verification  | Origin | 不完全 | Jamais |Никогда | Nunca |

|  Language |  日本語   | 繁體中文  | عربى  | português  |
|  ----  | ----  | ----  | ----  | ----  | 
| Support  | 未だに | 尚未 | ليس بعد | Ainda não |
| Verification  |  |  |  | |

## Contract

If you want to contact me individually, you can email [eric@bktus.com](mailto:eric@bktus.com).

### Contributing & Bugs Report

Feel free to dive in! [Open an issue](https://github.com/saturneric/GpgFrontend/issues/new) or submit PRs.

### Maintainers

[@Saturneric](https://github.com/saturneric).

## LOGO

![logo](https://github.com/saturneric/Blob/blob/master/logos/gpgfrontend-logo.jpg?raw=true)

## LICENSES

GpgFrontend itself is licensed under the [GPLv3](COPYING).

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_large)

### Dependency

There are some libraries and binary included in the zip-file which (may) have different licenses, for more information
check their homepages. You can also obtain the sources from there.

gnupg: https://gnupg.org/

gpg4usb: https://www.gpg4usb.org/

QT 5.15.2(opensource): https://www.qt.io/

MSYS2: https://www.msys2.org/

mingw-w64: http://mingw-w64.org/doku.php

AppImage: https://appimage.org/

rapidjson: https://github.com/Tencent/rapidjson

Application
Bundles: [Link](https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html#//apple_ref/doc/uid/10000123i-CH101-SW1)

The icons of this software use materials from [Alibaba Iconfont](!https://www.iconfont.cn/). The Alibaba vector icon
library is free to use. The icons in the free library aren't registered as trademarks. There is no copyright issue
involved and can be used commercially.
