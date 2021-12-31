# Understand Interface

As a novice, you only need to quickly grasp the meaning of a few important parts of the page. You will gradually discover other functions in the next exploration. The interface may not be the same for different versions.

![Interface](https://github.com/saturneric/Blob/blob/master/screenshots/interface-introduce.png?raw=true)

## Text Editor

In the text editing area, you can type text at will. Or create a new tab through the New option in the top file menu. Tabs can be moved, closed, and so on.

You can operate your text through the options in the Operations Bar. Or you can use some common shortcuts(ctrl+c/v/s) to copy-paste save or even find operations.

The text edited in the text box is UTF8 encoded and does not have any formatting. Not providing any formatting (plain text) ensures that the reality of redaction in a message is not confusing. I plan to join rich text editors in the future, but the details are still under consideration.

### Large text file support

Here, GpgFrontend supports opening some of the larger files without being stuck. However, when opening a relatively large file, you cannot edit this tab until the file has been totally read. Although this will make you unable to edit the file, you can still view it.

## Information Board

The result of your operation on the current tab page will be printed in the Information Board: success or failure. In addition to the success and failure information, its text will also contain other information to help you understand the details of your encryption, decryption, signature and other operations. Depending on your language settings, the output in the dashboard will vary.

I used the information board as a solution to be able to display more information in the same space. In the future, a graphical interface display of this information will be introduced, which can help users understand the actual role of this information.

### Font Color

- **Green**: When the operation succeeds and the results of the operation are verified and no problems are found, the font color will be green.
- **Yellow**: When the operation succeeds and there are some problems with the result testing of the operation at that time, the font turns yellow, which is a reminder to the user. At this time, the user needs to check the details of the operation.
- **Red **: When the operation is unsuccessful or there is a serious conflict with the result of the operation, the font color will turn red, and the user will need to carefully check the details of the operation to ensure security.

### Font Size

When you feel that the font of the information board is too small, you can set the font size in the Application column of the settings.  The font size defaults to 10 and can be set to a range of between 9 and 18.

### Information Board Actions Menu

The dashboard actions menu provides some commonly used actions for information board content. This enables users to quickly record large pieces of content in the Information Board for other uses.

#### Copy

This enables users to quickly record large pieces of content in the Information Board for other uses.

#### Save File

The operation stores the contents of the information board in the file system in UTF-8 format. Although the output file does not have a suffix name, in fact this file is in plain text format.

#### Clear

This action immediately empties the  information board. The empty operation includes the contents of the  information board and all the statuses. The emptying operation occurs automatically when you make the next operation (encryption, etc.).

### Optional Actions Menu

There will also be a column of Optional Actions Menu below the Information Board. If there are other auxiliary operations that can be done after your operation is completed (display more detailed information, send encrypted text through email, etc.), the entry points for these auxiliary operations will be displayed here.

## Key ToolBox

Here is a list of key pairs stored on your machine that can be used for Gpg operations.  The key lists in the Toolbox have multiple categories that correspond to different usage scenarios. The toolbox also provides some commonly used operations, all of which are in the Key List Menu.

### Usage

Most operations related to Gpg need to specify a key pair (such as encryption, decryption, signature, etc.). You can select the check box in the first column of the table in the key toolbox to specify one or more keys for your operation. Classifications that contain only public keys are often used in cryptographic scenarios.

### Classification

The Toolbox provides a categorical display through tabs. All classifications do not include all expired or revoked keys. If you want to view expired or revoked keys, use the Key Manager. The default classification contains all private and public keys. The operation takes only the key from the currently selected classification as input.

### Columns

It is important to understand this list. Now let me take you to understand it step by step.

- Select: Turn the checkbox in this column to let Gpg Frontend know that you specify the key of this row for your next
  operation.

- Type: See this column to let you know the type of key and whether the master key exists in your key pair.
    - `pub` means this is a public key, Can be used for encryption or verification operations.
    - `pub/sec` The key pair contains both public and private keys. It can be used for almost all operations(Need to
      combine the usage column to determine this).
    - `pub/sec#`  The key pair contains a public key and a private key, but the master key is not in the key pair. This shows that you will not be able to do some special (add subkeys, sign other key pairs, etc.)
    
- Name: The identity information of the key pair.

- Email Address: The identity information of the key pair.

- Usage: This determines which operations the key pair can use. Composed of four capital letters, each capital letter represents a usage.
    - `C` Certificate. Generally, the key pair that contains the master key will have this usage
    - `E` Encrypt. The key pair can be used for encryption operations.
    - `S` Sign. The key pair can be used for sign operations.
    - `A` Authenticate. The key pair can be used to perform operations like SSH authentication.
  
- Validity: One of the concepts of Gpg, simply put it represents the degree of trust in this key.

## Operations Bar

Here, you can perform corresponding operations by clicking the buttons above. For example, after typing text in a text editor and setting the key you want to use in the key toolbox, you can click the encryption button to perform the operation.

Some operations need to specify the key, and some are not used, which will be explained in other corresponding parts of the document.

### Customize

Some operations you may not use for a long time, at which point you can uncheck the relevant function group in the view of the top menu. Conversely, for some of the operations you use frequently, you can also add here.