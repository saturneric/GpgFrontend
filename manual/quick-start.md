# Getting Started

One of the unique features of GpgFrontend is its cross-platform capability.
Depending on your operating system, the installation process might vary.

## Prerequisites

**If you are a Windows or macOS user with Homebrew, you can skip this section.**

GpgFrontend relies on the basic functions provided by GnuPG. Hence, it is
necessary to install GnuPG before running GpgFrontend. Starting from GnuPG 2.0,
GPG operates based on separate modules for all its functionalities. For GPG to
operate smoothly, these modules must be correctly situated within your operating
system.

Unfortunately, GnuPG cannot operate within an App Sandbox, which is why
GpgFrontend is currently not available on the Apple Store. I have attempted
multiple times to integrate GnuPG into the App Sandbox but to no avail.

By default, most latest Linux distributions come with a GnuPG 2.0 environment.
You can verify this by entering `gpg --version` in the command line tool. **As a
general recommendation, you should install versions of GnuPG 2.2 or higher.**

## Install & Run Steps

### Windows (No Setup)

1. [Download](https://github.com/saturneric/GpgFrontend/releases/latest)
   `GpgFrontend-*******-windows-x86_64-portable.zip`
2. Unzip it. (Necessary)
3. Go into the `Program/bin` directory and double click `GpgFrontend.exe`.

### Windows（Setup）

1. [Download](https://github.com/saturneric/GpgFrontend/releases/latest)
   `GpgFrontend-*******-windows-x86_64-setup.exe`
2. Install it, and you can find GpgFrontend on your desktop.

### macOS

All published app packages have passed Apple's check, which means you can open it directly without
extra permission.

#### Homebrew Cask

You can use Homebrew Cask to quickly and easily install or remove GpgFrontend in your machine.

0. Check if Homebrew is installed in your machine.
1. Install GpgFrontend by running command `brew install --cask gpgfrontend`
2. Find GpgFrontend in Launchpad, double-click and run it.

#### Download and Install from DMG

0. Install `gnupg` using Homebrew OR download "GPG for OS X"
   [Here](https://sourceforge.net/projects/gpgosx/files).
1. [Download](https://github.com/saturneric/GpgFrontend/releases)
   `GpgFrontend-*******-macos-**.dmg`
   - `x86_64` just means it build in such an x86 machine. Actually, it can run
     smoothly on both Intel and Apple(M1/M2) Chips.
2. Double-Click GpgFrontend.dmg to load it.
3. Double-Click and run it.
4. If it satisfies you, you can drag it into your Application folder.

#### Debian/Ubuntu/CentOS (AppImage)

AppImage is a format used in Linux systems to distribute portable software
without the need for superuser privileges to install them. The core idea of
AppImage is a file as an application. Each AppImage contains the application and
all the files needed for the application to run. In other words, in addition to
the underlying components of the operating system itself, AppImage runs without
dependency. This is convenient for users.

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
1. [Download](https://github.com/saturneric/GpgFrontend/releases) `GpgFrontend-*******-linux-x86_64.AppImage`
2. Give it permission to execute
   ```shell
   $ chmod u+x ./GpgFrontend-***-linux-x86_64.AppImage
   ```
3. Just double-click it to run it.

## BSD(FreeBSD/OpenBSD)

For BSD users, unfortunately, the binary release is not provided yet. But you
can build and run GpgFrontend in these operating systems.

## Get from GitHub Release

The current mainstream distribution channel is the Release feature available
through GitHub. It's free and accessible to most people in the world, without me
having to worry about servers and bandwidth, which allows me to save a lot of
money.

When you click on the
[download](https://github.com/saturneric/GpgFrontend/releases/latest), you can
see such an interface. It identifies the current version number of the latest
release, the release date of that version, and so on.

![image-20220101225029218](https://www.bktus.com/wp-content/uploads/2023/08/image-20220101225029218.png)

You can see some notable features or fixes for the version in the main text, or
if you're a programmer, you can also click change log to get how the source code
differs from the previous version. It is worth mentioning that you can see two
green tick marks, which represent that I have personally signed up to the source
code of the version. This may be important for some people with high security
needs.

Then, if you swipe down, you can see a lot of Assets, which are releases for
GpgFrontend for different operating systems. As you can see, the parts of the
file name are separated by separators.

You need to know that the second section(e.g. 6171479) provides a unique
identification number for the version's source code, and when pointing out
problems with a version, you need to provide the 7-digit unique identification
number of the released version you are using.

![image-20220101225652736](https://www.bktus.com/wp-content/uploads/2023/08/image-20220101225652736.png)

Files with signature as the suffix are GPG separate signatures for the released
version of the file of the same name. You can use GPG to check if the changed
file is signed and approved by me.

Follow your needs or follow the instructions below to click on the name of the
corresponding release version to download.
