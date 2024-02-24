# Contributing to GpgFrontend

Thank you for considering contributing to GpgFrontend! As a community-driven
project currently maintained by a single individual, every contribution, no
matter how small, makes a significant difference. This guide is designed to
provide you with a clear pathway to contributing, whether you're submitting
changes via GitHub pull requests or sending git patches via email. Below, you'll
find the steps to set up your environment, make changes, and how to submit those
changes. Additionally, you'll find contact information for further assistance.

## Technical Requirements

To maintain the integrity and compatibility of GpgFrontend, please adhere to the
following technical standards for all contributions:

- **C++ Standard:** Contributions must comply with the C++17 standard. This
  requirement ensures that we leverage modern language features and maintain
  forward compatibility.
- **C Standard:** For code that involves C programming, adherence to the C99
  standard is required. This helps ensure our code takes advantage of more
  recent language features while maintaining compatibility with various
  platforms and compilers.
- **Compiler Compatibility:** Your code should compile successfully with both
  Clang and GCC compilers. This cross-compatibility is crucial for ensuring that
  GpgFrontend can be built on a variety of platforms and environments.
- **Third-Party Libraries:** Introducing third-party libraries should be done
  with caution. Any added library must be compatible with the GPL 3.0 license.
  Prior discussion with project maintainers about the necessity and implications
  of the new library is required.
- **Code Formatting:** Use our `.clang-format` and `.clang-tidy` configurations
  to format your code. Consistent code formatting aids in maintaining the
  readability and maintainability of the codebase.
- **Code Maintenance and Attribution:** Be aware that the project maintainer may
  edit your code to better fit the project or enhance compatibility. You are
  encouraged to include your name and contact information in the code comments
  for your contributions if you wish.

### Additional Standards to Consider

- **Static Analysis:** To ensure code quality and catch potential issues early,
  contributions should pass static analysis checks where applicable. Tools like
  Clang Static Analyzer or GCC's `-Wall -Wextra -pedantic` flags can be used to
  identify potential issues.
- **Unit Testing:** If your contribution adds new functionality or changes
  existing behavior, including unit tests to cover your changes is highly
  recommended. This helps ensure that your contributions do not inadvertently
  break existing functionality.
- **Documentation:** Update existing documentation or add new documentation as
  necessary to reflect your changes or additions to the project. Well-documented
  code is essential for future maintenance and for new contributors to
  understand the project.

## Getting Started

### Step 1: Set Up Your Environment

Make sure you have a configured Git environment. For GitHub contributions, fork
the repository and clone it locally. For email contributions, ensure Git is
installed on your machine.

For setting up local development Environment, you can refer to [this
section](setup-dev-env.md).

### Step 2: Making Changes

Create a new branch for your work, implement your changes while adhering to the
technical requirements and standards mentioned, and commit your changes with
clear, descriptive commit messages.

### Step 3: Submitting Contributions

#### Via GitHub Pull Request

Push your changes to your fork and submit a pull request to the original
repository. Ensure your pull request describes the changes made and the reason
for those changes.

#### Via Email with Git Patch

For email submissions, generate a git patch for your commits and send it to the
project's contribution email address. Make sure to include a detailed
description of your changes and the reasons for them in your email.

## Contact

If you have any technical questions or need assistance, refer to the Contact
document for the maintainer's email address. We are here to help and encourage a
collaborative development process.

## Conclusion

Your contributions are vital to the success and improvement of GpgFrontend. We
appreciate your efforts to adhere to these guidelines and look forward to your
innovative and high-quality contributions. Thank you for being a part of our
community.
