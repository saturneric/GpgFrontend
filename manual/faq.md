# FAQ

## What is GpgFrontend？

GpgFrontend is a cross-platform encryption tool that conforms to the OpenPGP standard. It is committed to making OpenPGP
easier to use, so that more people can use the tool to protect their privacy.

## Relationship between OpenPGP(pgp) and Gnupg(gpg)？

OpenPGP is a data encryption and decryption standard, and GpgFrontend supports it. GnuPG is a cryptographic software
used to encrypt, sign communication content and manage keys for asymmetric cryptography. It follows the OpenPGP
standard. GpgFrontend drives gnupg at runtime to implement operations such as encryption and decryption.

## How to obtain and use GpgFrontend？

The various versions of GpgFrontend will be released in the GitHub repository, and you can find and download the latest
version [HERE](https://www.gpgfrontend.pub/#/downloads). After downloading, you can refer to the instructions in ReadME and you can start using it in just a
few steps.

## I found some flaws in GpgFrontend, what should I do?

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
copy of Gnupg. This design improves the security of GpgFrontend.

## What is the release version with BETA?

The release version with the word "beta" means that some modules of this version have not yet been thoroughly tested. In
addition, some support for the beta version may not be complete. But rest assured, I will test after the beta version is
released, and release a stable version at an appropriate time.

But starting from 2.0.0, BETA versions will not be released unless there are special circumstances.

## How to deal with 'ENV Loading Failed'?

The reason for this problem is that GpgFrontend failed to find the GnuGP tool in your computer.

- For macOS users, please install GnuPG for OSX [Here](https://sourceforge.net/p/gpgosx/docu/Download/).
- For Linux users, please install gnupg through apt or yum.
- For Windows users, GnuPG is now integrated in the latest version of GpgFrontend, we recommend you to download the
  latest GpgFrontend.

