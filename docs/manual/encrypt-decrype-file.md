# Encrypt & Sign File

GpgFrontend provides two ways to perform file operations, a quick way and a custom way. The fast way is to use the
right-click menu of the file browser. It provides the recommended method for encryption and decryption operations (
encrypt & sign OR decrypt & verify). The custom way provides two outdated but general and old functions: only
encryption and only decryption.

The following is divided into two parts to introduce the above two ways respectively.

## Fast Way

In the top menu file option, you can open the file browser(Ctrl/Command + B). Then by using the file browser, first
enter your working directory. Then right-click the file you need to operate, and then select the operation you want in
the pop-up menu.

![ScreenShot](https://github.com/saturneric/Blob/blob/master/screenshots/file-browser.png?raw=true)


### Encrypt & Sign

This method provides encryption and signature functions, which GpgFrontend recommend, so that the receiver can know that
the ciphertext comes from you.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encr-sign-file.gif?raw=true)

### Decrypt & Verify

This ciphertext is verified while decrypting, which can improve security. In addition, you can also perform Only Verify
operations and this operation will verify without decryption.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt-verify-file.gif?raw=true)

## Custom Way

The custom mode provides a more flexible operation scheme for files. You can specify the directory, file name and suffix
of the output file.

![ScreenShot](https://github.com/saturneric/Blob/blob/master/screenshots/custom-way-1.png?raw=true)

![ScreenShot](https://github.com/saturneric/Blob/blob/master/screenshots/custom-way-2.png?raw=true)

### Encrypt

This operation will only encrypt the file which means the receiver won't know who this ciphertext is created by.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/encr-file-custom.gif?raw=true)

### Decrypt

For decryption, you do not need to check the corresponding key from the table. GpgFrontend will make the selection
automatically.

![GIF](https://github.com/saturneric/Blob/blob/master/gif/decrypt-file-custom.gif?raw=true)




