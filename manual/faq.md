# Frequently Asked Questions (FAQ)

## Understanding GpgFrontend

**What is GpgFrontend?** GpgFrontend is a user-friendly, cross-platform tool
designed to facilitate the use of OpenPGP encryption, making it easier for
individuals to protect their privacy and secure their communications.

**What can I do with GpgFrontend?** Beyond basic encryption and decryption,
GpgFrontend supports digital signatures to verify the integrity and origin of
messages. Users can manage and generate key pairs, encrypt files and emails, and
sign their communications for added security.

**How can I obtain and start using GpgFrontend?** You can download the latest
version of GpgFrontend from its GitHub repository. Visit [GpgFrontend's
Downloads Page](https://www.gpgfrontend.bktus.com/#/downloads) to find the most
recent release. Installation is straightforward: just follow the ReadME
instructions to set it up.

## OpenPGP and GnuPG Explained

**How do OpenPGP and GnuPG relate?** OpenPGP serves as a standardized protocol
for encrypting and decrypting data, which GpgFrontend supports. GnuPG, or GPG,
implements the OpenPGP standard, providing the necessary cryptographic
functions. GpgFrontend leverages GnuPG for operations like encryption,
decryption, and key management.

**Which operating systems does GpgFrontend support?** GpgFrontend is a
cross-platform application that supports Windows, macOS, and Linux, making it
accessible to nearly all users for their privacy and data protection needs.

## Troubleshooting GnuPG Installation Issues

**Encountering 'GnuPG not installed correctly'?** This issue typically arises
when GpgFrontend cannot locate GnuPG on your system. Here are steps to address
this based on your operating system:

### For macOS Users

- **Install GnuPG for OSX** from
  [here](https://sourceforge.net/p/gpgosx/docu/Download/), or install
  GpgFrontend using Homebrew with `brew install --cask gpgfrontend`.
- If GnuPG is installed in a custom location, please tell GpgFrontend where the
  gpgconf binary it is at GnuPG Controller.

### For Linux Users

- Install GnuPG via your package manager (apt, yum, etc.).
- If GnuPG is installed in a custom location, please tell GpgFrontend where the
  gpgconf binary it is at GnuPG Controller.

### For Windows Users

- The latest GpgFrontend versions include GnuPG. It's recommended to download
  the most recent GpgFrontend version.
- Alternatively, download GnuPG from
  [here](https://www.gnupg.org/ftp/gcrypt/binary/gnupg-w32-2.4.0_20221216.exe)
  and reinstall if necessary.

### Additional Assistance

- For more detailed guidance, refer to the quick start manual available at
  [Quick Start Guide](quick-start.md).

## Reporting Bugs and Contributing

**Found a bug?** If you encounter any issues with GpgFrontend, please report
them via the GitHub repository. You can also contact me directly if you're not
on GitHub; see the [Contact](contract.md) section for details.

**Interested in contributing?** Feel free to modify GpgFrontend's code and
submit a Pull Request with your enhancements. You can also send patches via
email if you prefer to contribute anonymously.

## Why the Need for GnuPG?

**Importance of Installing GnuPG** GpgFrontend itself does not handle direct
encryption or decryption; it requires GnuPG for these operations. This design
choice ensures higher security, allowing users to rely on their own verified
version of GnuPG.

## Understanding BETA Versions

**What does "BETA" signify in a release?** A "BETA" label indicates that the
version may not have undergone comprehensive testing, particularly for new
modules. While beta versions are carefully evaluated post-release, and stable
versions are published when ready, the aim is to avoid beta releases from
version 2.0.0 onwards, barring exceptional circumstances.

## Security and Privacy

**How does GpgFrontend ensure my communications are secure?** GpgFrontend uses
GnuPG to implement the OpenPGP standard, securing your data with robust
encryption algorithms to prevent unauthorized access. It supports public and
private key encryption methods, ensuring only intended recipients can decrypt
and read your messages.
