// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What the application is: its name and version, what it does the work with,
// its license, and where the project lives. Nothing here goes online; the
// address is only shown.
Page {
    id: page

    signal back()

    header: AppBar {
        title: qsTr("About")
        canGoBack: true
        raised: scroller.contentItem.contentY > 0
        onBack: page.back()
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
            spacing: 0

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.gap
                Layout.topMargin: 24
                spacing: 8

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: "qrc:/icons/gpgfrontend_logo.png"
                    sourceSize: Qt.size(72, 72)
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    visible: status === Image.Ready
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.titleSize
                    text: qsTr("GpgFrontend")
                }

                Label {
                    objectName: "aboutVersion"
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: Material.secondaryTextColor
                    font.pixelSize: Theme.secondarySize
                    text: qsTr("Version %1").arg(Qt.application.version)
                }
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Engine")
            }

            ListRow {
                Layout.fillWidth: true
                inert: true
                text: qsTr("OpenPGP on this device")
                detail: qsTr("Keys, encryption and signatures are handled "
                             + "on this device by the rPGP engine.")
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("License")
            }

            ListRow {
                Layout.fillWidth: true
                inert: true
                text: qsTr("GNU General Public License 3.0 or later")
                detail: qsTr("GpgFrontend is free software: you may use, "
                             + "study, share and change it under this license.")
            }

            ListRow {
                Layout.fillWidth: true
                inert: true
                text: qsTr("Included works")
                detail: qsTr("Material Symbols (Apache License 2.0) and "
                             + "JetBrains Mono (SIL Open Font License 1.1).")
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Project")
            }

            ListRow {
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.gap
                text: qsTr("Website")
                detail: "https://gpgfrontend.bktus.com"
                onClicked: {
                    copier.copy(detail, false)
                    notice.show(qsTr("Address copied."))
                }
            }
        }
    }

    OutputActions {
        id: copier
    }

    Snackbar {
        id: notice
    }
}
