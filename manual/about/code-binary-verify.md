# Code & Binary Verify

Some users of this software have relatively high requirements for security, so here are instructions on how to verify
whether the code and executable files have been tampered with.If you find that the signature is incorrect, please stop
using it immediately.

## Code compilation

When you see the document, every binary file version released on github is automatically compiled after submission using **Github Actions**. The generation of the final code is completely done by Github Actions. I will not compile the code on my local machine, but may use my machine to sign the packaged code. The code for all release versions comes from the main branch of the Github repository.

All commands and environment configuration at compile time can be consulted in the project
code(`.github/workflow/release.yml`).

## Introduction of third-party libraries

For safety reasons, GnuPG will not be available in the major releases of GpgFrontned. GpgFrontnend will not use insecure or non-open source third-party libraries. The compiled version of the third-party library comes from the public code repository.

## Verify Code

If you have the need, interest or time, you are welcome to review the code that I use and write. When you have any
questions, please feel free to [Contact ME](../contract.md).

I will sign most new git commit. The key I use is the key that I use to perform Git related operations:
Saturneric\<eric@bktus.com\>. It will be given at the end of this description. For information about signing git commits, you
can see it on the github interface.

The fingerprint of the key is now given:

```text
E3379489C39B7270E70E2E303AAF1C64137CEE57
```

## Verify Release Binary Verify

### ZIP package Sign

Starting from version 1.0.5, I will use the Gpg key to sign the ZIP package that contains the binary executable file.
Each ZIP compressed package will be accompanied by a signature file in the Release (end with sig suffix). You can use
the gpg command tool to verify them before use. 

The key used for binary signature is Saturneric\<eric@bktus.com\>. This is my official key and will be given below.

The fingerprint of the key is now given:

```text
E3379489C39B7270E70E2E303AAF1C64137CEE57
```

## Check on About Interface

There is an About interface under the help option in the top menu.You can check the version and platform of this binary
file. More importantly, you can view the github repository branch and commit hash code corresponding to the source code
used for compilation of this binary version on the second line.

![About Interface](https://image.cdn.bktus.com/i/2023/11/16/b460a4ce-6d9e-b3e1-236f-92d97ad5f87e.webp)

## Pubkey Mentioned Above

#### Fingerprint

```text
E3379489C39B7270E70E2E303AAF1C64137CEE57
```

#### Pubkey Content (OpenPGP)

```text
-----BEGIN PGP PUBLIC KEY BLOCK-----

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
hHHFn9+/wu20
=bgvm
-----END PGP PUBLIC KEY BLOCK-----

```