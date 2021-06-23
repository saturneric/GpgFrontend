# Understand Interface

As a novice, you only need to quickly grasp the meaning of a few important parts of the page. You will gradually
discover other functions in the next exploration.

## Text Editor

In the text editing area, you can type text at will. Or create a new tab through the New option in the top file menu.
You can use any keyboard shortcut you use frequently to speed up your editing (copy, paste, etc). You can save your text
through the save option in the top file menu.

## Information Board

The result of your operation on the current tab page will be printed in the Information Board: success or failure. In
addition to the success and failure information, its text will also contain other information to help you understand the
details of your encryption, decryption, signature and other operations.

### Optional Actions Menu

There will also be a column of Optional Actions Menu below the Information Board. If there are other auxiliary
operations that can be done after your operation is completed (display more detailed information, upload encrypted text,
etc.), the entry points for these auxiliary operations will be displayed here.

## Key ToolBox

Here is a list of key pairs stored on your machine that can be used for Gpg operations, not including expired or revoked
key pairs.

### Usage

Most operations related to Gpg need to specify a key pair (such as encryption, decryption, signature, etc.). You can
select the check box in the first column of the table in the key toolbox to specify one or more keys for your Operation.

### Columns

It is important to understand this list. Now let me take you to understand it step by step.

- Select: Turn the checkbox in this column to let Gpg Frontend know that you specify the key of this row for your next
  operation.

- Type: See this column to let you know the type of key and whether the master key exists in your key pair.
    - `pub` means this is a public key, Can be used for encryption or verification operations.
    - `pub/sec` The key pair contains both public and private keys. It can be used for almost all operations(Need to
      combine the usage column to determine this).
    - `pub/sec#`  The key pair contains a public key and a private key, but the master key is not in the key pair. This
      shows that you will not be able to do some special (add subkeys, sign other key pairs, etc.)

- Name: The identity information of the key pair.

- Email Address: The identity information of the key pair.

- Usage: This determines which operations the key pair can use. Composed of four capital letters, each capital letter
  represents a usage.
    - `C` Certificate. Generally, the key pair that contains the master key will have this usage
    - `E` Encrypt. The key pair can be used for encryption operations.
    - `S` Sign. The key pair can be used for sign operations.
    - `A` Authenticate. The key pair can be used to perform operations like SSH authentication.

- Validity: One of the concepts of Gpg, simply put it represents the degree of trust in this key.

## Operations Bar

Here, you can perform corresponding operations by clicking the buttons above. For example, after typing text in a text
editor and setting the key you want to use in the key toolbox, you can click the encryption button to perform the
operation.

Some operations need to specify the key and some are not used, which will be explained in other corresponding parts of
the document.