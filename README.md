<img width="100" height="100" align="right" style="position: absolute;right: 0;padding: 12px;top:12px;" src="https://github.com/saturneric/Blob/blob/master/logos/icon.png?raw=true" alt="ICON"/>

# GpgFrontend

![Language](https://img.shields.io/badge/language-C%2B%2B-green)
![License](https://img.shields.io/badge/License-GPL--3.0-orange)
![CodeSize](https://img.shields.io/github/languages/code-size/saturneric/GpgFrontend)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d1750e052a85430a8f1f84e58a0fceda)](https://www.codacy.com/gh/saturneric/GpgFrontend/dashboard?utm_source=github.com&utm_medium=referral&utm_content=saturneric/GpgFrontend&utm_campaign=Badge_Grade)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/saturneric/gpgfrontend)
![Homebrew Cask](https://img.shields.io/homebrew/cask/v/gpgfrontend)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=small)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_small)
[![Build & Package](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml)

GpgFrontend is a Free, Open Source, Powerful, Easy-to-Use, Compact, Cross-Platform [OpenPGP](https://www.openpgp.org/)
Crypt Tool. Also, it's one of the excellent GUI Frontends for Modern [GnuPG](https://www.gnupg.org/) (gpg).

By using GpgFrontend,

- You can quickly make encrypted files or texts.
- You can digitally sign your texts or files.
- You can easily manage all your GPG keys on your machine.
- You can easily and safely transfer all your GPG keys between your machines.
- Furthermore, you can build and run it on Windows, macOS, Linux, FreeBSD, etc.

GpgFrontend is **PERMANENTLY FREE**, but you can also "DONATE" it through STAR this project. Thanks!

[Languages Supported](#languages-support) by GpgFrontend including English, Chinese, French, Russian, German, Spanish, Portuguese, Arabic, etc.

[>> Download <<](https://github.com/saturneric/GpgFrontend/releases/latest)
| [>> User Manual <<](https://www.gpgfrontend.pub/#/overview)
| [>> Developer Document <<](https://doxygen.gpgfrontend.pub/)
| [>> Develop Code Repo <<](https://git.codesdream.com/?p=public/main/GpgFrontend.git;a=summary)

<img src="https://github.com/saturneric/Blob/blob/master/screenshots/main-ubuntu.png?raw=true" alt="Ubuntu Main Screenshot"/>

## Table of Contents

- [Usage](#usage)
- [User Manual](#user-manual)
- [Developer Document](#developer-document)
- [Build From Source Code](#build-from-source-code)
- [Languages Support](#languages-support)
- [Contract](#contract)
- [Licenses](#LICENSES)

## Usage

Here are some common usages to help you understand what GpgFrontend does. The interface
presented here may not be exactly the same as the latest stable release.

### Text Encryption & Decryption

Encryption can be done in just a few clicks.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encrypt-sign.gif?raw=true)

Paste the text, and just click decryption.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt-verify.gif?raw=true)

### File Encryption & Sign

What about files?

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encr-sign-file.gif?raw=true)

## User Manual

GpgFrontend provides documentations on its main features. If you want to know how to install, please
read the [User Manual](https://www.gpgfrontend.pub/#/quick-start) instead of README.

## Developer Document

You can view the developer documentations that is synchronized with the current latest develop code. This document will
help you understand the source code and get involved more quickly in the process of contributing to open source.

[Developer Document](https://doxygen.gpgfrontend.pub)

## Build From Source Code

For some experts, building GpgFrontend from source code is a good option. I encourage people to freely build,
package and distribute their own versions. The method I build in major operating systems is as follows:

Note: "$" Symbols indicate commands to should be executed with a normal user.

```shell
$ git clone --recurse-submodules https://github.com/saturneric/GpgFrontend.git
```

### For Windows

Before building, you need to install MSYS2. After installation, open the MSYS2 terminal (MSYS2 MinGW 64-bit), enter the
MSYS2 directory, and execute the following commands:

```shell
pacman --noconfirm -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-cmake autoconf
pacman --noconfirm -S --needed make texinfo mingw-w64-x86_64-libconfig mingw-w64-x86_64-boost automake
pacman --noconfirm -S --needed mingw-w64-x86_64-qt5 libintl msys2-runtime-devel gettext-devel mingw-w64-x86_64-gpgme
pacman --noconfirm -S --needed mingw-w64-x86_64-ninja mingw-w64-x86_64-gnupg mingw-w64-x86_64-libarchive
```

After executing these commands, you will start compiling.

```shell
$ cd GpgFrontend
$ mkdir build && cd build
$ cmake -G Ninja  -DCMAKE_BUILD_TYPE="Release" ..
$ ninja
```

After compiling, a release directory will be generated, which contains executable files that can be run directly.

```shell
$ ./release/GpgFrontend.exe
```

### For macOS

Install and compile dependencies.

```shell
$ brew install cmake git autoconf automake qt@5 texinfo gettext openssl@1.1 libarchive
$ brew install boost ninja
$ brew unlink gettext && brew link --force gettext
$ brew link qt@5
```

Build the code separately for debug.

```shell
$ cd GpgFrontend
$ mkdir build && cd build
$ cmake -G Ninja  -DCMAKE_BUILD_TYPE="Debug" -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1  ..
$ ninja
$ ./release/GpgFrontend # run the program
```

Build the code and make the App Bundle.

```shell
$ cd GpgFrontend
$ mkdir build && cd build
$ cmake -G Ninja  -DCMAKE_BUILD_TYPE="Release" -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 ..
$ ninja
$ macdeployqt ./release/GpgFrontend.app
```

### For Ubuntu 18.04 And Later

Install and compile dependencies.

```shell
$ sudo apt-get update
$ sudo apt-get -y install build-essential binutils git autoconf automake gettext texinfo
$ sudo apt-get -y install gcc-8 g++-8 ninja-build
$ sudo apt-get -y install libconfig++-dev libboost-all-dev libarchive-dev libssl-dev
$ sudo apt-get -y install gpg # If you need to run directly after building.
```

Compile and install libgpg-error/libassuan/gpgme. Notice: These component's version in third_party directory is
newer than those in apt.

```shell
# libgpg-error
$ cd GpgFrontend
$ cd  ./third_party/libgpg-error
$ ./autogen.sh
$ ./configure --enable-maintainer-mode && make
$ sudo make install
# libassuan
$ cd GpgFrontend
$ cd  ./third_party/libassuan
$ ./autogen.sh
$ ./configure --enable-maintainer-mode && make
$ sudo make install
# gpgme
$ cd GpgFrontend
$ cd  ./third_party/gpgme
$ ./autogen.sh
$ ./configure --enable-maintainer-mode --enable-languages=cpp && make
$ sudo make install
```

Build the code and make the deb package(Please use Ubuntu 20.04 or later).

```shell
$ cd GpgFrontend
$ mkdir build && cd build
$ cmake -G Ninja  -DCMAKE_BUILD_TYPE="Release" -DGPGFRONTEND_GENERATE_LINUX_INSTALL_SOFTWARE=ON ..
$ ninja
$ ninja package
```

Build the code separately for debug(Please use ubuntu 18.04 or later).

```shell
$ cd GpgFrontend
$ mkdir build && cd build
$ cmake -G Ninja  -DCMAKE_BUILD_TYPE="Debug" ..
$ ninja
$ ./release/GpgFrontend # run the program
```

Package the AppImage(Should use ubuntu 18.04).

```shell
$ cd GpgFrontend
$ mkdir build && cd build
$ cmake -G Ninja  -DCMAKE_BUILD_TYPE="Release" ..
$ ninja
$ mkdir ./AppImageOut
$ cd ./AppImageOut
$ wget -c -nv https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
$ chmod u+x linuxdeployqt-continuous-x86_64.AppImage
$ ./linuxdeployqt-continuous-x86_64.AppImage ../release/gpgfrontend/usr/share/applications/*.desktop -appimage
```

## Languages Support

<<<<<<< HEAD
The supported languages are listed here. Some languages use machine translation and have not been verified. If you want
to join translation and verification work, please refer to [HERE](https://gpgfrontend.pub/#/translate-interface).
=======
The supported languages are listed here. Some translations use machine translation and have not been verified. If you want
to join translation or verification work, please refer to [HERE](https://gpgfrontend.pub/#/translate-interface).

> > > > > > > main

### Supported Languages

'zh_CN', 'zh_TW', 'zh_HK', 'es_ES', 'fr_FR', 'de_DE', 'pl_PL', 'ru_RU', 'ja_JP', 'it_IT',
'ko_KR', 'pt_BR', 'ar_SA', 'ar_IQ', 'hi_IN', 'af_ZA', 'sq_AL', 'be_BY', 'bg_BG', 'ca_ES',
'hr_HR', 'cs_CZ', 'da_DK', 'nl_NL', 'et_EE', 'fa_IR', 'fi_FI', 'fr_CA', 'he_IL', 'id_ID',
'lt_LT', 'de_AT', 'de_CH', 'el_GR', 'es_MX', 'iw_IL', 'uk_UA', 'en_US', 'en_GB', 'en_AU',

Notice: Most translations are generated by Google's automatic translation machine. If you find that a certain translation
is wrong, you are welcome to join the translation work to provide a more suitable human translation.

## Contract

Please refer to [HERE](https://www.gpgfrontend.pub/#/contract) for my contact details.

### Contributing & Bugs Report

Feel free to dive in! [Open an issue](https://github.com/saturneric/GpgFrontend/issues/new) or submit PRs if you prefer
to use GitHub. For anonymous users, Git patches can be delivered by [mail](mailto:eric@bktus.com).

### Project Maintainer

[@Saturneric](https://github.com/saturneric)

### Project's LOGO

![logo](https://github.com/saturneric/Blob/blob/master/logos/gpgfrontend-logo.jpg?raw=true)

## LICENSES

GpgFrontend itself is licensed under the [GPLv3](COPYING).

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_large)

### Dependencies

There are some libraries and binary included in the zip-file which (may) have different licenses, for more information
check their homepages. You can also obtain the sources from there.

gnupg: https://gnupg.org

gpgme: https://gnupg.org/software/gpgme/index.html

Qt(opensource): https://www.qt.io

MSYS2: https://www.msys2.org

mingw-w64: http://mingw-w64.org/doku.php

AppImage: https://appimage.org

JSON for Modern C++: https://github.com/nlohmann/json

Qt-AES: https://github.com/saturneric/Qt-AES

macOS Application Bundles: [Link](https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html#//apple_ref/doc/uid/10000123i-CH101-SW1)

The icons of this software use materials from [Alibaba Iconfont](!https://www.iconfont.cn/). The Alibaba vector icon
library is free to use. The icons in the free library aren't registered as trademarks. There is no copyright issue
involved and can be used commercially.
