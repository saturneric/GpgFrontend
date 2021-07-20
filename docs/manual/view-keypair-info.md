# View Key Pair Details

Right-click the key pair in the key toolbox or key management interface and click Show key details to view the
information about the key pair.

This part may involve a brief introduction to gpg-related concepts, and it will be relatively long.

![screenshot](https://github.com/saturneric/Blob/blob/master/screenshots/key-details.png?raw=true)

## General Info

### Owner

Through this section, you can understand the owner of this key pair. This information is not fixed and unchangeable. You
can create a new UID in the UID section and set it as the main UID to change it.

According to the OpenPGP protocol, this part is divided into Name, Email, and Comment.

![screenshot](https://github.com/saturneric/Blob/blob/master/screenshots/key-details-owner.png?raw=true)

### Master Key

This part is the information of the master key of the key pair. The master key is very important, because without it,
the key pair cannot perform related management operations such as adding and revoking sub-keys (similar to the key ring
cannot be opened). Let's introduce the information of the master key separately below.

![screenshot](https://github.com/saturneric/Blob/blob/master/screenshots/key-details-master-key.png?raw=true)

#### Key ID

The unique identifier of the master key is fixed and unchanging.

#### Algorithm

The encryption algorithm used by the master key.

#### Key Size

The length of the master key. 
It can be said that the longer the key, the harder it is to crack the ciphertext, but at the same time, the more time it takes for a single operation. 
Generally speaking, a length of 2048 bits is safe enough.

#### Normal Usage

What can the key pair conceptually be used for (including the conceptual usage of the master key and sub-key). When the
master key or subkey generation can be used to sign, but it has already expired or does not exist, the signature usage
will still be displayed here.

#### Actual Usage

The actual usage of the master key and all subkeys. It is the union of their usage. 
If there is only one master key in the key pair that can be used for signing, but this master key does not exist. Then the signature usage will not appear here, only in Normal Usage.
In addition, when there is only one subkey that can be used for signing, if it has expired, the signature purpose will not be displayed here.

#### Expires on

The expiration time of the master key. When the master key expires, it will be invalid. You cannot use it for any operation. In addition, the subkeys in the key pair will also be unavailable.
Fortunately, you can change the expiration time of the master key at any time, or even set it to never expire. The prerequisite for this is that the master key exists.

#### Last Update

The time when the content of the key pair was last updated. Operations such as adding a UID or subkey will modify the
content of the key pair.

#### Secret Key Existence

Shows whether the actual content of the master key exists. When the master key does not exist, if there are still
available subkeys in the key pair, the key pair can still be used for normal operations. However, in the above case, the
content of the key pair cannot be modified (that is, operations such as adding UID or subkey cannot be performed), and
the key pair cannot sign other key pairs.

### Fingerprint

![screenshot](https://github.com/saturneric/Blob/blob/master/screenshots/key-details-fingerprint.png?raw=true)

The fingerprint of the key pair is used for humans to quickly compare whether the key pair is the expected key pair.

### Operations

![screenshot](https://github.com/saturneric/Blob/blob/master/screenshots/key-details-opera.png?raw=true)

This column lists all available operations.

## UID Info

UID is used to identify the key, and this mechanism is mainly used to facilitate human identification. Humans can
roughly identify whether a key pair is what he expected by looking at the UID, but to accurately identify it,
fingerprints need to be compared. A key can have one or more UIDs. A key pair has one and only one main UID.

UID has three elements, Name, Email, Comment. The name must be greater than or equal to five characters, and the email
must conform to the format. Comment rules are relatively loose.

### Signature

You can use the master key of another key pair to sign a UID. When the master UID of a key pair has many valid
signatures attached, it will be more trustworthy than without a valid key pair.

## Subkey Info

The sub-key mechanism is an important content of gpg, which enhances flexibility and security, but also brings a certain
degree of complexity, making it difficult for beginners to understand.

In order to help you understand this concept and get a preliminary grasp, you only need to read the following points:

- A key pair can be compared to a keychain, with a master key and multiple subkeys (or no subkeys).
- The sub-key can do related operations (such as signing, encryption) when the master key is not present or cannot.
- The functions of the sub-keys can overlap. When both sub-keys can be used for signing, the earliest generated one is
  selected for this operation.
- The sub-key can use more algorithms than the master key, but generally they have the same effect on daily operations.
- The disclosure of the subkey only affects the subkey, and the entire key pair is in danger after the disclosure of the
  master key.
