# Interface Understanding

As a beginner, you're only required to swiftly comprehend a few crucial sections
of the page. The exploration that follows will gradually unveil additional
functionalities. Bear in mind that interfaces may vary across different
versions.

![Interface](https://image.cdn.bktus.com/i/2023/11/16/27c0bd12-8a1d-b9ae-2ecd-dbde5f96e36f.webp)

## Text Editor

The text editing zone allows you the liberty to input any desired text or
establish a new tab through the "New" choice in the file menu at the top. Moving
or closing tabs can be done with ease as per your needs.

Numerous operations can be performed on your text using options available in the
Operations Bar. Alternatively, you may utilize standard shortcuts like
Ctrl+C/V/S for copy, paste, and save operations, or even searching within the
text.

The edited text within the text box is encoded in UTF8 without any formatting.
This plain text format ensures that no message alteration leads to confusion.
While we have plans to introduce rich text editing in the future, the specifics
are still being deliberated.

### Large Text File Support

GpgFrontend accommodates opening larger files without hindrance. However, when
dealing with relatively large files, editing of the tab won't be feasible until
the entire file is loaded. During this time, despite not being able to edit the
file, you still have the capability to view it.

## Information Board

GpgFrontend presents the outcome of the current tab page operation on the
Information Board, signifying the success or failure of the operation.
Additionally, the Information Board's text includes supplementary details to
assist in understanding the particulars of your encryption, decryption,
signature, and other operations. Depending on your language settings, the output
displayed on the dashboard may differ.

The Information Board was conceived to provide a comprehensive view of more
information within the same space. However, GpgFrontend plans to incorporate a
graphical interface in the future to augment the user's comprehension of this
information.

### Color Coding

- **Green**: Indicates a successful operation that has been thoroughly verified
  and found devoid of any issues. The appearance of green font color signifies
  an all-clear.
- **Yellow**: Denotes a successful operation with some potential issues detected
  during result testing. The yellow font color serves as a subtle alert for the
  user, necessitating a detailed review of the operation.
- **Red**: Symbolizes an unsuccessful operation or a significant discrepancy in
  the operation's outcome. The red font color acts as a clear warning, demanding
  the user to meticulously inspect the operation's specifics to ensure security.

### Customizable Font Size

Should you find the font size on the information board to be diminutive, you can
easily adjust it under the 'Application' section in the settings. The default
font size is 10, and it can be modified to any value ranging from 9 to 18.

### Dashboard Actions Menu

The dashboard actions menu facilitates quick and easy access to common
functionalities related to the content of the information board. It empowers
users to efficiently manage and process large chunks of content on the
Information Board for various purposes.

#### Copy

This function allows users to swiftly capture significant portions of content
from the Information Board for other applications.

#### Save File

This operation archives the contents of the information board into the file
system, utilizing the UTF-8 format. Although the resultant output file lacks a
suffix, it is essentially in a plain text format.

#### Clear

This command promptly purges all content from the information board. The
clearance includes both the contents and statuses of the information board. A
new operation (such as encryption) will automatically trigger this clearing
process.

### Optional Actions Menu

In addition to the Information Board, an Optional Actions Menu will be presented
below it. If any auxiliary operations can be performed post your main operation
(such as displaying more detailed information, sending encrypted text via email,
etc.), the access points for these supplementary tasks will be made available
here.

## Key ToolBox

This feature presents a comprehensive list of key pairs stored on your device,
intended for use with Gpg operations. The keys within the ToolBox are classified
into multiple categories, each corresponding to a unique usage context.
Additionally, the ToolBox provides access to a variety of common operations, all
of which can be found in the Key List Menu.

### Usage

Most Gpg-related operations require specifying a key pair (for tasks like
encryption, decryption, signature, etc.). You can select the checkbox in the
first column of the Key ToolBox's table to designate one or more keys for your
operation. Categories that contain only public keys are frequently utilized in
cryptographic scenarios.

### Classification

The ToolBox showcases categories via tabbed display. None of these categories
include any expired or revoked keys. To view such keys, you should refer to the
Key Manager. The default category comprises all private and public keys. During
any operation, only the keys from the currently selected category will be
considered for input.

### Columns

Understanding this list is crucial. Let's walk through its components step by
step.

- **Select**: Check the box in this column to notify Gpg Frontend that you wish
  to use the key from this row for your subsequent operation.

- **Type**: This column informs you about the key type and whether the primary
  key exists in your key pair.

  - `pub` signifies that it is a public key, which can be used for encryption or
    verification operations.
  - `pub/sec` indicates that the key pair contains both public and private keys.
    It can be employed for nearly all operations (consult the 'Usage' column to
    confirm this).
  - `pub/sec#` shows that the key pair has a public key and a private key, but
    the primary key is absent from the key pair. This suggests you won't be able
    to perform certain specific operations (like adding subkeys, signing other
    key pairs, etc.)
  - `pub/sec^` implies that one or more keys (subkeys or master keys) from the
    key pair are in the smart card.
  - `pub/sec#^` denotes a simultaneous occurrence of the previous two
    situations.

- **Name**: Represents the identity information of the key pair.
- **Email Address**: Also denotes the identity information of the key pair.
- **Usage**: Determines which operations the key pair can execute. Composed of
  four uppercase letters, each letter signifies a specific use.

  - `C` stands for Certificate. Key pairs containing the primary key generally
    have this usage.
  - `E` stands for Encrypt. The key pair can be used for encryption operations.
  - `S` stands for Sign. The key pair can be used for signing operations.
  - `A` stands for Authenticate. The key pair can be used to perform operations
    like SSH authentication.

- **Validity**: A Gpg concept that roughly represents the level of trust in this
  key.

## Operations Bar

Here, you can execute corresponding operations by clicking on the buttons
provided. For instance, after inputting text into a text editor and specifying
the desired key in the key toolbox, you can click the encryption button to
perform the operation.

Some operations require key specification, while others do not, as will be
detailed in the respective sections of this document.

### Customization

For operations that you may not use for a while, you have the option to uncheck
the associated function group in the top menu view. Conversely, for the
operations you frequently use, you have the ability to add them here.
