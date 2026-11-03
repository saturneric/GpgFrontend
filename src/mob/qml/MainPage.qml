// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GpgFrontend.Mobile

Page {
    // One step back, as Android's Back asks; false when there is none left
    // and the application may close.
    function back() {
        // A sheet takes Back itself, unless it is busy and must not close;
        // either way Back is not for the page under it, or the application.
        if (importSheet.opened || generateSheet.opened) return true
        if (stack.depth > 1 && tabs.currentIndex === 1) {
            if (stack.currentItem && stack.currentItem.busy) return true
            stack.pop()
            return true
        }
        if (settingsStack.depth > 1 && tabs.currentIndex === 2) {
            settingsStack.pop()
            return true
        }
        if (tabs.currentIndex !== 0) {
            tabs.currentIndex = 0
            return true
        }
        return false
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        TextPage {}

        StackView {
            id: stack

            initialItem: KeysPage {
                onKeyChosen: function(key) {
                    stack.push(keyDetail, {"key": key})
                }
                onImportRequested: importSheet.begin()
                onGenerateRequested: generateSheet.begin()
            }
        }

        StackView {
            id: settingsStack

            initialItem: SettingsPage {
                onAboutRequested: settingsStack.push(about)
            }
        }
    }

    footer: NavBar {
        id: tabs

        objectName: "mainTabs"
        // While typing the page ends at the keyboard (the window counts it as
        // a safe area), and the sections' bar steps aside for what is typed.
        visible: Theme.keyboard === 0
        sections: [
            {"text": qsTr("Text"), "icon": "description"},
            {"text": qsTr("Keys"), "icon": "key"},
            {"text": qsTr("Settings"), "icon": "settings"}
        ]
    }

    Component {
        id: keyDetail
        KeyDetailPage {
            onBack: stack.pop()
            onDeleted: {
                stack.pop()
                KeyListController.refresh()
            }
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
        onGenerated: KeyListController.refresh()
    }
}
