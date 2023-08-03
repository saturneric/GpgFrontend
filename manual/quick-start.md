# Getting Started

GpgFrontend is a versatile tool featuring cross-platform compatibility. The
installation process may vary depending on your operating system. 

## Prerequisites

**For Windows or macOS users with Homebrew, this section can be skipped.**

As GpgFrontend is built on the basic functions provided by GnuPG, you need to
install GnuPG before running GpgFrontend. From GnuPG 2.0 onward, GPG works on
separate modules for each of its functionalities, and these modules need to be
correctly integrated into your operating system for smooth operation.

Please note, GnuPG cannot operate within an App Sandbox, so GpgFrontend is
currently not available on the Apple Store.

Most recent Linux distributions come with a pre-installed GnuPG 2.0 environment.
You can verify this by typing `gpg --version` in the command line. **However, it
is recommended to install GnuPG versions 2.2 or higher.**

## Install & Run Steps

### Windows (No Setup)

1. [Download](https://github.com/saturneric/GpgFrontend/releases/latest) the
   file `GpgFrontend-*******-windows-x86_64-portable.zip`.
2. Unzip the downloaded file. (This step is necessary)
3. Navigate to the `Program/bin` directory and double-click `GpgFrontend.exe`.

### Windows (Setup)

1. [Download](https://github.com/saturneric/GpgFrontend/releases/latest) the
   file `GpgFrontend-*******-windows-x86_64-setup.exe`.
2. Follow the installation process, and GpgFrontend will be accessible from your
   desktop.

### macOS

All published app packages have passed Apple's verification checks, meaning you
can open it directly without requiring extra permissions.

#### Homebrew Cask

Use Homebrew Cask to install or remove GpgFrontend from your machine quickly and
easily.

0. Ensure Homebrew is installed on your machine.
1. Install GpgFrontend by running the command `brew install --cask gpgfrontend`.
2. Find GpgFrontend in your Launchpad, double-click, and run it.

#### Download and Install from DMG

0. Install `gnupg` using Homebrew OR download "GPG for OS X" from
   [Here](https://sourceforge.net/projects/gpgosx/files).
1. [Download](https://github.com/saturneric/GpgFrontend/releases)
   `GpgFrontend-*******-macos-**.dmg`.
   - `x86_64` indicates the build machine type. This version will run smoothly
     on both Intel and Apple(M1/M2) Chips.
2. Double-click GpgFrontend.dmg to load it.
3. Double-click to run it.
4. If you're satisfied with the program, you can drag it into your Application
   folder.

#### Debian/Ubuntu/CentOS (AppImage)

AppImage is a format used in Linux systems to distribute portable software
without needing superuser privileges to install them. It packages the
application and all files needed for the application to run in a single file,
thus avoiding dependency issues. This makes the distribution of software more
user-friendly.

0. Install gnupg (Skip if you have already done this)
   - For Debian/Ubuntu:
     ```shell
     $ sudo apt update
     $ sudo apt install gpg
     ```
   - For CentOS:
     ```shell
     $ sudo yum install gnupg
     ```
1. [Download](https://github.com/saturneric/GpgFrontend/releases) the file
   `GpgFrontend-*******-linux-x86_64.AppImage`.
2. Grant execute permissions to the file:
   ```shell
   $ chmod u+x ./GpgFrontend-***-linux-x86_64.AppImage
   ```
3. Double-click the file to run it.

## BSD(FreeBSD/OpenBSD)

If you're a BSD user, note that binary releases aren't provided yet. However,
you can still build and run GpgFrontend on these operating systems.

## Get from GitHub Release

The primary distribution channel is the Release feature available on GitHub.
This method is free and accessible to most people globally, eliminating the need
for me to manage servers and bandwidth, thus saving resources.

Upon clicking the
[download](https://github.com/saturneric/GpgFrontend/releases/latest) link, you
will see an interface displaying the current version number of the latest
release, its release date, and more.

![image-20220101225029218](https://www.bktus.com/wp-content/uploads/2023/08/image-20220101225029218.png)

In the main text, you can find some significant features or fixes for the
version. If you're a programmer, you can also view the change log to see how the
source code differs from the previous version. Notice the two green tick marks,
which represent that I have personally signed the source code for the version.
This can be crucial for users with high-security needs.

Scrolling down, you can find several Assets. These are releases of GpgFrontend
for different operating systems. The parts of the filename are separated by
separators. The second section (e.g., 6171479) is a unique identification number
for the version's source code. When pointing out problems with a version,
remember to provide this 7-digit unique identification number.

![image-20220101225652736](https://www.bktus.com/wp-content/uploads/2023/08/image-20220101225652736.png)

Files with 'signature' as the suffix are GPG separate signatures for the
corresponding released version of the file. You can use GPG to verify whether
the downloaded file is signed and approved by me.

Choose the appropriate release version according to your needs or the
instructions provided, and click on the name to download.