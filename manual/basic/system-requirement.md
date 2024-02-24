# System Requirement

Before proceeding with the installation and usage of GpgFrontend, it's crucial
to understand the system requirements that ensure optimal performance. This
section provides comprehensive details about the necessary software
dependencies, hardware specifications, and the compatible operating systems.
Meeting these requirements will ensure a smooth, efficient experience while
using GpgFrontend.

Please read the following subsections carefully to confirm that your system
aligns with the recommended configurations.

## Hardware

While the specific hardware requirements largely depend on the size and
complexity of the data you're working with, we generally recommend:

A computer with at least 1 GB of RAM. However, 2 GB or more is preferable for
smoother performance. The majority of these resources are allocated to your
operating system, but around 100 MB of memory is needed to ensure the smooth
running of GpgFrontend. At least 200 MB of free disk space for software
installation. Additional space will be needed for ongoing work.

Please note, these requirements are intended to be guidelines rather than strict
rules. It's possible that GpgFrontend will work on lower-spec hardware, but for
optimal performance, the above specifications are recommended.

## Operating System

GpgFrontend is compatible with major operating systems including Linux, macOS,
and Windows. Specifically, it recommends Windows 7 and later, macOS 11 and
later, and Ubuntu 20.04 LTS or other equivalent Linux distributions.

## Software

To ensure GpgFrontend functions seamlessly, it relies on the following software
dependencies:

- **Qt Framework:** GpgFrontend is developed using the Qt framework to offer a
  rich user experience and cross-platform compatibility. The application
  includes:

  - **Qt6:** The primary build utilizes Qt6, ensuring a modern interface and
    robust performance. Qt6 is included in the release packages for Linux,
    macOS, and Windows, offering straightforward setup without additional
    installations.
  - **Qt5 Support for Windows:** Recognizing the need to accommodate users on
    older versions of Windows, GpgFrontend also provides a Qt5-based version.
    This variant ensures compatibility with earlier Windows environments,
    extending the tool's accessibility and usability.

- **GnuPG 2.2.0 or Higher:** GpgFrontend integrates with GnuPG for its
  cryptographic operations, including encryption, decryption, and digital
  signing. GnuPG (version 2.2.0 or newer) is a necessary component to leverage
  the full capabilities of GpgFrontend. Please note, GnuPG 1.x versions are not
  supported by GpgFrontend due to differences in functionality and support.
  Users are encouraged to use GnuPG 2.x to ensure compatibility and secure
  operations.

By catering to a wide range of operating systems and ensuring backward
compatibility with older Windows versions through Qt5 support, GpgFrontend
strives to be as inclusive and accessible as possible.

## Network

Although not necessary for basic operation, an active Internet connection may be
required for software updates and accessing online help resources.

Please note that these are the minimal requirements that we tested, and actual
requirements for your use case could be higher, especially for large datasets.
