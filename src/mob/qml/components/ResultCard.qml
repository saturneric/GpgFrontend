// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One card of a result as the desktop's Status Panel draws it (a signature,
// a recipient, the message's metadata): a neutral sheet with a fine border
// and small corners, its status only a narrow edge on the left and a small
// stamp by the title, what it is about on the right; then each fact in two
// columns, labels a step quieter than their values, runs of facts (what
// happened, why, what next) set a little apart.
Rectangle {
    id: card

    // {title, status: "ok" | "warn" | "critical" | "neutral", fields:
    //  [{label, value, technical, note, group}]}, and the file it is about
    //  in a batch (source).
    property var model: ({})

    readonly property color statusColor: {
        switch (model.status) {
        case "ok": return Theme.positive
        case "warn": return Theme.warning
        case "critical": return Theme.danger
        default: return Material.secondaryTextColor
        }
    }
    // The page's own colours, as the desktop mixes them: labels between the
    // text and the page, identifiers a little nearer the text.
    readonly property color muted: Theme.mix(Material.foreground, page, 0.6)
    readonly property color technical: Theme.mix(Material.foreground, page, 0.8)
    readonly property color page: Material.backgroundColor
    readonly property color edge: Material.dividerColor

    // A long identifier in groups of eight, as the desktop shows it; words
    // as they are.
    function grouped(value) {
        if (value.length <= 24 || /\s/.test(value)) return value
        return value.match(/.{1,8}/g).join(" ")
    }

    objectName: "resultCard"
    implicitHeight: content.implicitHeight + 2 * 12
    radius: 4
    // A step off the sheet, as the desktop's card is off its panel.
    color: Theme.surface(page, 1)
    border.width: 1
    border.color: edge
    Accessible.role: Accessible.Grouping
    Accessible.name: model.title || ""

    // The status edge, inside the border on the left.
    Rectangle {
        x: 1
        y: 1
        width: 3
        height: parent.height - 2
        topLeftRadius: card.radius - 1
        bottomLeftRadius: card.radius - 1
        color: Theme.mix(card.statusColor, card.edge, 0.67)
    }

    ColumnLayout {
        id: content

        x: 3 + 14
        y: 12
        width: card.width - x - 14
        spacing: 6

        // The header: stamp and title on the left, the source on the right.
        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 4
            spacing: 8

            Image {
                source: Theme.cardStamp(card.model.status || "")
                sourceSize: Qt.size(14, 14)
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 3
                Accessible.ignored: true
            }

            Label {
                Layout.fillWidth: true
                text: card.model.title || ""
                wrapMode: Text.Wrap
                font.pixelSize: Theme.secondarySize
                font.weight: Font.DemiBold
            }

            Label {
                objectName: "resultCardSource"
                Layout.maximumWidth: Math.round(card.width * 0.4)
                Layout.alignment: Qt.AlignTop
                visible: text.length > 0
                text: card.model.source || ""
                elide: Text.ElideMiddle
                color: card.muted
                font.pixelSize: Theme.labelSize
            }
        }

        Repeater {
            model: card.model.fields || []

            // A fact, its name and its value side by side.
            Item {
                id: fact

                required property var modelData
                required property int index
                // A new run of facts starts a little apart.
                readonly property bool newRun: index > 0
                    && (card.model.fields[index - 1].group !== modelData.group)

                Layout.fillWidth: true
                Layout.topMargin: newRun ? 6 : 0
                implicitHeight: Math.max(name.y + name.implicitHeight, values.implicitHeight)

                Label {
                    id: name
                    // On the value's first line, though it is smaller.
                    y: Math.max(0, value.baselineOffset - baselineOffset)
                    width: Math.round(fact.width * 0.3)
                    text: fact.modelData.label
                    wrapMode: Text.Wrap
                    color: card.muted
                    font.pixelSize: Theme.labelSize
                }

                Column {
                    id: values
                    x: name.width + 10
                    width: fact.width - x
                    spacing: 1

                    TextEdit {
                        id: value
                        width: parent.width
                        text: fact.modelData.technical
                              ? card.grouped(fact.modelData.value)
                              : fact.modelData.value
                        readOnly: true
                        selectByMouse: true
                        wrapMode: fact.modelData.technical ? TextEdit.WrapAnywhere
                                                           : TextEdit.Wrap
                        color: fact.modelData.technical ? card.technical
                                                        : Material.foreground
                        selectionColor: Material.accentColor
                        font.family: fact.modelData.technical ? Theme.mono
                                                              : Qt.application.font.family
                        font.pixelSize: fact.modelData.technical ? Theme.labelSize
                                                                 : Theme.secondarySize
                        Accessible.role: Accessible.StaticText
                        Accessible.name: fact.modelData.label + ": "
                                         + fact.modelData.value
                    }

                    // What the value belongs to (a stage, a code): quieter.
                    Label {
                        width: parent.width
                        visible: (fact.modelData.note || "").length > 0
                        text: fact.modelData.note || ""
                        wrapMode: Text.Wrap
                        color: card.muted
                        font.family: Theme.mono
                        font.pixelSize: Theme.labelSize
                    }
                }
            }
        }
    }
}
