# Fundamental Concepts

If you're unfamiliar with GPG or PGP, it would be beneficial to learn some
fundamental concepts before using GpgFrontend. This can help prevent potential
mistakes, such as accidentally sharing your private key.

## Essential Concepts

Before you start using GPG, you need to generate a key pair, analogous to a key
ring. Each key pair comprises at least two keys: a public key and a
corresponding private key. It is possible for a key pair to contain multiple
public keys and their associated private keys, which we'll discuss later.

The public key can be shared with others, allowing them to encrypt data they
want to send you. Conversely, the private key should remain confidential since
its exposure can compromise your encryption.

It's vital to understand that the public key is used for encryption, and the
ciphertext encrypted with your public key can only be decrypted using your key
pair's corresponding private key. This process is based on cryptographic
principles and is reliable unless your private key gets compromised. Similarly,
you can encrypt a message with your private key, and someone else can decrypt it
using your public key, establishing a signature verification mechanism.

While understanding the underlying cryptography of GPG isn't necessary for most
users, remembering these basic principles can be beneficial. 

## Multiple Pairs of Public and Private Keys in a Key Pair

A key pair always contains at least one public-private key pair, but there's no
maximum limit. It's possible to generate additional public and private subkeys
to add to the key pair. Each pair consists of one public key and one private
key, which we refer to as the primary key pair.

Users can specify the purpose of each subkey pair. For instance, the first pair
can be used for encryption and decryption, the second for signing and
verification, and so forth. It's important to define the intended usage when
generating subkeys.

The primary key is automatically generated when creating the key pair, and users
can add subkeys as needed. When generating the primary key or subkeys, users can
select algorithms such as RSA or DSA.

## The First Pair of Public and Private Keys in a Key Pair (Primary Key)

The initial public-private key pair (primary key) in a key pair is crucial as
all subsequent subkeys are tied to it. If the primary key is compromised, they
could generate subkeys based on that information and impersonate the key owner.

Therefore, it's crucial to keep the primary key's private key confidential,
while the public key can be shared. If the private key of the primary key is
compromised, it makes the entire key pair vulnerable and its use should be
immediately discontinued.