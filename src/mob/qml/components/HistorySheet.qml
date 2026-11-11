// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What was done in this session, newest first: one row per operation, a
// batch of files as one. A tap opens its status, the same sheet the status
// line opens, with its Retry and its outputs. Kept only while GpgFrontend
// runs, as the operation log is.
BottomSheet {
    id: sheet

    // The SheetHost that opens a status and words a batch.
    property var host: null

    // Read again whenever the log changes.
    readonly property var batches: host && host.revision >= 0
                                   ? OperationQueue.log.recent() : []

    function begin() {
        open()
    }

    // A row's title: a batch by its counts, one operation by its outcome
    // and what it was done on.
    function titleOf(record) {
        if ((record.count || 1) > 1) {
            const records = OperationQueue.log.batch(record.batchId)
            return host.batchTitle(records, host.countsOf(records))
        }
        return qsTranslate("SheetHost", "%1 · %2").arg(Wording.title(record))
                                                  .arg(Wording.target(record))
    }

    function detailOf(record) {
        const when = record.submitted ? Qt.formatTime(record.submitted, Locale.ShortFormat) : ""
        const where = record.surface === "text" ? qsTr("Text") : qsTr("Files")
        return [when, where].filter(t => t.length > 0).join(" · ")
    }

    objectName: "historySheet"
    inset: 0
    title: qsTr("Recent operations")

    Label {
        objectName: "historyEmpty"
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sheetPad
        Layout.rightMargin: Theme.sheetPad
        visible: sheet.batches.length === 0
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: qsTr("Nothing was done yet since GpgFrontend started.")
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: sheet.batches

            ListRow {
                required property var modelData

                objectName: "history_" + modelData.operationId
                Layout.fillWidth: true
                leftPadding: Theme.sheetPad
                singleLineTitle: true
                symbol: modelData.phase === "done" ? "check_circle"
                        : modelData.phase === "failed" ? "error"
                        : modelData.phase === "cancelled" ? "info" : "schedule"
                symbolColor: modelData.phase === "done" ? Theme.positive
                             : modelData.phase === "failed" ? Theme.danger
                                                            : Material.secondaryTextColor
                text: sheet.titleOf(modelData)
                detail: sheet.detailOf(modelData)
                onClicked: {
                    const id = modelData.operationId
                    sheet.close()
                    // A batch whole, as the status line opens it.
                    sheet.host.openStatus(id, false)
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sheetPad
        Layout.rightMargin: Theme.sheetPad
        Layout.topMargin: Theme.gapSmall
        visible: sheet.batches.length > 0
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
        text: qsTr("Kept only until GpgFrontend closes.")
    }
}
