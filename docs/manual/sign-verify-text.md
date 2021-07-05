# Sign & Verify Text

## Only Sign

By signing the text, you can show that you are the only and unchangeable certification with this text. You can just sign
the text without encrypting the text like the following.

To check whether the key can be used for signing, please check the usage column in the key toolbox on the right (letter S stands for signature).

![GIF](https://github.com/saturneric/Blob/blob/master/gif/sign.gif?raw=true)

## Sign With Encrypt

You can also encrypt this short text while signing, which is equivalent to signing while encrypting.
A typical usage method is to check two key pairs, one is someone else's public key, which is used for encryption; the other is your own private key, which is used for signing.
If you do not check any key that can be used for signing, this is possible (equivalent to encryption only). The only difference is that you will receive a warning.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/sign-encrypt.gif?raw=true)

## Verify

After obtaining a plaintext and its signature, you can verify the signature.
Whether a key pair can be used for verification will not be displayed in the usage column, you only need to remember a valid public key can be used for verificationWhether a key pair can be used for verification will not be displayed in the usage column, you only need to remember a valid public key can be used for verification.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/verify.gif?raw=true)

## Verify With Decrypt

After obtaining a ciphertext, you can try to verify it while decrypting it. This is a good habit regardless of whether
the encryptor has signed in advance.
Whether a key pair can be used for decryption will not be displayed in the usage column. You only need to remember that a valid private key can be used to decrypt the ciphertext encrypted with the related public key. 

![GIF](https://github.com/saturneric/Blob/blob/master/gif/verify-decrypt.gif?raw=true)
