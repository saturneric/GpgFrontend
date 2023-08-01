# Sign & Verify Text

The process of signing and verifying is typically the inverse of the process of
encryption and decryption. When signing, the private key is used, and when
verifying, the public key is used. Similar to signing multiple names on a
document, multiple private keys can be selected when signing. However, there are
some notable differences. Digital signatures can be used to verify whether the
signature's content has been tampered with, while it is difficult to achieve the
same level of verification with handwritten signatures.

## Only Sign

By signing the text, you can show that you are the only and unchangeable
certification with this text. You can just sign the text without encrypting the
text like the following.

![Peek 2022-01-12 06-50](https://www.bktus.com/wp-content/uploads/2023/08/Peek-2022-01-12-06-50.gif)

To check whether the key can be used for signing, please check the usage column
in the key toolbox on the right (letter S stands for signature).

## Sign With Encrypt

You can also perform signing and encryption simultaneously by selecting both a
public key for encryption and your own private key for signing. This is a common
practice where you check two key pairs: one belonging to someone else for
encryption, and the other being your own private key for signing. If you do not
select any key for signing, encryption-only is possible, but you will receive a
warning. It is worth noting that combining signing and encryption provides an
additional layer of security, as it ensures the recipient that the message has
not been tampered with and that it came from the sender whose identity is
verified by the digital signature.

![Peek 2022-01-12
06-54](https://www.bktus.com/wp-content/uploads/2023/08/Peek-2022-01-12-06-54-16419417228411.gif)

## Verify

Once you have obtained a plaintext and its corresponding signature, you can
verify the signature using the public key of the signer. However, please note
that this form of signature is not suitable for use in emails as it can make the
email less readable.

![Peek 2022-01-12 06-56](https://www.bktus.com/wp-content/uploads/2023/08/Peek-2022-01-12-06-56.gif)

To verify a signature with text, you need to have the corresponding public key
for all included signatures. If a suitable public key for a signature is not
found locally during verification, GpgFrontend will remind you to import it.

![image-20220112070325556](https://www.bktus.com/wp-content/uploads/2023/08/image-20220112070325556.png)

## Verify With Decrypt

It is recommended to verify a ciphertext while decrypting it, regardless of
whether it has been signed by the encryptor or not. It is not possible to
determine from the format of the ciphertext whether it has been signed or not.
Therefore, it is a good habit to always use decryption operations with
verification when possible.

![Peek 2022-01-12 07-10](https://www.bktus.com/wp-content/uploads/2023/08/Peek-2022-01-12-07-10.gif)
