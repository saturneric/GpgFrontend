# Translate Interface

GpgFrontend is designed to support multiple languages, but requires volunteer contributions to achieve this goal.
Fortunately, translation work does not require an understanding of difficult technology. Volunteers only need to move
their fingers to complete.

There are two ways to translate, through a translation tool with a GUI interface or directly through a text editor. 

## What you need to know about translation work

From v2.0.1, Gpg Frontend uses the popular translation support library [GNU gettext](https://www.gnu.org/software/gettext/) in the GNU project. Before starting everything, you need to know something about this library. After you are sure about the content of the document, you can first try to see how GpgFrontned uses the tools provided by this library.

### Facts

Most of the translation work is carried out by Google Translate. In most cases, what you have to do is to correct some of the bad aspects of Google Translate.

## About translation files

1. Download or clone source code [Here](https://github.com/saturneric/GpgFrontend)
2. You will find some po files(.po) at path `resource/locale/po`
2. You will find some template file(.pot) at path `resource/locale/template`

## Before starting your work

In order to facilitate coordination, please contact me via email before you start this work. This is very important, please contact me first so that the work you do can be better used by GpgFrontend.

## Hand in your work

You can submit your great work in two ways:

1. Raise a pull request and merge the changed translation file(s) to the repository.
2. [Email ME](mailto:eric@bktus.com). Please attach the changed ts file on the email.