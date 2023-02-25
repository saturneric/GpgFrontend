# Generate Key Pair & Subkey

Sure, let's go through the process of generating a key pair and subkeys.

To generate a key pair using GpgFrontend, follow these steps:

1. Open GpgFrontend and click on the "Generate Key" button.
2. Fill in the required information, such as your name and email address.
3. Choose the type of key you want to generate (RSA or ECC).
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

![GIF](https://github.com/saturneric/Blob/blob/master/gif/generate-key-pair.gif?raw=true)

### Name & Email & Comment

The three fields, including name, email, and comment, are used to help users
differentiate this key pair from other key pairs they may have. Among these
three options, name and email are mandatory, while comment is optional.

It is important to note that the name should be at least 5 characters long, and
the email should follow the correct format (no actual email account is
required).

![uid](https://github.com/saturneric/Blob/blob/master/screenshots/uid.png?raw=true)

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

![expiration-date](https://github.com/saturneric/Blob/blob/master/screenshots/expriation-date.png?raw=true)

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

![keysize-algo](https://github.com/saturneric/Blob/blob/master/screenshots/keysize-algo.png?raw=true)

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

![usages](https://github.com/saturneric/Blob/blob/master/screenshots/usages.png?raw=true)

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

![GIF](https://github.com/saturneric/Blob/blob/master/gif/generate-subkey.gif?raw=true)

### Extra note

Below are some guidelines that may prove useful in comprehending the
aforementioned concepts and utilizing this tool accurately.

#### primary key & Subkey

A single primary key can be accompanied by several subkeys within a key pair.
This setup mitigates the risk of key leakage. In the event that a subkey is
exposed, it can be revoked promptly, thus limiting the damage. However, if the
primary key is leaked, the entire key pair becomes vulnerable, as the primary
key enables management of the entire key pair.

Hence, it is advisable to generate multiple subkeys upon creating the key pair
and store the master key separately in a secure location. This operation is not
yet supported by GpgFrontend; therefore, the gpg command must be used to carry
it out. However, GpgFrontend can detect and notify the user whether the primary
key exists or not, which is critical since certain actions, such as adding
subkeys or signing other keys, necessitate the presence of the primary key.

#### Some practical tips

Once generated, the primary key's intended purpose cannot be altered. However,
if a subkey has been designated for a specific purpose that the primary key
lacks, the key pair can still be utilized for activities related to that
purpose.

For instance, suppose you overlooked the encryption usage while creating the key
pair. In that case, generating a subkey and configuring it for encryption usage
would enable the key pair to perform encryption operations.
