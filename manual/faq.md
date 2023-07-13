# FAQ

## What is GpgFrontend?

GpgFrontend is a cross-platform encryption tool that adheres to the OpenPGP
standard. Its objective is to make the use of OpenPGP simpler, thereby enabling
more individuals to secure their privacy.

## What's the relationship between OpenPGP(PGP) and GnuPG(GPG)?

OpenPGP is a standard for data encryption and decryption, which is supported by
GpgFrontend. GnuPG is a cryptographic software that's used for encryption,
signing, and key management for asymmetric cryptography, and it follows the
OpenPGP standard. GpgFrontend utilizes GnuPG during its operation to perform
various tasks such as encryption and decryption.

## How to obtain and use GpgFrontend?

The various versions of GpgFrontend will be released in the GitHub repository,
and you can find and download the latest version
[HERE](https://www.gpgfrontend.bktus.com/#/downloads). After downloading, you can
refer to the instructions in ReadME and you can start using it in just a few
steps.

## How to deal with 'ENV Loading Failed'?

The reason for this problem is that GpgFrontend failed to find the GnuPG in your
machine. You can follow suggestions below.

### macOS

For macOS users, please install GnuPG for OSX
[Here](https://sourceforge.net/p/gpgosx/docu/Download/). Or just use Homebrew to
install GpgFrontend. By executing command:

`brew install --cask gpgfrontend`.

If you have installed GnuPG under a custom path, you can add the "bin" directory
of GnuPG in to PATH.

### Linux

For Linux users, please install GnuPG through apt or yum.

If you have installed GnuPG under a custom path, you can add the "bin" directory
of GnuPG in to PATH.

### Windows

For Windows users, GnuPG is now integrated in the latest version of GpgFrontend,
we recommend you to download the latest GpgFrontend if you don't have any ideas.

Or, you can download GnuPG installers for Windows
[HERE](https://www.gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.4.0_20221216.exe).
Try to reinstall GnuPG through installer when you have already install it.

### More Tips?

For more tips, you can see the quick start manual [HERE](quick-start.md). It
provides more information on dealing this situation.

## I found some bugs in GpgFrontend, what should I do?

If you find a defect in GpgFrontend, you are welcome to create an issue in the
GitHub repository to describe the problem. When I see your issue, I will respond
as soon as possible. If you do not have a GitHub account, please email my
personal mailbox. For contact information, please see [Contract](contract.md).

## Can I modify the code of GpgFrontend?

If you have any good ideas, you are free to modify the code of GpgFrontend. You
are welcome to submit a Pull Request to add your good ideas to the next version.

You can contribute completely anonymously, and you can email me patches.

## Why should I install GnuPG additionally?

The source code of GpgFrontend does not contain operations such as encryption
and decryption, which requires Gnupg to provide support. In addition, for users
with higher security requirements, they can let GpgFrontend drive their trusted
copy of Gnupg. This design improves the security of GpgFrontend.

## What is the release version with BETA?

The release version with the word "beta" means that some modules of this version
have not yet been thoroughly tested. In addition, some support for the beta
version may not be complete. But rest assured, I will test after the beta
version is released, and release a stable version at an appropriate time.

But starting from 2.0.0, BETA versions will not be released unless there are
special circumstances.
