# Quick Start

Getting started with GpgFrontend is very simple, you only need a few very simple steps. Moreover, it is oriented to all
platforms.

## Install & Run

### Windows

1. [Download](https://gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.3.1_20210420.exe) gnupg-w32-******.exe
2. Double Click it to install it
3. [Download](https://github.com/saturneric/GpgFrontend/releases) Latest GpgFrontend Windows Edition
4. Unzip GpgFrontend
5. Double Click gpgfrontend.exe

### MacOS

1. Install Homebrew [Here](https://brew.sh/) if you don't know it.
2. Install gnupg
    ```shell
    % brew update
    % brew install gnupg
    ```
3. [Download](https://github.com/saturneric/GpgFrontend/releases) Latest GpgFrontend macOS Edition
4. Unzip GpgFrontend
5. Get into folder and Give gpgfrontend permission to execute
    ```shell
    % cd gpgfrontend-*.*.*-macos-amd64-release/
    % chmod u+x gpgfrontend
    ```
6. Just run it
    ```shell
    $ ./gpgfrontend
    ```

### Debian/Ubuntu/CentOS

1. Install gnupg
    - For Debian/Ubuntu
       ```shell
       $ sudo apt update
       $ sudo apt install gpg
       ```
    - For CentOS
       ```shell
       $ sudo yum install gnupg
       ```
2. [Download](https://github.com/saturneric/GpgFrontend/releases) Latest GpgFrontend Linux Edition
3. Unzip GpgFrontend
4. Get into folder and Give gpgfrontend permission to execute
    ```shell
    $ cd gpgfrontend-*.*.*-linux-amd64-release/
    $ chmod u+x gpgfrontend
    ```
5. Just run it
    ```shell
    $ ./gpgfrontend
    ```