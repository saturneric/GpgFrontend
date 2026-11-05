// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Facts about this build, as the desktop's About dialog lists them (Build
// Information, rPGP Engine): a line on what they are, then each section's
// facts, a name and its value; all of them copied at once from the menu, to
// paste where someone asks.
Page {
    id: page

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

    // What it is, said once above the facts.
    property string intro: ""
    // [{title, rows: [{label, value, full}]}]; full, where given, is what
    // a shortened value stands for, and what is copied.
    property var sections: []

    signal back()

    readonly property alias notice: done

    // The facts as text: "Name: value", a line each, sections apart.
    function asText() {
        return sections.map(s => s.rows.map(r => r.label + ": " + (r.full || r.value))
                                   .join("\n"))
                       .join("\n\n")
    }

    header: AppBar {
        title: page.title
        canGoBack: true
        raised: scroller.contentItem.contentY > 0
        onBack: page.back()

        BarButton {
            objectName: "factsCopy"
            text: qsTr("Copy")
            icon.source: Theme.icon("content_copy")
            onClicked: {
                output.copy(page.asText(), false)
                done.show(qsTr("Copied."))
            }
        }
    }

    OutputActions {
        id: output
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

            Label {
                Layout.fillWidth: true
                Layout.margins: Theme.gap
                visible: page.intro.length > 0
                wrapMode: Text.Wrap
                color: Material.secondaryTextColor
                text: page.intro
            }

            Repeater {
                model: page.sections

                ColumnLayout {
                    id: section

                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 0

                    SectionHeader {
                        Layout.fillWidth: true
                        text: section.modelData.title
                    }

                    // A label on the left, its value on the right, the value
                    // wrapping in its own column however long it is.
                    Repeater {
                        model: section.modelData.rows

                        InfoRow {
                            required property var modelData

                            objectName: "fact_" + modelData.label
                            Layout.fillWidth: true
                            Layout.leftMargin: Theme.gap
                            Layout.rightMargin: Theme.gap
                            Layout.topMargin: 6
                            Layout.bottomMargin: 6
                            label: modelData.label
                            value: modelData.value
                        }
                    }
                }
            }

            Item {
                Layout.preferredHeight: Theme.gap
            }
        }
    }

    Snackbar {
        id: done
    }
}
