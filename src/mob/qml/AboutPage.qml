// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What the application is, as the desktop's About tells it: its name,
// version and what it is for, who makes it and how to reach them, where the
// project lives, its license. Nothing here goes online: a tap copies the
// address, to open it where one chooses.
Page {
    id: page

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

    signal back()

    function copyLink(address) {
        copier.copy(address, false)
        notice.show(qsTr("Address copied."))
    }

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
                    sourceSize: Qt.size(88, 88)
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 88
                    visible: status === Image.Ready
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.titleSize
                    font.weight: Font.DemiBold
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

                Label {
                    objectName: "aboutTagline"
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.secondarySize
                    text: qsTr("A user-friendly OpenPGP tool for encryption, "
                               + "signing, and key management.")
                }
            }

            // As the desktop's: set apart, the whole card the target.
            ItemDelegate {
                id: star

                readonly property string url: "https://github.com/saturneric/GpgFrontend"

                objectName: "aboutStar"
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                Layout.bottomMargin: Theme.gapSmall
                padding: 14
                text: qsTr("Star GpgFrontend on GitHub")
                Accessible.description: starText.text
                onClicked: page.copyLink(url)

                background: Rectangle {
                    radius: 8
                    color: star.pressed ? Theme.alpha(Theme.warning, 0.12) : "transparent"
                    border.width: 1
                    border.color: Theme.warning
                }

                contentItem: ColumnLayout {
                    spacing: 4

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        font.weight: Font.DemiBold
                        text: "\u2605 " + star.text
                    }

                    Label {
                        id: starText
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.secondarySize
                        color: Material.secondaryTextColor
                        text: qsTr("GpgFrontend is free and open source. A star helps "
                                   + "more people discover it and keeps the project "
                                   + "moving forward.")
                    }
                }
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Developer")
            }

            ListRow {
                Layout.fillWidth: true
                inert: true
                text: qsTr("Developed and maintained by Saturneric.")
            }

            Repeater {
                model: [
                    {"text": qsTr("Report an issue on GitHub"),
                     "url": "https://github.com/saturneric/GpgFrontend/issues"},
                    {"text": qsTr("About and contact information"),
                     "url": "https://gpgfrontend.bktus.com/overview/contact/"},
                    {"text": qsTr("Email"), "url": "mailto:eric@bktus.com",
                     "shown": "eric@bktus.com"},
                ]

                ListRow {
                    required property var modelData
                    Layout.fillWidth: true
                    text: modelData.text
                    detail: modelData.shown || modelData.url
                    onClicked: page.copyLink(modelData.shown || modelData.url)
                }
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Resources")
            }

            Repeater {
                model: [
                    {"text": qsTr("Website"), "shown": "gpgfrontend.bktus.com",
                     "url": "https://gpgfrontend.bktus.com"},
                    {"text": qsTr("Documentation"), "shown": qsTr("User guides and overview"),
                     "url": "https://gpgfrontend.bktus.com/overview/glance"},
                    {"text": qsTr("Source code"), "shown": "github.com/saturneric/GpgFrontend",
                     "url": "https://github.com/saturneric/GpgFrontend"},
                    {"text": qsTr("Release notes"), "shown": qsTr("Changelog and downloads"),
                     "url": "https://github.com/saturneric/GpgFrontend/releases"},
                ]

                ListRow {
                    required property var modelData
                    objectName: "aboutResource_" + index
                    required property int index
                    Layout.fillWidth: true
                    text: modelData.text
                    detail: modelData.shown
                    onClicked: page.copyLink(modelData.url)
                }
            }

            // Kept here, as the phone's own: what it bundles beyond the code.
            ListRow {
                Layout.fillWidth: true
                inert: true
                text: qsTr("Included works")
                detail: qsTr("Material Symbols (Apache License 2.0) and "
                             + "JetBrains Mono (SIL Open Font License 1.1).")
            }

            // The license and copyright, small and quiet, as the desktop ends.
            Label {
                objectName: "aboutLicense"
                Layout.fillWidth: true
                Layout.topMargin: Theme.gap
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.secondarySize
                color: Material.secondaryTextColor
                text: qsTr("GpgFrontend is free software, licensed under "
                           + "GPL-3.0-or-later.")
            }

            Label {
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.gap
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.labelSize
                color: Material.secondaryTextColor
                text: "\u00A9 2021-2026 Saturneric <eric@bktus.com>"
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
