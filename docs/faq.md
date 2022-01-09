# FAQ

## What is GpgFrontend

GpgFrontend is a cross-platform encryption tool that conforms to the OpenPGP standard. It is committed to making OpenPGP
easier to use, so that more people can use the tool to protect their privacy.

## Relationship with OpenPGP(pgp) and Gnupg(gpg)

OpenPGP is a data encryption and decryption standard, and GpgFrontend supports it. GnuPG is a cryptographic software
used to encrypt, sign communication content and manage keys for asymmetric cryptography. It follows the OpenPGP
standard. GpgFrontend drives gnupg at runtime to implement operations such as encryption and decryption.

## How to obtain and use GpgFrontend

The various versions of GpgFrontend will be released in the GitHub repository, and you can find and download the latest
version in Releases. After downloading, you can refer to the instructions in ReadME and you can start using it in just a
few steps.

## I found some flaws in GpgFrontend, what should I do?

If you find a defect in GpgFrontend, you are welcome to create an issue in the Github repository to describe the
problem. When I see your issue, I will respond as soon as possible. If you do not have a GitHub account, please send an
email to my personal mailbox. For contact information, please see Contract.

## Can I modify the code of GpgFrontend?

If you have any good ideas, you are free to modify the code of GpgFrontend. You are welcome to submit a Pull Request to
add your good ideas to the next version.

## Why should I install GnuPG additionally

The source code of GpgFrontend does not contain operations such as encryption and decryption, which requires Gnupg to
provide support. In addition, for users with higher security requirements, they can let GpgFrontend drive their trusted
copy of Gnupg. This design improves the security of GpgFrontend.

## What is the release version with BETA

The release version with the word "beta" means that some modules of this version have not yet been thoroughly tested. In
addition, some of the support for the beta version may not be complete. But rest assured, I will test after the beta
version is released, and release a stable version at an appropriate time.

