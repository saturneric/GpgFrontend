# Quick Start

Getting started with GpgFrontend is very simple, you only need a few very simple steps. Moreover, it is oriented to all
platforms.

## Install & Run

#### Windows

0. If you haven't installed gnupg,
   please [Download](https://gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.3.1_20210420.exe) `gnupg-w32-******.exe` and
   install it.
1. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases)  Windows Edition from the latest release
2. Unzip `gpgfrontend-windows-latest-*******.zip`
3. Go into the directory and double click `GpgFrontend.exe`.

#### macOS

1. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) macOS edition from the latest release
2. Double-Click GpgFrontend.dmg to load it
   - macOS will automatically decompress the zip file and then you will be able to see the dmg
3. Double click and run it
   (due to macOS security policy, you may need a little more step).
4. If it satisfies you, you can drag it into your Application folder.

#### Debian/Ubuntu/CentOS

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