# Encrypt & Decrypt Text

To start encryption and decryption operations, you need to prepare a paragraph of text. In addition, you also need a key
pair that can perform encryption operations.How to generate such a key can be found in the chapter on generating a key
pair.

## Encrypt

### Only Encrypt

In this case, you only encrypt the ciphertext, and the ciphertext will be shorter.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encrypt.gif?raw=true)

### Encrypt And Sign

By encrypting and signing at the same time, not only can the text be protected, but the recipient can also know that the
cipher text is from your hand.

Since the signature requires the use of a private key, a password is required when the private key is protected by a
password. The following animation demonstrates how to enter the password at the same time.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encrypt-sign.gif?raw=true)

### Encrypt With Multiply Keys

It is perfectly possible to encrypt with multiple key pairs, so that the ciphertext can be sent to multiple users.

## Decrypt

Paste the ciphertext you obtained directly, GpgFrontend will automatically select the appropriate key in the list for
decryption. If you don't find the right key to decrypt, don't worry GpgFrontend will remind you.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt.gif?raw=true)

## Decrypt And Verify

While decrypting, you can learn some information about the encryptor of the key (if you signed it during encryption).

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt-verify.gif?raw=true)
