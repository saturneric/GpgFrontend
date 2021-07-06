# Encrypt & Decrypt Text

To start encryption and decryption operations, you need to prepare a paragraph of text. In addition, you also need a key
pair that can perform encryption operations.How to generate such a key can be found in the chapter on generating a key
pair.

## Encrypt

### Only Encrypt

In this case, you only encrypt the ciphertext, and the ciphertext will be shorter.
Remember, when you need to use a key pair for encryption, make sure that the key you check can be used for encryption. 
This can be viewed in the usage column of the key toolbox (the letter E stands for encryption).


![GIF](https://github.com/saturneric/Blob/blob/master/gif/encrypt.gif?raw=true)

### Encrypt And Sign

By encrypting and signing at the same time, not only can the text be protected, but the recipient can also know that the
cipher text is from your hand.
A typical usage method is to check two key pairs, one is someone else's public key, which is used for encryption; the other is your own private key, which is used for signing.
If you do not check any key that can be used for signing, this is possible (equivalent to encryption only). The only difference is that you will receive a warning.

Since the signature requires the use of a private key, a password is required when the private key is protected by a
password. The following animation demonstrates how to enter the password at the same time.

To check whether the key can be used for signing, please check the usage column in the key toolbox on the right (letter S stands for signature).

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encrypt-sign.gif?raw=true)

### Encrypt With Multiply Keys

It is perfectly possible to encrypt with multiple key pairs, so that the ciphertext can be sent to multiple users.

## Decrypt

Paste the ciphertext you obtained directly, GpgFrontend will automatically select the appropriate key in the list for
decryption. If you don't find the right key to decrypt, don't worry GpgFrontend will remind you.
Whether a key pair can be used for decryption will not be displayed in the usage column. You only need to remember that a valid private key can be used to decrypt the ciphertext encrypted with the related public key. 

Regarding whether this key is a public key or a private key, you can check the type column in the key toolbox.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt.gif?raw=true)

## Decrypt And Verify

While decrypting, you can learn some information about the encryptor of the key (if you signed it during encryption).

Whether a key pair can be used for verification will not be displayed in the usage column, you only need to remember a valid public key can be used for verificationWhether a key pair can be used for verification will not be displayed in the usage column, you only need to remember a valid public key can be used for verification.
Regarding whether this key is a public key or a private key, you can check the type column in the key toolbox.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt-verify.gif?raw=true)
