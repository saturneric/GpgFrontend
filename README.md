<img width="100" height="100" align="right" src="https://github.com/saturneric/Blob/blob/master/logos/icon.png?raw=true" alt="ICON"/>

# GpgFrontend

![Language](https://img.shields.io/badge/language-C%2B%2B-green)
![License](https://img.shields.io/badge/License-GPL--3.0-orange)
![CodeSize](https://img.shields.io/github/languages/code-size/saturneric/GpgFrontend)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d1750e052a85430a8f1f84e58a0fceda)](https://www.codacy.com/gh/saturneric/GpgFrontend/dashboard?utm_source=github.com&utm_medium=referral&utm_content=saturneric/GpgFrontend&utm_campaign=Badge_Grade)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/saturneric/gpgfrontend)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=small)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_small)
[![Build & Package](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml)

GpgFrontend is a Free, Open Source, Powerful, Easy-to-Use, Compact, Cross-Platform [OpenPGP](https://www.openpgp.org/)
Crypt Tool.

By using GpgFrontend, you can quickly encrypt and decrypt text or files. Or at the same time as the above operations,
you can add your own signature to let others know that this document or this paragraph of text was issued by you.
Please **take the initiative to protect privacy in your own hands**, instead of relying on servers and algorithms
provided by large companies. Please use this tool to transmit or store information that you regard as very precious. You
can also use it to securely guarantee the authenticity of your information.

GpgFrontend [supports languages](#languages-support) that are widely used in most countries and regions around the
world, including English, Chinese, French, Russian, German, Spanish, Portuguese, Arabic, etc.

**Notice:** GpgFrontend does not provide an embedded [gnupg](https://gnupg.org/) (gpg) binary and needs to be installed
by the user. **This is to ensure safety and avoid code or binary files involved in encryption and decryption being
implanted in the back-door during the delivery process.**

If you find this tool useful and promising, welcome to encourage me through STAR this project. Thanks!

[>> Quick Start <<](#quick-start) |
[>> Code & Binary Security <<](https://gpgfrontend.pub/#/about/code-binary-verify)

<div align="center">
<img src="https://github.com/saturneric/Blob/blob/master/screenshots/main-ubuntu.png?raw=true" alt="Ubuntu Main Screenshot"/>
</div>
---

## Table of Contents

- [Usage](#usage)
    - [Quick Start](#quick-start)
- [Document](#document)
- [Build](#build)
- [Languages Support](#languages-support)
- [Contributing & Bugs Report](#contributing--bugs-report)
    - [Contract](#contract)
    - [Maintainers](#maintainers)
- [Licenses](#LICENSES)

## Usage

### Quick Start

Here are some common usages to help you understand what GpgFrontend does and where it comes in handy. The interface presented may not be exactly the same as the latest stable release.

#### Fast Text Encryption

Encryption can be done in just a few clicks.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encrypt-sign.gif?raw=true)

#### Fast Text Decryption

I want to see what you wrote right away.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt-verify.gif?raw=true)

#### Fast File Encryption & Sign

What about files?

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encr-sign-file.gif?raw=true)

## Document

GpgFrontend provides detailed documentation on his main features. If you want to know how to install with it, please read the [Quick Start](https://www.gpgfrontend.pub/#/quick-start) instead of README. 

## Build

The tutorial for building the software will be released soon. Before the relevant documents are released, you can refer to the project-related Github Action file if you know it.

## Languages Support

The supported languages are listed here. Some languages use machine translation and have not been verified. If you want to join translation and verification work, please refer to
the [here](https://gpgfrontend.pub/#/translate-interface).

### Supported Languages

'zh_CN', 'es_ES', 'zh_TW', 'zh_HK', 'fr_FR', 'de_DE', 'pl_PL', 'ru_RU', 'ja_JP', 'it_IT',
'ko_KR', 'pt_BR', 'ar_SA', 'ar_IQ', 'hi_IN', 'af_ZA', 'sq_AL', 'be_BY', 'bg_BG', 'ca_ES',
'hr_HR', 'cs_CZ', 'da_DK', 'nl_NL', 'et_EE', 'fa_IR', 'fi_FI', 'fr_CA', 'he_IL', 'id_ID',
'lt_LT', 'De_AT', 'De_CH', 'El_GR', 'Es_MX', 'Iw_IL', 'UK_UA'

Notice: The verification may not be complete and rigorous.

## Contract

Please refer to [here](https://www.gpgfrontend.pub/#/contract) for my contact details.

### Contributing & Bugs Report

Feel free to dive in! [Open an issue](https://github.com/saturneric/GpgFrontend/issues/new) or submit PRs.

### Project Maintainer

[@Saturneric](https://github.com/saturneric).

## LOGO

![logo](https://github.com/saturneric/Blob/blob/master/logos/gpgfrontend-logo.jpg?raw=true)

## LICENSES

GpgFrontend itself is licensed under the [GPLv3](COPYING).

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_large)

### Dependency

There are some libraries and binary included in the zip-file which (may) have different licenses, for more information check their homepages. You can also obtain the sources from there.

gnupg: https://gnupg.org

gpgme: https://gnupg.org/software/gpgme/index.html

gpg4usb: https://www.gpg4usb.org

QT 5.15(opensource): https://www.qt.io

QT 5.12(opensource): https://www.qt.io

MSYS2: https://www.msys2.org

mingw-w64: http://mingw-w64.org/doku.php

AppImage: https://appimage.org

JSON for Modern C++: https://github.com/nlohmann/json

Application
Bundles: [Link](https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html#//apple_ref/doc/uid/10000123i-CH101-SW1)

The icons of this software use materials from [Alibaba Iconfont](!https://www.iconfont.cn/). The Alibaba vector icon library is free to use. The icons in the free library aren't registered as trademarks. There is no copyright issue involved and can be used commercially.
