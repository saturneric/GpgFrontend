# FAQ

## What is GpgFrontend?

GpgFrontend is a cross-platform encryption tool that conforms to the OpenPGP standard. It is committed to making GnuPG
easier to use, so that more people can use the tool to protect their privacy during their communications on Internet.

## Relationship between OpenPGP(PGP) and GnuPG(GPG)？

OpenPGP(PGP) is a data encryption and decryption standard, and GpgFrontend supports it. GnuPG(GPG) is a cryptographic
software used to encrypt, sign communication content and manage keys for asymmetric cryptography. It follows the
OpenPGP standard. GpgFrontend drives GnuPG at runtime to implement operations such as encryption and decryption.

## How to obtain and use GpgFrontend?

The various versions of GpgFrontend will be released in the GitHub repository, and you can find and download the latest
version [HERE](https://www.gpgfrontend.pub/#/downloads). After downloading, you can refer to the instructions in README
and you can start using it in just a few steps.

## How to deal with 'ENV Loading Failed'?

The reason for this problem is that GpgFrontend failed to find the GnuPG in your machine. You can follow suggestions
below.

### macOS

For macOS users, please install GnuPG for OSX [Here](https://sourceforge.net/p/gpgosx/docu/Download/). Or just use Homebrew
to install GpgFrontend. By executing command:

`brew install --cask gpgfrontend`.

If you have installed GnuPG under a custom path, you can add the "bin" directory of GnuPG in to PATH.

### Linux

For Linux users, please install GnuPG through apt or yum.

If you have installed GnuPG under a custom path, you can add the "bin" directory of GnuPG in to PATH.

### Windows

For Windows users, GnuPG is now integrated in the latest version of GpgFrontend, we recommend you to download the
latest GpgFrontend if you don't have any ideas.

Or, you can download GnuPG installers for Windows [HERE](https://www.gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.4.0_20221216.exe).
Try to reinstall GnuPG through installer when you have already install it.

### More Tips?

For more tips, you can see the quick start manual [HERE](quick-start.md). It provides more information on
dealing this situation.

## I found some bugs in GpgFrontend, what should I do?

If you find a defect in GpgFrontend, you are welcome to create an issue in the GitHub repository to describe the
problem. When I see your issue, I will respond as soon as possible. If you do not have a GitHub account, please email my
personal mailbox. For contact information, please see [Contract](contract.md).

## Can I modify the code of GpgFrontend?

If you have any good ideas, you are free to modify the code of GpgFrontend. You are welcome to submit a Pull Request to
add your good ideas to the next version.

You can contribute completely anonymously, and you can email me patches.

## Why should I install GnuPG additionally?

The source code of GpgFrontend does not contain operations such as encryption and decryption, which requires Gnupg to
provide support. In addition, for users with higher security requirements, they can let GpgFrontend drive their trusted
copy of GnuPG. This design improves the security of GpgFrontend.

## What is the release version with BETA?

The release version with the word "beta" means that some modules of this version have not yet been thoroughly tested. In
addition, some support for the beta version may not be complete. But rest assured, I will test after the beta version is
released, and release a stable version at an appropriate time.

But starting from 2.0.0, BETA versions will not be released unless there are special circumstances.
