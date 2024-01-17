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

GpgFrontend is a free, open-source, robust yet user-friendly, compact and
cross-platform tool for [OpenPGP](https://www.openpgp.org/) encryption. It
stands out as an exceptional GUI frontend for the modern
[GnuPG](https://www.gnupg.org/) (gpg).

When using GpgFrontend, you can:

- Rapidly encrypt files or text.
- Digitally sign your files or text with ease.
- Conveniently manage all your GPG keys on your device.
- Transfer all your GPG keys between devices safely and effortlessly.
- Furthermore, you can build and run it on various operating systems, including
  Windows, macOS, Linux, FreeBSD, and more.

GpgFrontend is **PERMANENTLY FREE** of charge. However, you can support us by
"starring" this project. Your contributions are highly appreciated!

[Language Supported](#language-support) by GpgFrontend including English, Chinese, French, Russian, German, Spanish, Portuguese, Arabic, etc.

[>> Download <<](https://github.com/saturneric/GpgFrontend/releases/latest)
| [>> User Manual <<](https://www.gpgfrontend.bktus.com/#/overview)
| [>> Developer Document <<](https://doxygen.gpgfrontend.bktus.com/)

## Table of Contents

- [User Manual](#user-manual)
- [System Requirement](#system-requirement)
- [Developer Document](#developer-document)
- [Build From Source Code](#build-from-source-code)
- [Language Support](#language-support)
- [Contract](#contract)
- [Licenses](#licenses)

## User Manual

GpgFrontend provides documentations on its main features. If you want to know how to install, please
read the [User Manual](https://www.gpgfrontend.bktus.com/#/quick-start) instead of README.

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

## Developer Document

You can view the developer documentations that is synchronized with the current latest develop code. This document will
help you understand the source code and get involved more quickly in the process of contributing to open source.

[Developer Document](https://doxygen.gpgfrontend.bktus.com)

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
pacman --noconfirm -S --needed mingw-w64-x86_64-qt6 libintl msys2-runtime-devel gettext-devel mingw-w64-x86_64-gpgme
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
$ ./artifacts/GpgFrontend.exe
```

### For macOS

Install and compile dependencies.

```shell
$ brew install cmake git autoconf automake qt@6 texinfo gettext openssl@1.1 libarchive
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
$ ./artifacts/GpgFrontend # run the program
```

Build the code and make the App Bundle.

```shell
$ cd GpgFrontend
$ mkdir build && cd build
$ cmake -G Ninja  -DCMAKE_BUILD_TYPE="Release" -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 ..
$ ninja
$ macdeployqt ./artifacts/GpgFrontend.app
```

### For Ubuntu 18.04 And Later

Install and compile dependencies.

```shell
$ sudo apt-get update
$ sudo apt-get -y install build-essential binutils git autoconf automake gcc-8 g++-8 ninja-build
$ sudo apt-get -y install gettext texinfo qt6-base-dev libqt6core5compat6-dev libgpgme-dev
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
$ ./configure --enable-maintainer-mode && make -j$(nproc)
$ sudo make install
# libassuan
$ cd GpgFrontend
$ cd  ./third_party/libassuan
$ ./autogen.sh
$ ./configure --enable-maintainer-mode && make -j$(nproc)
$ sudo make install
# gpgme
$ cd GpgFrontend
$ cd  ./third_party/gpgme
$ ./autogen.sh
$ ./configure --enable-maintainer-mode --enable-languages=cpp && make -j$(nproc)
$ sudo make install
```

Build the code separately for debug(Please use ubuntu 18.04 or later).

```shell
$ cd GpgFrontend
$ mkdir build && cd build
$ cmake -G Ninja  -DCMAKE_BUILD_TYPE="Debug" ..
$ ninja
$ ./artifacts/GpgFrontend # run the program
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
$ ./linuxdeployqt-continuous-x86_64.AppImage ../artifacts/gpgfrontend/usr/share/applications/*.desktop -appimage
```

## Language Support

Listed below are the languages currently supported by GpgFrontend. Please be
aware that some of these languages are translated via machine translation and
may not have been manually verified for accuracy. If you are interested in
joining our translation and verification work, please refer to [this
link](https://gpgfrontend.bktus.com/#/translate-interface).

### Supported Languages

GpgFrontend currently supports a wide array of languages including:

- Chinese
- Spanish: 'es_ES'
- French: 'fr_FR'
- German: 'de_DE'
- Polish: 'pl_PL'
- Russian: 'ru_RU'
- Japanese: 'ja_JP'
- Italian: 'it_IT'
- Korean: 'ko_KR'
- Brazilian Portuguese: 'pt_BR'
- Arabic: 'ar_SA'
- Hindi: 'hi_IN'
- Afrikaans: 'af_ZA'
- Albanian: 'sq_AL'
- Belarusian: 'be_BY'
- Bulgarian: 'bg_BG'
- Catalan: 'ca_ES'
- Croatian: 'hr_HR'
- Czech: 'cs_CZ'
- Danish: 'da_DK'
- Dutch: 'nl_NL'
- Estonian: 'et_EE'
- Persian: 'fa_IR'
- Finnish: 'fi_FI'
- Hebrew: 'he_IL'
- Indonesian: 'id_ID'
- Lithuanian: 'lt_LT'
- Greek: 'el_GR'
- Ukrainian: 'uk_UA'
- English: 'en_US'

**Notice:** Please note that most translations are generated by Google's
automatic translation service. If you find an error in any of the translations,
we welcome you to [join our translation
work](https://www.gpgfrontend.bktus.com/#/translate-interface) to provide a more
accurate human translation.

## Contract

Please refer to [HERE](https://www.gpgfrontend.bktus.com/#/contract) for my contact details.

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

### Dependencies and Acknowledgements

GpgFrontend incorporates various libraries and binaries that come with their
unique licenses. For additional details or to obtain the source code, please
visit their respective homepages:

- **GnuPG**: [https://gnupg.org](https://gnupg.org)
- **GPGME**: [https://gnupg.org/software/gpgme/index.html](https://gnupg.org/software/gpgme/index.html)
- **Qt (Open Source)**: [https://www.qt.io](https://www.qt.io)
- **MSYS2**: [https://www.msys2.org](https://www.msys2.org)
- **Mingw-w64**: [http://mingw-w64.org/doku.php](http://mingw-w64.org/doku.php)
- **AppImage**: [https://appimage.org](https://appimage.org)
- **JSON for Modern C++**: [https://github.com/nlohmann/json](https://github.com/nlohmann/json)
- **Qt-AES**: [https://github.com/saturneric/Qt-AES](https://github.com/saturneric/Qt-AES)
- **macOS Application Bundles**: [Link](https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html#//apple_ref/doc/uid/10000123i-CH101-SW1)

The icons utilized in this software are sourced from [Alibaba
Iconfont](https://www.iconfont.cn/). This vector icon library is free of use,
isn't registered as a trademark, has no copyright issues, and can be
commercially utilized.
