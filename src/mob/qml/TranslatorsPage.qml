// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Who translated GpgFrontend, as the desktop's About dialog thanks them:
// the list the application carries, and where to read how to help. Nothing
// here goes online; the address is only shown.
Page {
    id: page

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

    signal back()

    readonly property string list: about.translators()

    header: AppBar {
        title: qsTr("Translators")
        canGoBack: true
        raised: scroller.contentItem.contentY > 0
        onBack: page.back()
    }

    AboutInfo {
        id: about
    }

    ScrollView {
        id: scroller

        anchors.fill: parent
        // Under the bar, which takes the page's colour, nothing shows.
        clip: true
        contentWidth: availableWidth
        implicitWidth: 0

        ColumnLayout {
            width: scroller.availableWidth
            spacing: 12

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                Layout.topMargin: Theme.gap
                wrapMode: Text.Wrap
                font.pixelSize: Theme.bodySize
                font.weight: Font.Medium
                text: qsTr("Thanks to all translators")
            }

            TextEdit {
                objectName: "translatorsList"
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.PlainText
                color: Material.foreground
                selectionColor: Material.accentColor
                font.pixelSize: Theme.secondarySize
                text: page.list.length > 0 ? page.list
                      : qsTr("Translator information is not available.")
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                Layout.bottomMargin: Theme.gap
                wrapMode: Text.Wrap
                color: Material.secondaryTextColor
                font.pixelSize: Theme.secondarySize
                text: qsTr("To help translate GpgFrontend, read the translation "
                           + "guide at gpgfrontend.bktus.com.")
            }
        }
    }
}
