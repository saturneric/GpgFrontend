# Signing & Verifying Text

Digital signatures, much like their analog counterparts, serve as a method for
asserting the authenticity and integrity of a digital document or message.
However, unlike traditional signatures, digital signatures offer a much higher
level of security, making it possible to ascertain not only the identity of the
signer but also whether the content has been tampered with since it was signed.

The foundation of digital signing and verification lies in the field of public
key cryptography, a cornerstone of modern secure communication. This system
relies on two keys: a private key, which is kept secret by the owner, and a
public key, which can be shared with anyone. To sign a document, the signer uses
their private key to generate a digital signature on the document. This
signature is unique to both the document and the private key, ensuring that any
changes made to the document after it has been signed can be detected.

Verification, on the other hand, requires the corresponding public key. When a
document is received along with its digital signature, the recipient can use the
signer's public key to verify the signature. This process checks that the
signature matches the document and was created with the private key
corresponding to the public key. If the document has been altered after signing,
the verification will fail, alerting the recipient to the tampering.

One of the advantages of digital signatures is the ability to use multiple
private keys for signing a document, similar to having a document signed by
multiple parties. Each signer uses their private key to sign the document, and
each signature can be independently verified with the corresponding public key.
This method is particularly useful in scenarios requiring the approval or
authorization of multiple entities.

Digital signatures are a critical component of secure communications, providing
assurances of authenticity, integrity, and non-repudiation. Non-repudiation
means that a signer cannot later deny the authenticity of the signature on a
document they signed. This is especially important in legal, financial, and
sensitive communications, where trust and authenticity are paramount.

Tools like GpgFrontend facilitate the process of creating and verifying digital
signatures in a user-friendly manner. GpgFrontend is built on top of the OpenPGP
standard, which is a widely accepted protocol for encryption and digital
signatures. The tool allows users to easily manage their encryption keys, sign
documents, and verify the signatures of received documents, thereby enhancing
the security and trustworthiness of digital communications.

In summary, digital signing and verification through tools like GpgFrontend
leverage public key cryptography to ensure the security and integrity of digital
communications. By enabling users to sign documents with their private keys and
allowing others to verify those signatures with corresponding public keys,
digital signatures provide a robust mechanism for authenticating the origin and
integrity of digital documents, far surpassing the capabilities of traditional
handwritten signatures.

## Signature Only

By signing the text, you establish that you are the sole and unalterable
authority for this text. You can simply sign the text without encrypting it as
follows:

![Peek 2022-01-12
06-50](https://image.cdn.bktus.com/i/2023/11/16/9c95a381-52b9-4d2b-c21d-38fdc6cbc76d.gif)

To check whether a key can be used for signing, please review the 'Usage' column
in the key toolbox on the right (the letter 'S' stands for signature).

## Signature with Encryption

You also have the option to sign and encrypt at the same time by choosing a
public key for encryption and your private key for signing. This is a common
practice where you select two key pairs: one belonging to someone else for
encryption, and your private key for signing. If you don't select a key for
signing, only encryption is possible, but you will receive a warning. It's worth
noting that combining signing with encryption provides an additional layer of
security as it assures the recipient that the message hasn't been altered and it
came from the sender whose identity is verified by the digital signature.

![Peek 2022-01-12
06-54](https://image.cdn.bktus.com/i/2023/11/16/fd98e968-5e59-7bee-abea-99ab234be7a6.gif)

## Verification

Once you have a plaintext and its corresponding signature, you can verify the
signature using the signer's public key. However, this type of signature isn't
suitable for emails as it can make the email less readable.

![Peek 2022-01-12
06-56](https://image.cdn.bktus.com/i/2023/11/16/fbde7130-72c3-1fce-8366-47643fc0e804.gif)

To verify a signature with text, you need to have the corresponding public key
for all included signatures. If a suitable public key for a signature isn't
found locally during verification, GpgFrontend will prompt you to import it.

![image-20220112070325556](https://image.cdn.bktus.com/i/2023/11/16/5ab80063-dbf7-0394-5c44-4c23f7b4702b.webp)

## Verification with Decryption

When decrypting a ciphertext, it's advised to verify it simultaneously,
regardless of whether the encryptor signed it or not. It's impossible to
determine from the ciphertext's format if it has been signed. Therefore, it's a
good habit to always perform decryption operations with verification whenever
possible.

![Peek 2022-01-12
07-10](https://image.cdn.bktus.com/i/2023/11/16/9e06ce22-f98d-47f1-ea76-e4e23b6dd32d.gif)
