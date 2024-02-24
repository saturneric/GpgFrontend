# Generate Key Pair & Subkey

Sure, let's go through the process of generating a key pair and subkeys.

To generate a key pair using GpgFrontend, follow these steps:

1. Open GpgFrontend and click on the "Generate Key" button.
2. Fill in the required information, such as your name and email address.
3. Choose the type of key you want to generate (RSA, DSA or ECC).
4. Set the key size and expiration date, if desired.
5. Create a passphrase to protect your private key.
6. Click "Generate" to create your key pair.

Once your key pair is generated, you can add subkeys to it by following these
steps:

1. Select the key pair you want to add a subkey to.
2. Click on the "Add Subkey" button.
3. Choose the type of subkey you want to add (encryption, signing,
   authentication, or all).
4. Set the subkey size and expiration date, if desired.
5. Create a passphrase to protect your subkey.
6. Click "Add" to create your subkey.

You can add multiple subkeys to a key pair, each with their own specific
purposes. This allows you to have more control over your key pair's security and
usage.

## Generate Key Pair

You can quickly understand the process of generating a key pair by watching the
following animation.

![GIF](https://image.cdn.bktus.com/i/2023/11/16/711f0379-eea6-ee25-2072-8e77d07d2ad5.gif)

### Name & Email & Comment

The three fields, including name, email, and comment, are used to help users
differentiate this key pair from other key pairs they may have. Among these
three options, name and email are mandatory, while comment is optional.

It is important to note that the name should be at least 5 characters long, and
the email should follow the correct format (no actual email account is
required).

![uid](https://image.cdn.bktus.com/i/2023/11/16/3ad515e0-6d9e-6507-552c-55101da16836.webp)

### Expiration Date

Setting an expiration date for the key pair is a way to limit the validity of
the key over time. Once the expiration date is reached, the key can still be
used, but its operations, especially signature operations, will be considered
invalid. By default, GpgFrontend suggests setting the expiration date to two
years after generation, but you can also choose to check the "Never expire"
checkbox to make the key pair permanent.

It's important to note that this option can be changed at any time after
generation, even after the expiration date has passed, as long as the primary
key still exists.

![expiration-date](https://image.cdn.bktus.com/i/2023/11/16/ce9b446d-a7a0-2944-b8e4-3517c0d3a861.webp)

### Key Size & Algo

Setting an expiration date for the key pair is a way to limit the validity of
the key over time. Once the expiration date is reached, the key can still be
used, but its operations, especially signature operations, will be considered
invalid. By default, GpgFrontend suggests setting the expiration date to two
years after generation, but you can also choose to check the "Never expire"
checkbox to make the key pair permanent.

It's important to note that this option can be changed at any time after
generation, even after the expiration date has passed, as long as the primary
key still exists.

![keysize-algo](https://image.cdn.bktus.com/i/2023/11/16/4ce5ecfa-7ad0-7a81-cbe1-2ea93f7872ea.webp)

### Passphrase

Setting a password to protect the primary key is crucial in case of a security
breach. If the "Do not set password" checkbox is unchecked, you will be prompted
to enter a password during the key pair generation process. Follow the prompts
to set the password. Once the password is set, whenever you need to use the
primary key for an operation, you will need to enter the password to unlock it
(some systems have a password manager to automate this process).

However, you can also check the "Do not set password" checkbox to skip setting a
protection password for the primary key. But this is not recommended due to
security concerns.

### Usage

When generating a key pair, you can specify the usage for the first subkey,
which is the primary key. There are four options:

![usages](https://image.cdn.bktus.com/i/2023/11/16/f9bae59d-9181-2cb8-53a6-b51c0698c613.webp)

- Encryption: Once generated, this key can be used for encryption purposes.

- Signing: Once generated, this key can be used for signature purposes.

- Certification: This key can be used to certify or verify other keys. Only the
  primary key can have this usage.

- Authentication: This key can be used for authentication purposes, such as with
  SSH keys.

The third of these four uses (authentication purposes) can only be owned by the
primary key. In addition, some usages are not available when using certain
algorithms for encryption. For example, when the DSA algorithm is selected, the
encryption uses are disabled.

## Generate Subkey

It is possible to append subkeys to an existing key pair. The subkey does not
require the input of a name, email, or comment, as the remaining steps are
essentially identical to those for generating a key pair.

![Generate Subkey](https://image.cdn.bktus.com/i/2023/11/16/4871ee77-5da5-5473-a2be-2d9c29d6b842.gif)

### Extra note

Below are some guidelines that may prove useful in comprehending the
aforementioned concepts and utilizing this tool accurately.

#### Understanding Primary Keys and Subkeys

In the realm of cryptography, key management plays a crucial role in ensuring
data security. A key pair consists of a primary key and one or more subkeys,
each serving distinct functions yet working together to secure and manage
digital identities and communications. This structure not only enhances security
but also provides flexibility in key usage and management.

#### The Role of Primary Key and Subkeys

- **Primary Key**: The primary key is the cornerstone of your cryptographic
  identity. It is used for identity verification, which includes signing other
  keys to establish trust. The primary key's signature on a subkey validates the
  subkey's association with the identity of the primary key holder.

- **Subkeys**: Subkeys are associated with the primary key and are used for
  encryption and signing documents or messages. Subkeys can be thought of as
  extensions of the primary key, each designated for specific tasks. This
  separation of duties allows for greater security and operational flexibility.
  For example, you can have separate subkeys for signing and encryption.

#### Advantages of Using Subkeys

1. **Enhanced Security**: By using subkeys for day-to-day operations, you
   minimize the risk associated with key exposure. If a subkey is compromised,
   it can be revoked without affecting the primary key or other subkeys, thereby
   limiting the potential damage.

2. **Operational Flexibility**: Subkeys allow for specific roles (e.g., signing,
   encryption) to be isolated. This means you can renew or revoke subkeys as
   needed without disrupting the overall cryptographic setup.

3. **Convenient Key Rotation**: Regularly updating keys is a best practice in
   cryptography. Subkeys make it easier to rotate keys for signing and
   encryption without needing to re-establish the primary key's trust
   relationships.

#### Managing Primary Keys and Subkeys

- **Secure Storage**: The primary key should be stored in a highly secure
  location, preferably offline or in a hardware security module (HSM), to
  prevent unauthorized access. This is because the loss or compromise of the
  primary key jeopardizes the entire cryptographic framework.

- **Key Generation and Maintenance**: While tools like GpgFrontend provide
  user-friendly interfaces for managing keys, they may lack support for advanced
  operations like generating multiple subkeys. Therefore, using the command-line
  `gpg` tool for such tasks is advisable. Despite this limitation, GpgFrontend
  can play a critical role in monitoring the presence of the primary key, which
  is essential for certain operations like adding subkeys or signing other keys.

- **Revocation and Renewal**: Prepare revocation certificates for your primary
  key and subkeys in advance. In case of key compromise or expiration, these
  certificates allow you to invalidate the keys, informing others in your trust
  network not to use them anymore.

#### Practical Tips for Effective Key Management

- **Purpose-Specific Subkeys**: If your primary key was not generated with
  certain capabilities (e.g., encryption), you can create a subkey with the
  required functionality. This allows the key pair to be used for the intended
  cryptographic operations without regenerating the primary key.

- **Multiple Subkeys for Different Devices**: For users operating across
  multiple devices, generating separate subkeys for each device can enhance
  security. If one device is compromised, only the subkey on that device needs
  to be revoked, leaving the others unaffected.

- **Backup and Recovery**: Regularly back up your key pair, including the
  primary key and all subkeys. Secure backups ensure that you can recover your
  cryptographic capabilities even in the event of hardware failure or data loss.

In summary, understanding and implementing a robust key management strategy,
with a clear distinction between primary keys and subkeys, is essential for
maintaining the integrity and security of cryptographic operations. By adhering
to best practices for key usage, storage, and renewal, users can safeguard their
digital identities and ensure the confidentiality and authenticity of their
communications.
