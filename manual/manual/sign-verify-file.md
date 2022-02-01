# Sign & Verify File

GpgFrontend provides two ways to perform file operations, a quick way and a custom way. The fast way is to use the
right-click menu of the file browser. The custom way uses the file path designate interface.

The following is divided into two parts to introduce the above two ways respectively.

## Quick Way

In the top menu file option, you can open the file browser(Ctrl/Command + B). Then by using the file browser, first
enter your working directory. Then right-click the file you need to operate, and then select the operation you want in
the pop-up menu.

![ScreenShot](https://github.com/saturneric/Blob/blob/master/screenshots/file-browser.png?raw=true)

There are two control buttons at the top of the file tab. The one on the left is the up level, and the one on the right
is to enter or refresh the corresponding path in the input box on the left.

### Sign

Through the right-click menu, you can quickly sign a file. This operation will generate a file with a sig suffix, which
contains the signature content. In this case, you need to pass this file along with the original file so that the other
party can verify it.

### Verify

This operation needs to select a file with a gpg suffix (may be invalid for the ciphertext of a binary file) or a file
with a sig suffix for verification.

When selecting a file with the sig suffix, make sure that the source file is also in this directory. This means that the
name of the source file is just missing a sig suffix.

## Custom Way

The custom mode provides a more flexible operation scheme for files. You can specify the directory, file name and suffix
of the output file.

### Sign

This operation will generate a file with a sig suffix(You can customize the name and path of the generated file), which
contains the signature content. In this case, you need to pass this file along with the original file so that the other
party can verify it.

### Verify

Consistent with fast operation. The difference is that you can specify the path of the source file and the signature
file, and their naming rules are basically unlimited.