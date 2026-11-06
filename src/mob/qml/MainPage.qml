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
        // The sheet on top takes Back itself (one step, or nothing while it
        // is busy); either way Back is not for the page under it, or the
        // application. Every sheet, a page's own too, not only some.
        if (sheets.back()) return true
        const open = Theme.openSheets
        if (open.length > 0) {
            open[open.length - 1].back()
            return true
        }
        if (tabs.currentIndex === 0 && textPage.back()) return true
        if (tabs.currentIndex === 1 && filesPage.back()) return true
        if (stack.depth > 1 && tabs.currentIndex === 2) {
            if (stack.currentItem && stack.currentItem.busy) return true
            stack.pop()
            return true
        }
        if (settingsStack.depth > 1 && tabs.currentIndex === 3) {
            // A key page reached from Settings is as busy as on the Keys tab.
            if (settingsStack.currentItem && settingsStack.currentItem.busy) return true
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
        // A sheet at work (a key being made) stays, and so does its tab: the
        // files are in the list when the person gets there.
        const stays = main.closeSheets()
        const count = got.files.length
        if (count > 0) {
            if (!stays) tabs.currentIndex = 1
            filesPage.notice.show(
                qsTr("%n shared file(s) added, as copies for this session.", "", count))
        }
        if (got.refused > 0)
            filesPage.notice.show(qsTr("A shared file could not be opened. Save it "
                                       + "first, then add it here."))
        if (got.text.length > 0) {
            if (count === 0 && !stays) tabs.currentIndex = 0
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

    // Closes every sheet open, on any page, but one at work, which may not
    // be let go; true when such a sheet stays.
    function closeSheets() {
        sheets.closeAll()
        return Theme.closeOpenSheets()
    }

    // One for the application: a switch begun from Keys or from Settings is
    // seen under way on both.
    KeyDatabasesController {
        id: sharedDatabases
    }

    // Back to the first page of the Keys and the Settings tabs, but for a
    // page at work (it ends what it does first).
    function popToRoot(view) {
        while (view.depth > 1 && !(view.currentItem && view.currentItem.busy)) view.pop()
    }

    // Another key database: the pages of keys, groups and categories of
    // the last one would act on this one's keys of the same fingerprint.
    function keyDatabaseSwitched() {
        closeSheets()
        popToRoot(stack)
        popToRoot(settingsStack)
    }

    // The key @p key (the key list's roles) on the Keys page; a page there
    // at work stays under it.
    function showKey(key) {
        tabs.currentIndex = 2
        popToRoot(stack)
        stack.push(keyDetail, {"key": key})
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        TextPage {
            id: textPage
            sheets: sheets
            onImportRequested: text => importSheet.begin(text)
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
                keyDatabases: sharedDatabases
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
                onKeyDatabaseSwitched: main.keyDatabaseSwitched()
            }
        }

        StackView {
            id: settingsStack

            initialItem: SettingsPage {
                keyDatabases: sharedDatabases
                onAboutRequested: settingsStack.push(about)
                onBuildInfoRequested: settingsStack.push(buildInfo)
                onEngineInfoRequested: settingsStack.push(engineInfo)
                onTranslatorsRequested: settingsStack.push(translators)
                onCategoriesRequested: settingsStack.push(categoriesPage)
                onProfilesRequested: settingsStack.push(profilesPage)
                onProfileManagerRequested: settingsStack.push(profileManager)
                onKeyDatabaseSwitched: main.keyDatabaseSwitched()
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
            main.closeSheets()
            tabs.currentIndex = 1
            filesPage.showFile(id)
        }
        onOpenKeyRequested: key => main.showKey(key)
    }

    Component {
        id: keyDetail
        KeyDetailPage {
            id: detailPage

            // Its key was deleted while another page was opened over it:
            // it goes as soon as it is the one shown again.
            property bool gone: false

            onBack: StackView.view.pop()
            StackView.onActivated: if (gone) Qt.callLater(() => detailPage.StackView.view.pop())
            onDeleted: {
                // Only this page: one opened over it while it worked stays.
                if (StackView.view.currentItem === detailPage) StackView.view.pop()
                else gone = true
                KeyListController.refresh()
                // Said where the person now is: the page that would have said
                // it is gone.
                keysPage.notice.show(qsTr("Key deleted."))
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
