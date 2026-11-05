// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GpgFrontend.Mobile

Page {
    id: main

    readonly property alias sheets: sheets

    // One step back, as Android's Back asks; false when there is none left
    // and the application may close. In order: what is open over the page,
    // then what the page is in the middle of, then the page itself.
    function back() {
        // A sheet takes Back itself, unless it is busy and must not close;
        // either way Back is not for the page under it, or the application.
        if (importSheet.opened || generateSheet.opened || generatedSheet.opened) return true
        if (sheets.back()) return true
        if (tabs.currentIndex === 0 && textPage.back()) return true
        if (tabs.currentIndex === 1 && filesPage.back()) return true
        if (stack.depth > 1 && tabs.currentIndex === 2) {
            if (stack.currentItem && stack.currentItem.busy) return true
            stack.pop()
            return true
        }
        if (settingsStack.depth > 1 && tabs.currentIndex === 3) {
            settingsStack.pop()
            return true
        }
        if (tabs.currentIndex !== 0) {
            tabs.currentIndex = 0
            return true
        }
        return false
    }

    // What other applications shared, taken when the page comes up and each
    // time the application comes back: files to the Files page, as copies,
    // text to the Text page.
    function takeShares() {
        const got = ShareInbox.take()
        if (got.files === undefined) return
        sheets.closeAll()
        const count = got.files.length
        if (count > 0) {
            tabs.currentIndex = 1
            filesPage.notice.show(
                qsTr("%n shared file(s) added, as copies for this session.", "", count))
        }
        if (got.refused > 0)
            filesPage.notice.show(qsTr("A shared file could not be opened. Save it "
                                       + "first, then add it here."))
        if (got.text.length > 0) {
            if (count === 0) tabs.currentIndex = 0
            textPage.receive(got.text)
        }
    }

    Component.onCompleted: {
        takeShares()
        ShareInbox.SignalArrived.connect(main.takeShares)
    }

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (Qt.application.state === Qt.ApplicationActive) main.takeShares()
        }
    }

    // Shared while the page is already on screen (split screen, a window).
    // Signals named Signal... have no on-handler in QML; the inbox outlives
    // the page.
    Component.onDestruction: ShareInbox.SignalArrived.disconnect(main.takeShares)

    // The key @p key (the key list's roles) on the Keys page.
    function showKey(key) {
        tabs.currentIndex = 2
        while (stack.depth > 1) stack.pop()
        stack.push(keyDetail, {"key": key})
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        TextPage {
            id: textPage
            sheets: sheets
            onImportRequested: text => importSheet.begin(text)
            onOpenFilesRequested: {
                tabs.currentIndex = 1
                filesPage.addFiles()
            }
        }

        FilesPage {
            id: filesPage
            sheets: sheets
        }

        StackView {
            id: stack

            initialItem: KeysPage {
                id: keysPage

                creatingKey: generateSheet.quickRunning
                onKeyChosen: function(key) {
                    stack.push(keyDetail, {"key": key})
                }
                onImportRequested: importSheet.begin()
                onGenerateRequested: generateSheet.begin()
                onQuickGenerateRequested: generateSheet.quick()
                onCategoriesManageRequested: stack.push(categoriesPage)
                onGroupOpened: function(id, pick) {
                    stack.push(keyGroupPage, {"groupId": id, "pickOnOpen": pick})
                }
            }
        }

        StackView {
            id: settingsStack

            initialItem: SettingsPage {
                onAboutRequested: settingsStack.push(about)
                onBuildInfoRequested: settingsStack.push(buildInfo)
                onEngineInfoRequested: settingsStack.push(engineInfo)
                onTranslatorsRequested: settingsStack.push(translators)
                onCategoriesRequested: settingsStack.push(categoriesPage)
                onProfilesRequested: settingsStack.push(profilesPage)
                onProfileManagerRequested: settingsStack.push(profileManager)
            }
        }
    }

    footer: NavBar {
        id: tabs

        objectName: "mainTabs"
        // Always there: the keyboard covers it while it is up (the window
        // does not make room for the keyboard), and it is there again when
        // the keyboard goes, whatever the input method still says.
        sections: [
            {"text": qsTr("Text"), "icon": "description"},
            {"text": qsTr("Files"), "icon": "folder"},
            {"text": qsTr("Keys"), "icon": "key"},
            {"text": qsTr("Settings"), "icon": "settings"}
        ]
        // Going elsewhere: what ends meanwhile is not pushed at the person,
        // and a sheet left open does not follow them.
        onCurrentIndexChanged: {
            sheets.distract()
            sheets.closeAll()
        }
    }

    // The keys for this work and how operations went, for every page.
    SheetHost {
        id: sheets

        anchors.fill: parent
        textSurface: textPage
        filesSurface: filesPage
        onShowFileRequested: function(id) {
            tabs.currentIndex = 1
            filesPage.showFile(id)
        }
        onOpenKeyRequested: key => main.showKey(key)
    }

    Component {
        id: keyDetail
        KeyDetailPage {
            onBack: StackView.view.pop()
            onDeleted: {
                StackView.view.pop()
                KeyListController.refresh()
            }
        }
    }

    // The categories, and one of them: in whichever stack opened them (the
    // Keys tab's or Settings').
    Component {
        id: categoriesPage
        KeyCategoriesPage {
            onBack: StackView.view.pop()
            onCategoryOpened: function(category) {
                StackView.view.push(categoryKeysPage, {"category": category})
            }
        }
    }

    Component {
        id: categoryKeysPage
        CategoryKeysPage {
            onBack: StackView.view.pop()
            onKeyChosen: function(key) {
                StackView.view.push(keyDetail, {"key": key})
            }
            onGroupOpened: function(id) {
                StackView.view.push(keyGroupPage, {"groupId": id})
            }
        }
    }

    Component {
        id: keyGroupPage
        KeyGroupPage {
            onBack: StackView.view.pop()
        }
    }

    Component {
        id: profilesPage
        KeyGenProfilesPage {
            onBack: settingsStack.pop()
            onEditRequested: (index, profile) => settingsStack.push(
                                 profileEditor, {"index": index, "profile": profile})
        }
    }

    Component {
        id: profileEditor
        KeyGenProfileEditPage {
            onBack: settingsStack.pop()
            onSaved: settingsStack.pop()
        }
    }

    Component {
        id: profileManager
        ProfilesPage {
            onBack: settingsStack.pop()
        }
    }

    // What the desktop's About dialog tells of a build, a page each.
    AboutInfo {
        id: aboutInfo
    }

    Component {
        id: buildInfo
        FactsPage {
            objectName: "buildInfoPage"
            title: qsTr("Build information")
            sections: [{"title": qsTr("Build"), "rows": aboutInfo.build()},
                       {"title": qsTr("Android package"),
                        "rows": aboutInfo.androidBuild()}]
                      .filter(s => s.rows.length > 0)
            onBack: settingsStack.pop()
        }
    }

    Component {
        id: engineInfo
        FactsPage {
            objectName: "engineInfoPage"
            title: qsTr("rPGP engine")
            intro: qsTr("GpgFrontend can use more than one OpenPGP engine. "
                        + "Here, all the work is done by rPGP, an engine "
                        + "written in Rust and built into the application.")
            sections: [{"title": qsTr("Engine"), "rows": aboutInfo.engine()},
                       {"title": qsTr("Key dependencies"),
                        "rows": aboutInfo.engineDependencies()}]
                      .filter(s => s.rows.length > 0)
            onBack: settingsStack.pop()
        }
    }

    Component {
        id: translators
        TranslatorsPage {
            onBack: settingsStack.pop()
        }
    }

    Component {
        id: about
        AboutPage {
            onBack: settingsStack.pop()
        }
    }

    ImportSheet {
        id: importSheet
    }

    GenerateSheet {
        id: generateSheet
        onGenerated: function(fingerprint) {
            KeyListController.refresh()
            generatedSheet.begin(fingerprint)
        }
        onQuickFailed: error => keysPage.notice.show(
                           qsTr("The key could not be generated: %1").arg(error))
    }

    GeneratedKeySheet {
        id: generatedSheet
        onOpenRequested: key => main.showKey(key)
    }
}
