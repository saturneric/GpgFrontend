# Setting Up Your Local Development Environment

Creating a local development environment that mirrors the GitHub Actions
workflow ensures consistency between local development and continuous
integration builds. This guide leverages the steps defined in our GitHub Actions
workflow to help you set up a similar environment on your local machine. By
following these steps, you'll be able to compile, build, and test the project in
an environment closely resembling our CI pipeline, minimizing integration
issues. The exact commands and environment configurations used during the
compilation are documented within the project's `.github/workflow/release.yml`
file.

## Prerequisites

- **Git:** Installed and configured on your system.
- **Compilers:** GCC and Clang for cross-compatibility.
- **CMake:** For generating build files.
- **Qt6:** If working on a project that utilizes Qt for its GUI.

## Environment Setup Steps

### Clone the Repository

```bash
git clone https://github.com/saturneric/GpgFrontend.git
cd GpgFrontend
```

### Configure Git Line Endings

This step ensures that line endings are consistent across different operating
systems.

- **For Windows:**

```bash
git config --global core.autocrlf false
git config --global core.eol lf
```

- **For macOS:**

```bash
git config --global core.autocrlf false
git config --global core.eol lf
```

### Install Dependencies

- **On Ubuntu 20.04:**

```bash
sudo apt-get update
sudo apt-get install -y build-essential binutils git autoconf automake gettext texinfo gcc g++ ninja-build libarchive-dev libssl-dev libgpgme-dev
```

- **On macOS (11 and 12):**

```bash
brew install cmake autoconf automake texinfo gettext openssl@3 ninja libarchive gpgme
brew link --force openssl@3

```

- **For Windows (via MSYS2):** Set up MSYS2 according to its documentation and
  install the necessary packages:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-cmake autoconf automake make texinfo mingw-w64-x86_64-qt6 libintl msys2-runtime-devel gettext-devel mingw-w64-x86_64-ninja mingw-w64-x86_64-gnupg mingw-w64-x86_64-libarchive
```

### Install Qt6 (if applicable)

Use the Qt online installer or your package manager to install Qt6 and the
required modules for your project.

### Build Third-Party Libraries (if needed)

Follow the project's documentation to clone and build necessary third-party
libraries such as `libgpg-error`, `libassuan`, and `GpgME`. Use the same
commands as specified in the GitHub Actions workflow, adapted for your local
environment.

### Configure and Build the Project

- **For Linux and macOS:**

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

- **For Windows (via MSYS2):**

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j$(nproc)
```

### Running Tests

After building, run the project's tests to verify everything is working as
expected.

## Notes

- Adjust the build type (`Release`, `Debug`, etc.) as needed.
- Replace project-specific commands and dependency installation commands based
  on your project's requirements.
- For macOS, additional steps for code signing and notarization are required only
  for distribution.

By closely following the GitHub Actions workflow for local setup, you're
creating a development environment that minimizes surprises during the
integration and deployment phases.
