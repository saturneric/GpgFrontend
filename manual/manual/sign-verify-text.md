# Signing & Verifying Text

The process of signing and verifying generally corresponds to the process of
encryption and decryption. For signing, the private key is used, and for
verifying, the public key is utilized. Similar to signing a document with
multiple signatures, you can choose multiple private keys during the signing
process. However, there are key differences. Digital signatures enable us to
check whether the signed content has been altered, which is difficult to achieve
with handwritten signatures.

## Signature Only

By signing the text, you establish that you are the sole and unalterable
authority for this text. You can simply sign the text without encrypting it as
follows:

![Peek 2022-01-12
06-50](https://cdn.bktus.com/wp-content/uploads/2023/08/Peek-2022-01-12-06-50.gif)

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
06-54](https://cdn.bktus.com/wp-content/uploads/2023/08/Peek-2022-01-12-06-54-16419417228411.gif)

## Verification

Once you have a plaintext and its corresponding signature, you can verify the
signature using the signer's public key. However, this type of signature isn't
suitable for emails as it can make the email less readable.

![Peek 2022-01-12
06-56](https://cdn.bktus.com/wp-content/uploads/2023/08/Peek-2022-01-12-06-56.gif)

To verify a signature with text, you need to have the corresponding public key
for all included signatures. If a suitable public key for a signature isn't
found locally during verification, GpgFrontend will prompt you to import it.

![image-20220112070325556](https://cdn.bktus.com/wp-content/uploads/2023/08/image-20220112070325556.png)

## Verification with Decryption

When decrypting a ciphertext, it's advised to verify it simultaneously,
regardless of whether the encryptor signed it or not. It's impossible to
determine from the ciphertext's format if it has been signed. Therefore, it's a
good habit to always perform decryption operations with verification whenever
possible.

![Peek 2022-01-12
07-10](https://cdn.bktus.com/wp-content/uploads/2023/08/Peek-2022-01-12-07-10.gif)
