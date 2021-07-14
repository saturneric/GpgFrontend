# Quick Start

Getting started with GpgFrontend is very simple, you only need a few very simple steps. Moreover, it is oriented to all
platforms.

## Install & Run

#### Windows

1. [Download](https://gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.3.1_20210420.exe) `gnupg-w32-******.exe`
2. Double Click it to install it
3. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases)  Windows Edition from the latest release
4. Unzip `gpgfrontend-windows-latest-*******.zip`
5. Go into the directory and double click `GpgFrontend.exe`.

#### macOS

0. If command `gpg` is not available, please use homebrew to install it first.
1. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) macOS edition from the latest release
2. Double-Click GpgFrontend.dmg to load it
3. Double click and run it
   (due to macOS security policy, you may need a little more step).
4. If it satisfies you, you can drag it into your Application folder.

#### Debian/Ubuntu/CentOS

1. Install gnupg (If you have already followed please skip)
   - For Debian/Ubuntu
      ```shell
      $ sudo apt update
      $ sudo apt install gpg
      ```
   - For CentOS
      ```shell
      $ sudo yum install gnupg 
      ```
2. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) Linux edition from the latest release
3. Unzip gpgfrontend-ubuntu-16.04-*******.zip
   - `ubuntu-16.04` shows that Linux distributions at the same time as 16.04 or later are supported.
4. Give `GpgFrontend-***.AppImage` permission to execute
    ```shell
    $ chmod u+x ./GpgFrontend-***.AppImage
    ```
5. Just double-click `GpgFrontend-***.AppImage` to run it.