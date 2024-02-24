# Getting Started with GpgFrontend

Welcome to GpgFrontend, the cross-platform, OpenPGP encryption tool designed for
simplicity and security. This guide will walk you through the installation
process tailored to your operating system, ensuring you can start securing your
communications as quickly and efficiently as possible.

## Before You Begin: Prerequisites

**Note for Windows or macOS users who utilize Homebrew: You may skip this
prerequisites section.**

GpgFrontend leverages the robust functionalities of GnuPG for encryption,
decryption, and key management. It is crucial to have GnuPG installed on your
device to make full use of GpgFrontend. Starting from version 2.0, GnuPG
operates on modular components for enhanced functionality, requiring proper
integration with your system.

Be aware, due to GnuPG's inability to function within an App Sandbox,
GpgFrontend is not available through the Apple Store.

For Linux users, most contemporary distributions come with GnuPG 2.0
pre-installed. Check your GnuPG version with `gpg --version` in your terminal.
It is advisable to upgrade to GnuPG version 2.2 or later for optimal performance
and compatibility.

## Installation & Execution Guide

### For Windows Users

#### Portable Version (No Installation Required)

1. **Download** the portable version from [GpgFrontend's latest
   releases](https://github.com/saturneric/GpgFrontend/releases/latest), labeled
   `GpgFrontend-*******-windows-x86_64-portable.zip`.
2. **Extract** the contents of the downloaded ZIP file.
3. **Run** `GpgFrontend.exe` from the extracted `Program/bin` directory.

#### Installer Version

1. **Download** the installer from [GpgFrontend's latest
   releases](https://github.com/saturneric/GpgFrontend/releases/latest), named
   `GpgFrontend-*******-windows-x86_64-setup.exe`.
2. **Install** GpgFrontend by following the on-screen instructions. After
   installation, you can access GpgFrontend directly from your desktop.

### For macOS Users

GpgFrontend's macOS packages are Apple-verified, allowing straightforward
opening without additional permissions.

#### Using Homebrew Cask

For an effortless install or removal process, use Homebrew Cask:

1. Ensure **Homebrew** is installed.
2. **Install** GpgFrontend with the command `brew install --cask gpgfrontend`.
3. **Launch** GpgFrontend from your Launchpad.

#### Manual Installation from DMG

1. **Install GnuPG** via Homebrew or download it from [GPG for OS
   X](https://sourceforge.net/projects/gpgosx/files).
2. **Download** the `GpgFrontend-*******-macos-**.dmg` file from [GpgFrontend's
   releases](https://github.com/saturneric/GpgFrontend/releases). Select
   `x86_64` for compatibility with Intel and Apple Silicon chips.
3. **Mount** the DMG file and **run** GpgFrontend.
4. **Optional:** Drag GpgFrontend into your Applications folder for easy access.

### For Linux Users (AppImage)

AppImage simplifies software distribution by bundling applications and all
necessary libraries into a single, executable file, eliminating dependency
conflicts.

1. **Install GnuPG** if it's not already installed.
   - Debian/Ubuntu: `sudo apt update && sudo apt install gpg`
   - CentOS: `sudo yum install gnupg`
2. **Download** the AppImage from [GpgFrontend's
   releases](https://github.com/saturneric/GpgFrontend/releases), labeled
   `GpgFrontend-*******-linux-x86_64.AppImage`.
3. **Make it executable:** `chmod u+x ./GpgFrontend-***-linux-x86_64.AppImage`.
4. **Launch** the AppImage with a double-click or through the terminal.

### BSD Users (FreeBSD/OpenBSD)

Currently, there are no binary releases for BSD systems. However, GpgFrontend
can be compiled and run on BSD. Refer to the GitHub repository for build
instructions.

## Downloading from GitHub Releases

GpgFrontend is primarily distributed through GitHub Releases, ensuring
accessibility and eliminating the need for personal server management.

Upon navigating to [GpgFrontend's download
page](https://github.com/saturneric/GpgFrontend/releases/latest), you'll find
detailed information about the latest version, including new features and fixes.
Each release is signed for security verification purposes, allowing users with
heightened security needs to confirm the authenticity of the downloaded files.

Scroll down to the **Assets** section to select the appropriate version for your
operating system. The filenames include a unique identification number, crucial
for reporting any version-specific issues.

Files ending in 'signature' are GPG separate signatures, which can be verified
with GPG to ensure they are officially released and approved by me.

Select the version that suits your system or follows the provided instructions
to begin the download.

By following these steps, you'll be ready to use GpgFrontend, enhancing your
digital security with powerful encryption tools at your fingertips.

## Basic Operations with GpgFrontend

After successfully installing GpgFrontend, you're ready to dive into the world
of encryption and secure communication. This guide will walk you through some
fundamental operations to get you started.

### Generating Your First Key Pair

1. Open GpgFrontend: Launch the application.
2. Access Key Management: Navigate to the "Key Management" section.
3. Generate Key Pair: Look for an option to "Generate" a new key pair.
4. Fill in your details, such as name and email, and choose your desired key
   type and size. You can also add a comment if you wish.
5. Set an expiration date for the key, or choose 'Never Expire' if you prefer.
6. Create a passphrase for your key to ensure its security. Remember to use a
   strong passphrase that you won't forget.
7. Once all details are filled in, click 'OK' to generate your key pair.

### Encrypting

1. **Write a Message:** Start by writing a message in the main text area of
   GpgFrontend.
2. **Choose Recipient:** Select the public key of the message's recipient. If
   you're practicing, you can select your own public key.
3. **Encrypt:** With the recipient's public key selected, click on the "Encrypt"
   option. The text will be encrypted, making it readable only by the selected
   recipient.

### Decrypting

1. **Receive an Encrypted Message:** Copy the encrypted message into
   GpgFrontend's main text area.
2. **Decrypt:** Click on the "Decrypt" option. If the message was encrypted with
   your public key, you would need to enter the password for your private key to
   decrypt it.

### Signing

1. **Create a Message:** Type your message in the text area.
2. **Sign:** Choose the "Sign" option and select your private key. Enter your
   key's password to sign the message. This adds a digital signature that
   verifies you as the message's sender.

### Verifying

1. **Receive a Signed Message:** Paste the signed message into GpgFrontend.
2. **Verify:** Click on "Verify" to check the signature. If the signature
   matches the sender's public key, GpgFrontend will confirm the message's
   integrity and authenticity.

### Key Management and Sharing

#### Exporting and Sharing Your Public Key

1. Go to 'Manage Keys'.
2. Select your key pair and choose 'Export Key'.
3. Save your public key to a file or copy it to the clipboard to share it with
   others.

#### Importing a Public Key

To communicate securely with someone else, you need their public key. Click
'Import key'. You can import a key file or copy and paste the public key
directly. After importing, the public key will appear in your keyring, ready for
use.

### Exchanging Public Keys with Friends

1. **Export Your Public Key:** From the "Key Management" section, find the
   option to export your public key. Save it as a file or copy it to your
   clipboard.
2. **Share Your Public Key:** Send your public key to your friend via email,
   messaging app, or any secure channel you prefer.
3. **Import Your Friend's Public Key:** When your friend sends you their public
   key, import it into GpgFrontend using the "Import" option in the "Key
   Management" section.

By exchanging public keys, you establish a secure channel for encrypted
communication. Only the holder of the corresponding private key can decrypt
messages encrypted with a public key, ensuring privacy and security in your
communications.

## Conclusion

GpgFrontend is a powerful tool for enhancing your digital security. By
generating key pairs, encrypting and decrypting messages, signing documents, and
exchanging public keys, you can safeguard your communications in a world where
privacy is increasingly precious. As you become more familiar with these
operations, explore further features and settings within GpgFrontend to tailor
your security practices to your needs. Remember, the cornerstone of digital
security is practicing safe key management and password hygiene. Welcome to the
secure side!
