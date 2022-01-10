# Generate Key Pair & Subkey

For GpgFrontend, to understand the key, you must first understand two concepts: key pair and subkey.

A key pair can be compared to a key ring. When it is generated, there is a key in the ring called the primary key. This
primary key can do the intended operation (encryption, decryption, etc.). At the same time, keep this in mind, only
through the primary key can you open the keychain to add new keys to it.

The sub-keys can be analogous to the keys you add to the key pair later, and each of them can independently perform
operations such as encryption and decryption. It can be considered that the primary key mentioned above is a special
subkey.

When there is no primary key in the key pair, you will not be able to open the key ring to add a new sub key, but you
can still use this sub key if it is changed to exist for your operations. This mechanism is very helpful to the security
of the key.

Let's see how to generate them next.

## Generate Key Pair

You can quickly grasp the operation of generating a key pair through the following animation.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/generate-key-pair.gif?raw=true)

### Name & Email & Comment

These three fields are used to facilitate people to distinguish this key pair from the card key pair. For these three
options, except for name and email, which are required, comments are optional.

In addition, the length of the name is required to be greater than 5 letters, as long as the email conforms to the
format (no actual existence is required).

![uid](https://github.com/saturneric/Blob/blob/master/screenshots/uid.png?raw=true)

### Expiration Date

You can set an expiration date for the key pair. After this date, the key may still be used normally, but the operation
it does will be logically invalid (especially for signature operations). GpgFrontend recommends and defaults this date
to two years later. If you wish, check the Never expire checkbox to make this key pair never expire.

But don't worry, you can change this option at any time after generation, even long after the expiration date (as long
as the primary key exists).

![expiration-date](https://github.com/saturneric/Blob/blob/master/screenshots/expriation-date.png?raw=true)

### Key Size & Algo

These two options are related. In general, different encryption algorithms have different optional lengths. GpgFrontend
will give you sufficient hints on the UI so that you will not go wrong.

Just remember that the larger the key length, the more secure, but it will be slower when performing operations.

![keysize-algo](https://github.com/saturneric/Blob/blob/master/screenshots/keysize-algo.png?raw=true)

### Passphrase

You can set a key to protect the primary key, which is very important when the primary key is leaked. When the Do not
set password check box is not checked, an interface for you to enter the password will pop up during the process of
generating the password. Just follow the prompts. After setting the password, when you need to use the primary key for
operation, you may enter the password to unlock it
(some systems have a password networkAccessManager to take over this process).

You can also check the checkbox to not set a protection password for the primary key, but due to security
considerations, this is not recommended.

### Usage

In the option of generating a key pair, you can specify the usage for the first subkey of the key pair, which is the
primary key. There are four options:

![usages](https://github.com/saturneric/Blob/blob/master/screenshots/usages.png?raw=true)

- Encryption: After generation, it can be used for encryption operations.

- Signing: After generation, it can be used for signature operations.

- Certification: Popular understanding can be used to unlock this key ring (key pair). Only the primary key can check
  this function.

- Authentication: It can perform authentication operations like SSH keys.

The third of these four uses (authentication purposes) can only be owned by the primary key. In addition, some usages
are not available when using certain algorithms for encryption. For example, when the DSA algorithm is selected, the
encryption uses are disabled.

## Generate Subkey

We can add sub-keys to the generated key pair. The subkey does not need to fill in the name, email and comment options.
The rest is basically the same as generating the key pair.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/generate-subkey.gif?raw=true)

### Extra note

Here are some tips you might want to know. These tips will help you better understand the above concepts and use this
tool correctly.

#### primary key & Subkey

A key pair can have multiple subkeys and one primary key. Using this design reduces the risk of key leakage. For subkey,
After the subkey is leaked, you can revoke it at any time to reduce the loss. However, when the primary key is leaked,
the entire key pair will be insecure (the popular reason is that the main force can be used to manage this key pair).

Therefore, the recommended approach is to generate multiple subkeys after creating the key pair, and export the master
key separately and store it in a safe place. This operation is not yet supported by GpgFrontend, you need to use the gpg
command to perform it. But gpgfrontend can identify and prompt the user whether the primary key exists or not. This is
very important, because some special operations (adding subkeys, signing other keys, etc.) cannot be performed without
the primary key.

#### Some practical tips

The purpose of the primary key cannot be changed after it is generated. If the primary key of this key pair does not
have a certain purpose, but a certain sub-key has this purpose, this key pair can still be used for operations
corresponding to this purpose.

For example, when you generated the key pair, you didn't check the encryption usage. Don't worry, you can generate a
subkey and check the encryption usage. In this way, this key pair can still perform encryption operations.

