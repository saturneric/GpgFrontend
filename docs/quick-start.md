# Quick Start

Getting started with GpgFrontend is very simple, you only need a few very simple steps. Moreover, it is oriented to all
platforms.

## Install & Run

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