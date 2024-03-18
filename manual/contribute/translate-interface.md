# Translate Interface

GpgFrontend is designed to support multiple languages, but requires volunteer
contributions to achieve this goal. Fortunately, translation work does not
require an understanding of difficult technology. Volunteers simply need to use their skills to get the job done and make it accessible to everyone.

## Work Offline

### What you need to know about translation work

From v2.1.2, GpgFrontend uses the Qt translation support library [Qt
Linguist](https://doc.qt.io/qt-6/qtlinguist-index.html) in the Qt project.
Before starting everything, you need to know something about this library. After
you are sure about the content of the document, you can first try to see how
GpgFrontend uses the tools provided by this library.

### About translation files

1. Download or clone source code
   [HERE](https://github.com/saturneric/GpgFrontend)
2. You will find some ts files(.ts) at path `resource/lfs/locale/ts`
3. Add a new language: Create a new file; see [locale codes](https://saimana.com/list-of-country-locale-code/).
4. To edit or update an existing language, navigate to the path `resource/lfs/locale/ts`.

### Before starting your work

In order to facilitate coordination, please contact me via email before you
start this work. This is very important, please contact me first so that the
work you do can be better used by GpgFrontend.

### Updates to Translation Work Process

- **For correcting existing translations**: You can directly modify the ts files
  and then send them to me via email.
- **For adding a new language translation**: If you're unfamiliar with the
  process, you can request a new ts file by emailing me. I will provide you with
  the necessary file to start the translation for the new language.

### Hand in your work

You can submit your great work in two ways:

1. Raise a pull request and merge the changed translation file(s) to the
   repository.
2. [Email ME](mailto:eric@bktus.com). Please attach the changed ts file in the
   email.
