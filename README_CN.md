<img width="100" height="100" align="right" src="https://github.com/saturneric/Blob/blob/master/logos/icon.png?raw=true" alt="ICON"/>

# GpgFrontend

![Language](https://img.shields.io/badge/language-C%2B%2B-green)
![License](https://img.shields.io/badge/License-GPL--3.0-orange)
![CodeSize](https://img.shields.io/github/languages/code-size/saturneric/GpgFrontend)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d1750e052a85430a8f1f84e58a0fceda)](https://www.codacy.com/gh/saturneric/GpgFrontend/dashboard?utm_source=github.com&utm_medium=referral&utm_content=saturneric/GpgFrontend&utm_campaign=Badge_Grade)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/saturneric/gpgfrontend)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend.svg?type=small)](https://app.fossa.com/projects/git%2Bgithub.com%2Fsaturneric%2FGpgFrontend?ref=badge_small)

GpgFrontend 是一个易于使用、小巧、跨平台和免安装的 [OpenPGP](https://www.openpgp.org/) 加密解密签名工具。

通过使用 GpgFrontend，你可以快速加密和解密文本或文件。或者在进行上述操作的同时加上自己的签名，让别人知道这个文件或者这段文字是出自你之手。
该软件旨在让普通用户更快上手 gpg 工具，让专业用户更便捷。 GpgFrontend 支持 OpenPGP 的新特性。

注意：GpgFrontend 不提供嵌入式 gnupg 二进制库，需要用户自行安装。这是为了确保安全，避免涉及加密解密的代码或二进制文件在传递过程中被植入后门。

如果你觉得这款工具有用或者有希望，`欢迎通过STAR这个项目来鼓励我`。

[>> 快速开始](#quick-start)

[>> 代码与二进制文件安全](https://saturneric.github.io/GpgFrontend/index.html#/about/code-binary-verify)

<div align="center">
<img width="640" src="https://github.com/saturneric/Blob/blob/master/screenshots/main_mac.jpg?raw=true" alt="macOS Screenshot"/>
</div>
<div align="center">
<img width="320" src="https://github.com/saturneric/Blob/blob/master/screenshots/key_info.PNG?raw=true" alt="Windows Screenshot"/>
<img width="320" src="https://github.com/saturneric/Blob/blob/master/screenshots/keygen_ubuntu.png?raw=true" alt="Ubuntu Screenshot"/>
</div>

#### Workflows 状态：

[![Build & Package](https://github.com/saturneric/GpgFrontend/actions/workflows/release.yml/badge.svg?branch=main)](https://github.com/saturneric/GpgFrontend/actions/workflows/cmake.yml)

---

## 内容目录

- [软件特性](#软件特性)
- [使用方法](#使用方法)
    - [快速开始](#快速开始)
    - [如何安装](#如何安装)
        - [Windows](#windows)
        - [macOS](#macos)
        - [Debian/Ubuntu/CentOS](#debianubuntucentos)
- [文档](#文档)
- [开发宗旨](#开发宗旨)
- [构建方法](#构建方法)
- [联系](#联系)
    - [做出贡献或者报告问题](#做出贡献或者报告问题)
    - [关于维护者](#关于维护者)
- [许可证](#许可证)

## 软件特性

- 可以在 Windows、Linux 和 macOS 上运行。
- 开源，免费，无需安装。
- 只需双击即可自由使用。
- 支持多种语言。
    - 有兴趣的可以帮我 [翻译一下界面](https://saturneric.github.io/GpgFrontend/index.html#/translate-interface) 。

## 使用方法

### 快速开始

### 快速加密

只需单击几下即可完成加密。

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encrypt.gif?raw=true)

### 快速解密

我想马上看看对方写了什么。

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt.gif?raw=true)

### 快速文件加密与签名

对于文件，我该怎么做？

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encr-sign-file.gif?raw=true)

### 还有那些有用的操作

阅读 [文档](https://saturneric.github.io/GpgFrontend/index.html#/) 中的动图，了解更多精彩操作。

### 如何安装

#### Windows

1. [下载](https://gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.3.1_20210420.exe) gnupg-w32-**\*\***.exe
2. 双击安装
3. [下载 GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) Windows Edition from release
4. 解压 gpgfrontend-windows-latest-**\*\*\***.zip
5. 进入目录，双击运行 GpgFrontend.exe

#### macOS

0. 如果命令 `gpg` 不可用,请先用 Homebrew 安装它.
1. [下载 GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) macOS edition from release
2. 双击并加载 GpgFrontend.dmg
3. 双击并运行
   (由于 macOS 的安全策略，在真正能运行前请遵照系统说明).
4. 如果你满意的话，可以将本软件复制到 Application 文件夹.

#### Debian/Ubuntu/CentOS

1. 安装 gnupg (如果你已经安装了请跳过)
    - For Debian/Ubuntu
      ```shell
      $ sudo apt update
      $ sudo apt install gpg
      ```
    - For CentOS
      ```shell
      $ sudo yum install gnupg
      ```
2. [下载 GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) Linux edition from the latest release
3. 解压 gpgfrontend-ubuntu-16.04-**\*\*\***.zip
    - `ubuntu-16.04` 说明 ubuntu 16.04 及其同时期与后来的 Linux 发行版都可以正常运行。
4. 赋予 `GpgFrontend-***.AppImage` 执行权限
   ```shell
   $ chmod u+x ./GpgFrontend-***.AppImage
   ```
5. 双击运行 `GpgFrontend-***.AppImage`.

## 文档

如果你想获取更多信息，请阅读 [文档](https://saturneric.github.io/GpgFrontend/index.html#/).

## 开发宗旨

GpgFrontend 项目继承自一个相对成熟但未维护的 [gpg4usb](https://www.gpg4usb.org/) 项目。
它继承了 gpg4usb 稳定、易用、小巧、免安装的特点。

GpgFrontend 未来会增加更多功能，提高 GPG 在端到端传输中的易用性并缩短密文长度。同时，新功能的加入不影响旧的基础功能。
我个人的力量总是有限的。 GpgFrontend 欢迎志愿者加入。你可以使用 GitHub 平台提交问题或提交 pull request。

GpgFrontend 项目作为开源项目，也坚持使用开源代码和库。

## 构建方法

构建软件的教程将很快发布。你可以先查看本项目的 GitHub Action 文件。

## 联系

如果你想与我私下联系，你可以发送电子邮件到 [eric@bktus.com](mailto:eric@bktus.com)。

### 做出贡献或者报告问题

欢迎！你可以通过 [提出 issue](https://github.com/saturneric/GpgFrontend/issues/new) 或提交 PR 来做出贡献。

### 关于维护者

该项目的维护者是 [@Saturneric](https://github.com/saturneric).

## LOGO

![logo](https://github.com/saturneric/Blob/blob/master/logos/gpgfrontend-logo.jpg?raw=true)

## 许可证

GpgFrontend 在 [GPLv3](COPYING) 许可证下。

项目使用到了一些库和二进制文件，它们（可能）具有不同的许可证，请查看它们的主页获取更多信息。您也可以从那里获取相关信息。

gnupg: https://gnupg.org/

gpg4usb: https://www.gpg4usb.org/

QT 5.15.2(opensource): https://www.qt.io/

MSYS2: https://www.msys2.org/

mingw-w64: http://mingw-w64.org/doku.php

AppImage: https://appimage.org/

rapidjson: https://github.com/Tencent/rapidjson

Application Bundles: [Link](!https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html#//apple_ref/doc/uid/10000123i-CH101-SW1)

本软件图标使用来自 [阿里巴巴矢量图标库](!https://www.iconfont.cn/) 的素材。免费库中的图标未注册为商标。不涉及版权问题。