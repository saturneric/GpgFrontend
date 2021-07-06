# Encrypt & Sign File

GpgFrontend provides two ways to perform file operations, a quick way and a custom way.
The fast way is to use the right-click menu of the file browser.
It provides the recommended method for encryption and decryption operations (encrypt & sign and decrypt & verify).
The custom way provides two outdated but general and old functions: only encryption and only decryption.

The following is divided into two parts to introduce the above two ways respectively.

## Fast Way

In the top menu file option, you can open the file browser.
Then by using the file browser, first enter your working directory.
Then right-click the file you need to operate, and then select the operation you want in the pop-up menu.

### Encrypt & Sign

This method provides encryption and signature functions, which GpgFrontend recommend, so that the receiver can know that the ciphertext comes from you.


### Decrypt & Verify

This ciphertext is verified while decrypting, which can improve security.
In addition, you can also perform Only Verify operations and this operation will verify without decryption.

## Custom Way

The custom mode provides a more flexible operation scheme for files. You can specify the directory, file name and suffix of the output file.

### Encrypt 

This operation will only encrypt the file which means the receiver won't know who this ciphertext is created by.

### Decrypt










