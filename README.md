<img width="100" height="100" align="right" src="https://github.com/saturneric/Blob/blob/master/logos/icon.png?raw=true" alt="ICON"/>

# GpgFrontend

![Language](https://img.shields.io/badge/language-C%2B%2B-green)
![License](https://img.shields.io/badge/License-GPL--3.0-orange)
![CodeSize](https://img.shields.io/github/languages/code-size/saturneric/GpgFrontend)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d1750e052a85430a8f1f84e58a0fceda)](https://www.codacy.com/gh/saturneric/GpgFrontend/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=saturneric/GpgFrontend&amp;utm_campaign=Badge_Grade)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/saturneric/gpgfrontend)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=small)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_small)

GpgFrontend is an Easy-to-Use, Compact, Cross-Platform, and Installation-Free [OpenPGP](https://www.openpgp.org/)
Frontend Tool.

By using GpgFrontend, you can quickly **encrypt and decrypt text or files**. Or at the same time as the above
operations, you can add **your own signature** to let others know that this document or this paragraph of text was
issued by you. It aims to allow ordinary users to quickly use gpg and make professional users more convenient.
GpgFrontend supports new features of OpenPGP.

**Notice:** GpgFrontend does not provide an embedded [gnupg](https://gnupg.org/) binary library and needs to be
installed by the user. **This is to ensure safety and avoid code or binary files being implanted in the backdoor during
the delivery process.**

[>> Quick Start](#quick-start)

[>> Code & Binary Security](https://saturneric.github.io/GpgFrontend/index.html#/about/code-binary-verify)

<div align="center">
<img width="640" src="https://github.com/saturneric/Blob/blob/master/screenshots/main_mac.jpg?raw=true" alt="macOS Screenshot"/>
</div>
<div align="center">
<img width="320" src="https://github.com/saturneric/Blob/blob/master/screenshots/key_info.PNG?raw=true" alt="Windows Screenshot"/>
<img width="320" src="https://github.com/saturneric/Blob/blob/master/screenshots/keygen_ubuntu.png?raw=true" alt="Ubuntu Screenshot"/>
</div>

#### Workflows Status:

[![Build & Package](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/saturneric/GpgFrontend/actions/workflows/cmake.yml)

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
- [Contract](#contract)
    - [Contributing](#contributing)
    - [Maintainers](#maintainers)
- [Licenses](#LICENSES)

## Features

- Can run on **Windows, Linux and macOS**.
  
- Open source, free, no need to install.
  
- For Windows user, just double-click, and then you can use it freely.
  
- For macOS and Linux user, just type few command before running it.

- Supports multiple languages. Don’t forget to help me translate this software.

## Usage

### Quick Start

### Fast encryption

Encryption can be done in just a few clicks.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encrypt.gif?raw=true)

### Fast Decryption

I want to see what you wrote right away.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt.gif?raw=true)

### More Helpful Operation

Read the animated pictures in the [Document](https://saturneric.github.io/GpgFrontend/index.html#/) to learn more
awesome operations.

### How to Run it

#### Windows

1. [Download](https://gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.3.1_20210420.exe) gnupg-w32-******.exe
2. Double Click it to install it
3. [Download](https://github.com/saturneric/GpgFrontend/releases) GpgFrontend Windows Edition
4. Unzip gpgfrontend-windows-latest-*******.zip
5. Go into the directory and double click gpgfrontend.exe

#### macOS

1. Install Homebrew [Here](https://brew.sh/) if you don't know it.
2. Install gnupg
    ```shell
    % brew update
    % brew install gnupg qt@5 gpgme
    ```
3. [Download](https://github.com/saturneric/GpgFrontend/releases) GpgFrontend macOS Edition
4. Unzip GpgFrontend
5. Get into folder and Give gpgfrontend permission to execute
    ```shell
    % cd gpgfrontend-macos-latest-*******/
    % chmod u+x gpgfrontend
    ```
6. Just run it
    ```shell
    % ./gpgfrontend
    ```

#### Debian/Ubuntu/CentOS

1. Install gnupg
    - For Debian/Ubuntu
       ```shell
       $ sudo apt update
       $ sudo apt install gpg libgpgme11 qt-default
       ```
    - For CentOS
       ```shell
       $ sudo yum install gnupg gpgme qt5-qtbase
       ```
2. [Download](https://github.com/saturneric/GpgFrontend/releases) GpgFrontend Linux Edition
3. Unzip GpgFrontend
4. Get into folder and Give gpgfrontend permission to execute
    ```shell
    $ cd gpgfrontend-ubuntu-latest-*******/
    $ chmod u+x gpgfrontend
    ```
5. Just run it
    ```shell
    $ ./gpgfrontend
    ```

## Document

For more usage information, please read [Document](https://saturneric.github.io/GpgFrontend/index.html#/).

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

The tutorial for building the software will be released shortly.

## Contract

If you want to contact me individually, you can email [eric@bktus.com](mailto:eric@bktus.com).

### Contributing

Feel free to dive in! [Open an issue](https://github.com/saturneric/GpgFrontend/issues/new) or submit PRs.

### Maintainers

[@Saturneric](https://github.com/saturneric).

## LOGO

![logo](https://github.com/saturneric/Blob/blob/master/logos/gpgfrontend-logo.jpg?raw=true)

## LICENSES

GpgFrontend itself is licensed under the [GPLv3](COPYING).

There are some libraries and binary included in the zip-file which (may) have different licenses, for more information
check their homepages. You can also obtain the sources from there.

gpg4usb: https://www.gpg4usb.org/

Gnupg: https://gnupg.org/

QT: https://www.qt.io/

MSYS2: https://www.msys2.org/

mingw-w64: http://mingw-w64.org/doku.php

The icons of this software use materials from Alibaba vector icon library. The Alibaba vector icon library is free to
use. The icons in the free library aren't registered as trademarks. There is no copyright issue involved and can be used
commercially.

