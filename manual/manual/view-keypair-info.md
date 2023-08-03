# View Key Pair Details

You can view the details of a key pair by right-clicking on the key pair in the
key toolbox or key management interface and selecting "Show key details".

This section may include a brief introduction to gpg-related concepts and could
be relatively long.

Below is a screenshot of a friend's public key that I obtained from the key
server.

![image-20220110185144734](https://www.bktus.com/wp-content/uploads/2023/08/image-20220110185144734.png)

And here is a randomly generated private key. The most significant difference
between this and the previous key is that the key pair with only the public key
is used for encryption only, but if you possess the private key, you can perform
more actions (it also depends on your algorithm; DSA can only be used for
signatures).

![image-20220110185215204](https://www.bktus.com/wp-content/uploads/2023/08/image-20220110185215204.png)

## General Info

This interface provides some useful information to help you manage your key pair
properly.

### Owner

This section enables you to know the owner of this key pair. This information is
not fixed and can be changed. You can create a new UID in the UID section and
set it as the primary UID to change it.

According to the OpenPGP protocol, this part is divided into Name, Email, and
Comment.

![image-20220110185740491](https://www.bktus.com/wp-content/uploads/2023/08/image-20220110185740491.png)

### Primary Key

This part is the information of the primary key of the key pair. The primary key
is very crucial because without it, you cannot perform related management
operations like adding and revoking sub-keys (similar to not being able to open
the key ring). Let's introduce the information of the primary key separately
below. If you want to learn more, see the [Basic Concepts](../basic-concepts.md)
section.

The absence of the master key means that the private key of the master key does
not exist, but this doesn't mean that neither the public key nor the private key
exists. Please remember: Each subkey and primary key consist of a pair of public
and private keys.

![image-20220110185819775](https://www.bktus.com/wp-content/uploads/2023/08/image-20220110185819775.png)

#### Key ID

This is the unique identifier of the key, which is fixed and unchanging. Note
that this key ID is the key ID of the primary key. The key ID is uniquely
determined after the key is generated. Compared with the fingerprint, the key ID
is shorter and more user-friendly.

#### Algorithm

This refers to the algorithm used for key generation. This also pertains to the
generation algorithm of the primary key. The generation algorithm determines the
properties and capabilities of the key. Algorithms such as RSA can be used for
encryption and signature, but DSA can only be used for signature. However, the
DSA key length can be shorter.

#### Key Size

This is the length of the primary key. Generally, the longer the key, the harder
it is to crack the ciphertext. But at the same time, it takes more time for a
single operation. Generally speaking, a length of 2048 bits is safe enough (this
refers to the key generated using the RSA algorithm).

#### Normal Usage

This refers to what the key pair can conceptually be used for, including the
conceptual usage of the primary key and sub-key. When the primary key or subkey
generation can be used to sign, but it has already expired or does not exist,
the signature usage will still be displayed here.

#### Actual Usage

This is the actual usage of the primary key and all subkeys, which is the union
of their usage. If there is only one primary key in the key pair that can be
used for signing, but this primary key does not exist, then the signature usage
will not appear here, only in Normal Usage. In addition, when there is only one
subkey that can be used for signing, if it has expired, the signature purpose
will not be displayed here.

#### Expires on

This is the expiration time of the primary key. When the primary key expires, it
will become invalid and you can't use it for any operation. In addition, the
subkeys in the key pair will also be unavailable. Fortunately, you can change
the expiration time of the primary key at any time, or even set it to never
expire. The prerequisite for this is that the primary key exists in the key
pair.

#### Last Update

This is the time when the content of the key pair was last updated. Operations
such as adding a UID or subkey will modify the content of the key pair.

#### Secret Key Existence

This indicates whether the actual content of the primary key exists. When the
primary key does not exist, if there are still available subkeys in the key
pair, the key pair can still be used for normal operations. However, in the
above case, the content of the key pair cannot be modified (that is, operations
such as adding UID or subkey cannot be performed), and the key pair cannot sign
other key pairs.

### Fingerprint

![image-20220110190639502](https://www.bktus.com/wp-content/uploads/2023/08/image-20220110190639502.png)

The fingerprint of the key pair is used for humans to quickly compare whether
the key pair is the expected key pair. This field is unique for all keys in the
world. You can certainly do this with the key ID mentioned above.

This also refers to the fingerprint of the primary key.

## UID Info

User ID (UID) is used to identify a key, mainly for human identification. It's
similar to a name tag that accompanies a key ring, indicating who the key ring
belongs to. By looking at the UID, users can get a rough idea of whether a key
pair is what they expected. However, for accurate identification, fingerprints
or key IDs should be compared. A key can have multiple UIDs, but a key pair can
only have one primary UID, which is always listed first in the interface.

![image-20220110190943207](https://www.bktus.com/wp-content/uploads/2023/08/image-20220110190943207.png)

UID has three elements: Name, Email, Comment. The name should be at least five
characters long, and the email should conform to the format. The rules for
comments are relatively loose.

### Signature of UID

The lower section of the interface displays the signature of the selected User
ID (UID), not the checked one. This is a key trust system. When someone receives
your public key, they can use their private key to sign your nameplate,
indicating their recognition of your public key. Afterward, they can upload the
keyring with their signature to the keyserver. If many people do the same, the
public key on the keyserver will have numerous signatures, making it
trustworthy.

You can also use the primary key of another key pair to sign a UID. Generally, a
primary UID of a key pair with many valid signatures is considered more
trustworthy.

## Subkey Info

The sub-key mechanism is a crucial feature of GPG that improves both flexibility
and security. However, it also introduces some complexity, which can be
challenging for beginners. For a basic understanding, consider the following
points:

- A key pair can be likened to a key ring, comprising a primary key (a pair of
  public and private keys) and multiple subkeys (or none).
- Each subkey and primary key consists of a pair of public and private keys.
- The subkey can perform related operations (such as signing, encryption) in the
  absence or unavailability of the primary key.
- The functions of subkeys can overlap, and when both subkeys can be used for
  signing, the earlier one is selected.
- Subkeys can use more algorithms than the primary key, but usually have the
  same effect on daily operations.
- The disclosure of a subkey only affects that subkey, while the disclosure of
  the primary key endangers the entire key pair.

The primary key and all subkeys in the key pair are displayed on the interface.
Some information about the key is also listed below.

### Key In smart card

Whether a key is in the smart card refers to whether the key is moved to the
smart card. Moving the key to the smart card changes the structure of the key
and is irreversible.

### Operations

In this column, what you can do differs for a key pair that only has a public
key and a key pair that includes a private key.

Here's what you can do with a public key-only key pair:

![image-20220110193420845](https://www.bktus.com/wp-content/uploads/2023/08/image-20220110193420845.png)

And here's what you can do with a key pair that includes a private key:

![image-20220110193555076](https://www.bktus.com/wp-content/uploads/2023/08/image-20220110193555076.png)

These operations will be explained in detail throughout the documentation.