# Quick Start

Getting started with GpgFrontend is very simple, you only need a few very simple steps. Moreover, it is oriented to all
platforms.

## Install & Run

### Windows

1. [Download](https://gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.3.1_20210420.exe) gnupg-w32-******.exe
2. Double Click it to install it
3. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases)  Windows Edition from release
4. Unzip gpgfrontend-windows-latest-*******.zip
5. Go into the directory and double click GpgFrontend.exe

### macOS

1. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) macOS edition from release
2. Double-Click GpgFrontend.dmg to load it
3. Double click and run it
   (due to macOS security policy, you may need a little more step).
4. If it satisfies you, you can drag it into your Application folder.

### Debian/Ubuntu/CentOS

1. Install gnupg
    - For Debian/Ubuntu
       ```shell
       $ sudo apt update
       $ sudo apt install gpg qt-default
       ```
    - For CentOS
       ```shell
       $ sudo yum install gnupg qt5-qtbase
       ```
2. [Download GpgFrontend](https://github.com/saturneric/GpgFrontend/releases) Linux edition from release
3. Unzip gpgfrontend-ubuntu-latest-*******.zip
4. Get into folder and Give gpgfrontend permission to execute
    ```shell
    $ cd gpgfrontend-ubuntu-latest-*******/
    $ chmod u+x GpgFrontend
    ```
5. Just run it
    ```shell
    $ ./GpgFrontend
    ```