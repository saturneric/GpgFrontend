// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// How an operation went -- or a batch of them -- from its record
// (OperationLog), never from what a page shows now: its keys as they were
// asked, each signature's three facts apart, and what can be done with what
// it made. Half the window first; expanded, every file of a batch, the
// engine's own report and what else was done this session.
//
// Closing it never stops anything.
BottomSheet {
    id: sheet

    property var host: null
    // The operation shown: any operation of a batch shows the whole batch,
    // unless it is asked for alone (a file of the batch, opened from it).
    property int shownId: 0
    property bool single: false
    // The file alone was opened from its batch's list, so Back goes back
    // there; opened alone from elsewhere (its file), Back closes it.
    property bool fromBatch: false

    readonly property var log: OperationQueue.log
    readonly property var items: {
        // Read again whenever the log changes.
        if (!host || host.revision < 0 || shownId <= 0) return []
        const first = log.record(shownId)
        if (first.operationId === undefined) return []
        return single ? [first] : log.batch(first.batchId)
    }
    readonly property bool isBatch: items.length > 1
    readonly property var record: items.length > 0 ? items[0] : ({})
    readonly property var counts: host ? host.countsOf(items) : ({})
    readonly property bool active: counts.active > 0
    readonly property bool isText: (record.surface || "") === "text"
    // The cards of what is shown: one operation's, or every file's of a
    // batch, each card naming its file as the desktop names a card's
    // source, so each file's facts are found by its name.
    readonly property var cards: {
        let all = []
        for (const item of items) {
            const name = isBatch ? Wording.target(item) : ""
            for (const card of ((item.report || {}).cards || [])) {
                all.push(Object.assign({}, card, {"source": name}))
            }
        }
        return all
    }
    // The engine's words for it; a batch's, file by file under its name.
    // Without the rule the desktop ends each report with.
    readonly property string reportText: {
        const bare = r => ((r || {}).report || "").trim().replace(/\n+-{8,}$/, "")
        if (!isBatch) return bare(record.report)
        return items.filter(item => bare(item.report).length > 0)
                    .map(item => qsTr("File: %1").arg(Wording.target(item)) + "\n"
                                 + bare(item.report))
                    .join("\n\n")
    }

    // Shows operation @p id; with @p alone, that one even of a batch.
    function show(id, alone) {
        single = alone === true
        fromBatch = false
        shownId = id
        showReport.currentIndex = 0
        reportScroll.contentY = 0
        open()
    }

    // What the batch made that is still in the list.
    function batchOutputs() {
        let outputs = []
        for (const item of sheet.items) outputs = outputs.concat(item.outputs || [])
        const surface = sheet.host ? sheet.host.filesSurface : null
        if (!surface) return []
        return outputs.filter(id => surface.workspace.entry(id).fileId !== undefined)
    }

    function textHeld() {
        return isText && host && host.textSurface !== null
               && host.textSurface.holds(record.operationId)
    }

    // What is done with a result: compact text buttons, quieter than the
    // report they follow.
    // Its own icon and label: the style's label elided a short "Save all"
    // to "Save …" inside a Flow.
    component ResultAction: Button {
        id: action

        flat: true
        leftPadding: 12
        rightPadding: 12
        topInset: 4
        bottomInset: 4

        contentItem: Row {
            spacing: 8

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: action.icon.source.toString().length > 0
                source: action.icon.source
                sourceSize: Qt.size(18, 18)
                color: action.enabled ? Material.foreground : Material.hintTextColor
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: action.text
                font.pixelSize: Theme.labelSize
                font.weight: Font.Medium
                color: action.enabled ? Material.foreground : Material.hintTextColor
            }
        }
    }

    objectName: "statusSheet"
    expandable: true
    // Its cards and report show only once it is open all the way.
    revealsMore: true
    // From one file back to its batch, before the sheet closes.
    stepBack: function() {
        if (!single || !fromBatch || log.batch(record.batchId || 0).length <= 1) return false
        single = false
        fromBatch = false
        return true
    }
    onClosed: {
        single = false
        fromBatch = false
    }
    // How it ended, as a report says it ({status, severity, cancelled}), for the
    // header every finished operation has, text or files, one or a batch;
    // null while it runs. An operation the engine never reported on ended
    // failed or stopped; a batch ended well if every file did, badly if
    // none did, and with a warning between.
    readonly property var outcome: {
        if (items.length === 0 || active) return null
        if (!isBatch) {
            if (Object.keys(record.report || {}).length > 0) return record.report
            if (record.phase === "cancelled") return {"cancelled": true}
            return record.phase === "failed" ? {"status": -1, "severity": "critical"}
                                             : null
        }
        if (counts.failed + counts.cancelled === 0) return {"status": 1, "severity": "ok"}
        if (counts.done > 0) return {"status": 0, "severity": "warn"}
        return counts.failed > 0 ? {"status": -1, "severity": "critical"}
                                 : {"cancelled": true}
    }
    readonly property bool hasReport: outcome !== null
    // Why one operation failed, as a kind with words of its own; then the
    // sheet says it once, in those words, and keeps the engine's for the
    // details.
    readonly property string failureKind: !isBatch && record.phase === "failed"
                                          ? ((record.report || {}).reason || "") : ""
    readonly property bool explained: Wording.failureReason(failureKind).length > 0
    // The engine's own texts, each once and none the description already
    // holds.
    readonly property var engineErrors: {
        const description = ((record.report || {}).description || "").toLowerCase()
        const seen = []
        for (const e of ((record.report || {}).errors || [])) {
            const t = (e || "").trim()
            if (t.length === 0 || seen.includes(t)) continue
            if (description.includes(t.toLowerCase())) continue
            seen.push(t)
        }
        return seen
    }
    // Finished, the header below says it: the outcome, then what it was.
    title: hasReport ? "" : isBatch ? host.batchTitle(items, counts)
                                    : Wording.title(record)

    // How it went, as the desktop's Status Panel stamps it: the outcome as
    // the heading, what was done under it, and the report's seal in the
    // corner, a postmark tilted on the page.
    RowLayout {
        objectName: "statusStamp"
        Layout.fillWidth: true
        visible: sheet.hasReport
        spacing: 12

        readonly property var report: sheet.outcome || ({})

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: Wording.reportWord(parent.parent.report)
                color: Theme.reportColor(parent.parent.report,
                                         Material.secondaryTextColor)
                font.pixelSize: Theme.titleSize
                wrapMode: Text.Wrap
            }

            Label {
                objectName: "statusOperation"
                Layout.fillWidth: true
                // As the desktop's report heads it: the outcome, then which
                // operation.
                text: sheet.isBatch
                      ? qsTr("%1 · %2 files").arg(Wording.operationName(sheet.record.operation))
                                             .arg(sheet.items.length)
                      : Wording.operationName(sheet.record.operation)
                color: Material.secondaryTextColor
                font.pixelSize: Theme.bodySize
                wrapMode: Text.Wrap
            }
        }

        ReportSeal {
            objectName: "statusSeal"
            Layout.alignment: Qt.AlignTop | Qt.AlignRight
            // In the corner, as a stamp sits on an envelope; not past the
            // sheet's edge, which clips.
            Layout.rightMargin: -8
            Layout.preferredWidth: 124
            Layout.preferredHeight: 72
            kind: Theme.sealKind(parent.report)
            accent: Theme.reportColor(parent.report, Material.secondaryTextColor)
            text: Material.foreground
            // A quiet shade of the text over the page, as the desktop mixes it.
            muted: Theme.mix(Material.foreground, sheet.Material.backgroundColor, 0.6)
            dark: Theme.dark
            inscription: Wording.reportWord(parent.report).toUpperCase()
            Accessible.role: Accessible.Graphic
            Accessible.name: qsTr("Report status: %1").arg(Wording.reportWord(parent.report))
        }
    }

    // What it was for, and whether that is what is on the page now.
    Label {
        objectName: "statusTarget"
        Layout.fillWidth: true
        // A batch says how many in its title already; a text says which
        // text it was, on the page now or an earlier one, before a Retry.
        visible: !sheet.isBatch
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: {
            if (sheet.isBatch)
                return qsTr("%1 files").arg(sheet.items.length)
            const what = Wording.target(sheet.record)
            if (!sheet.isText || !sheet.host || !sheet.host.textSurface) return what
            const relation = sheet.host.textSurface.relation(sheet.record.operationId)
            return relation === "output" ? qsTr("The text on the page now")
                 : relation === "input" ? qsTr("Made from the text on the page now")
                                        : qsTr("An earlier text")
        }
    }

    // How it was asked: to whom, signed by whom -- as then, not as now.
    Label {
        objectName: "statusKeys"
        Layout.fillWidth: true
        visible: text.length > 0
        wrapMode: Text.Wrap
        text: sheet.host ? sheet.host.keysOf(sheet.record) : ""
    }

    // How the files ended, when not all alike: "1 done · 1 failed".
    Label {
        Layout.fillWidth: true
        visible: sheet.isBatch && sheet.counts.done !== sheet.counts.total
        wrapMode: Text.Wrap
        text: sheet.host ? sheet.host.countsText(sheet.counts) : ""
    }

    // Why it failed, when its kind is known: in a few words, then what
    // that means; the engine's words are under Details.
    Label {
        objectName: "statusReason"
        Layout.fillWidth: true
        visible: sheet.explained
        wrapMode: Text.Wrap
        font.weight: Font.DemiBold
        color: Theme.danger
        text: Wording.failureReason(sheet.failureKind)
    }

    Label {
        objectName: "statusDescription"
        Layout.fillWidth: true
        visible: !sheet.isBatch && text.length > 0
        wrapMode: Text.Wrap
        text: sheet.explained ? Wording.failureExplanation(sheet.failureKind)
              : (sheet.record.report || {}).description
                || Wording.reason(sheet.record.reason || "")
    }

    // Each signature: does it hold, what state is the key in, and was the
    // signer's identity weighed at all. Three lines, never folded into one.
    Repeater {
        model: sheet.isBatch ? [] : ((sheet.record.report || {}).signatures || [])

        Frame {
            required property var modelData

            Layout.fillWidth: true
            padding: 12

            GridLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                columns: 2
                columnSpacing: 12
                rowSpacing: 4

                Label {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    wrapMode: Text.WrapAnywhere
                    text: modelData.uid || modelData.keyId || modelData.fingerprint
                    font.weight: Font.DemiBold
                }

                Label {
                    text: qsTr("Signature")
                    color: Material.secondaryTextColor
                }
                Label {
                    objectName: "factSignature"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: Wording.signatureFact(modelData.signature)
                    color: Theme.signatureColor(modelData.signature)
                }

                Label {
                    text: qsTr("Key")
                    color: Material.secondaryTextColor
                }
                Label {
                    objectName: "factKey"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: Wording.keyStatusFact(modelData.keyStatus)
                    color: modelData.keyStatus === "ok" ? Material.foreground
                                                        : Theme.warning
                }

                Label {
                    text: qsTr("Identity")
                    color: Material.secondaryTextColor
                }
                Label {
                    objectName: "factIdentity"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    // Neutral, never green: nothing was weighed.
                    text: Wording.identityFact(modelData.identityTrust)
                    color: Material.secondaryTextColor
                }

                Label {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    visible: modelData.identityTrust === "not_assessed"
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.labelSize
                    color: Material.secondaryTextColor
                    text: qsTr("A valid signature proves the content was signed with "
                               + "this key. Compare the key's fingerprint with its "
                               + "owner to know whose it is.")
                }
            }
        }
    }

    Repeater {
        // A cancel says so in the stamp; the engine's word for it is not an
        // error. A failure already said in words is not said again.
        model: sheet.isBatch || sheet.record.phase === "cancelled"
               || sheet.explained ? [] : sheet.engineErrors

        Label {
            required property var modelData
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.danger
            text: modelData
        }
    }

    // What can be done with what it made, or about how it went.
    Flow {
        objectName: "statusActions"
        Layout.fillWidth: true
        // The first button's text in line with the sheet's.
        Layout.leftMargin: -12
        spacing: 8

        ResultAction {
            objectName: "statusSave"
            visible: !sheet.isText && !sheet.isBatch
                     && (sheet.record.outputs || []).length > 0
            text: qsTr("Save")
            icon.source: Theme.icon("save")
            onClicked: sheet.host.filesSurface.saveOutput(sheet.record.outputs[0])
        }
        ResultAction {
            visible: !sheet.isText && !sheet.active
            text: qsTr("Show in Files")
            icon.source: Theme.icon("folder")
            onClicked: {
                const outputs = sheet.record.outputs || []
                const target = outputs.length > 0 ? outputs[0]
                             : (sheet.record.targets || [{"fileId": -1}])[0].fileId
                sheet.close()
                sheet.host.showFileRequested(target)
            }
        }
        ResultAction {
            objectName: "statusRetry"
            visible: !sheet.isBatch && !sheet.active
                     && (sheet.record.phase === "failed"
                         || sheet.record.phase === "cancelled")
            enabled: !sheet.isText || sheet.textHeld()
            text: qsTr("Retry")
            onClicked: sheet.host.retry(sheet.record)
        }
        ResultAction {
            objectName: "statusSaveAll"
            visible: !sheet.isText && sheet.isBatch && !sheet.active
                     && sheet.batchOutputs().length > 0
            text: qsTr("Save all")
            icon.source: Theme.icon("save")
            onClicked: sheet.host.filesSurface.saveToFolder(sheet.batchOutputs())
        }
        ResultAction {
            objectName: "statusRetryFailed"
            visible: sheet.isBatch && !sheet.active
                     && (sheet.counts.failed + sheet.counts.cancelled) > 0
            text: qsTr("Retry the failed")
            onClicked: sheet.host.retryFailed(sheet.record.batchId)
        }
        ResultAction {
            objectName: "statusStop"
            visible: sheet.active && !sheet.isText && sheet.counts.queued > 0
            text: qsTr("Stop waiting files")
            flat: true
            onClicked: sheet.host.filesSurface.stopWaiting()
        }
        // The cards and the engine's report: hidden until asked for, and
        // put away again the same way. One of the row, as its neighbours.
        ResultAction {
            objectName: "statusMore"
            // Not "Details": that is the report's tab, below.
            text: sheet.expanded ? qsTr("Show less") : qsTr("Show more")
            icon.source: Theme.icon("info")
            onClicked: sheet.expanded = !sheet.expanded
        }
    }

    // Why a held-up button does nothing.
    Label {
        Layout.fillWidth: true
        // Beside the Retry it holds up, and only there.
        visible: sheet.isText && !sheet.active && !sheet.textHeld()
                 && (sheet.record.phase === "failed"
                     || sheet.record.phase === "cancelled")
        wrapMode: Text.Wrap
        font.pixelSize: Theme.labelSize
        color: Material.secondaryTextColor
        text: qsTr("This text is no longer kept. Run the operation again to "
                   + "get it back.")
    }

    // ---- In full, once expanded

    // A file of a batch its cards do not cover (one that failed, was
    // stopped or is still waiting), with how it went; the rest are in the
    // cards below, each under its name.
    Repeater {
        model: sheet.expanded && sheet.isBatch
               ? sheet.items.filter(item => ((item.report || {}).cards || [])
                                            .length === 0)
               : []

        ListRow {
            required property var modelData

            Layout.fillWidth: true
            Layout.leftMargin: -Theme.sheetPad
            Layout.rightMargin: -Theme.sheetPad
            leftPadding: Theme.sheetPad
            singleLineTitle: true
            symbol: modelData.phase === "done" ? "check_circle"
                    : modelData.phase === "failed" ? "error"
                    : modelData.phase === "cancelled" ? "info" : "draft"
            symbolColor: modelData.phase === "done" ? Theme.positive
                         : modelData.phase === "failed" ? Theme.danger
                                                        : Material.secondaryTextColor
            text: Wording.target(modelData)
            detail: modelData.phase === "failed"
                    ? (((modelData.report || {}).errors || [])[0]
                       || Wording.title(modelData))
                    : Wording.title(modelData)
            detailLines: 2
            // The file alone: why it failed, and its Retry (picked again
            // when it can no longer be opened). Back goes to the batch.
            onClicked: {
                sheet.single = true
                sheet.fromBatch = true
                sheet.shownId = modelData.operationId
            }
        }
    }

    OutputActions {
        id: reportOutput
    }

    // Status or Details, as the desktop's Status Panel has them: the
    // summary in cards, or the engine's own words. The style's tabs, on
    // the sheet's own surface.
    TabBar {
        id: showReport

        // Details, the engine's report, is shown.
        readonly property bool checked: currentIndex === 1

        objectName: "statusViews"
        Layout.fillWidth: true
        visible: sheet.expanded && sheet.reportText.length > 0
        Material.background: "transparent"
        onCurrentIndexChanged: reportScroll.contentY = 0

        TabButton {
            objectName: "statusViewStatus"
            text: qsTr("Status")
        }

        TabButton {
            objectName: "statusViewDetails"
            text: qsTr("Details")
        }
    }

    // The engine's own words for a failure already explained above, and
    // what it reported, in full: nothing of the diagnosis is lost.
    ColumnLayout {
        objectName: "statusTechnical"
        Layout.fillWidth: true
        visible: sheet.expanded && !showReport.checked && sheet.explained
        spacing: 2

        Label {
            Layout.fillWidth: true
            text: qsTr("Technical details")
            color: Material.secondaryTextColor
            font.pixelSize: Theme.labelSize
        }

        Repeater {
            model: sheet.explained
                   ? [(sheet.record.report || {}).description || ""]
                     .concat(sheet.engineErrors).filter(t => t.length > 0)
                   : []

            Label {
                required property var modelData
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.family: Theme.mono
                font.pixelSize: Theme.labelSize
                text: modelData
            }
        }
    }

    // What it was about, in cards as the desktop's Status Panel shows them:
    // each signature, recipient and the message's metadata, fact by fact.
    Repeater {
        // The cards or the engine's report: one at a time.
        model: sheet.expanded && !showReport.checked ? sheet.cards : []

        ResultCard {
            required property var modelData
            Layout.fillWidth: true
            model: modelData
        }
    }


    // The engine's report as the desktop's Status Panel shows it raw: its
    // own text, in the outcome's colour, monospaced, in a bordered box of the
    // page's base colour. A long one scrolls inside the box.
    Rectangle {
        // As tall as the report, up to a little under half the window: a
        // long one scrolls in the box, the sheet stays a sheet.
        readonly property real wanted: Math.min(report.implicitHeight + 24,
                                                sheet.windowHeight * 0.45)

        objectName: "statusReportBox"
        Layout.fillWidth: true
        Layout.preferredHeight: wanted
        visible: showReport.visible && showReport.checked
        radius: Theme.radiusSmall
        color: sheet.Material.backgroundColor
        border.width: 1
        border.color: Material.dividerColor

        Flickable {
            id: reportScroll

            anchors.fill: parent
            anchors.margins: 12
            // Clear of the copy button in the corner.
            anchors.rightMargin: copyReport.width
            contentHeight: report.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollIndicator.vertical: ScrollIndicator {}

            TextEdit {
                id: report
                objectName: "statusReport"
                width: reportScroll.width
                readOnly: true
                selectByMouse: false
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                color: Theme.reportColor(sheet.record.report || {}, Material.foreground)
                selectionColor: Material.accentColor
                font.family: Theme.mono
                font.pixelSize: Theme.labelSize
                text: sheet.reportText
            }
        }

        // The report is the one thing here worth copying: to paste where
        // someone asks what the engine said. It names keys and outcomes,
        // never the content, so it is copied as plain, not sensitive.
        BarButton {
            id: copyReport

            objectName: "statusCopyReport"
            anchors.top: parent.top
            anchors.right: parent.right
            text: qsTr("Copy the report")
            icon.source: Theme.icon("content_copy")
            onClicked: {
                reportOutput.copy(report.text, false)
                copiedTip.open()
            }

            // Said for a moment, then gone by itself.
            ToolTip {
                id: copiedTip

                objectName: "statusReportCopied"
                text: qsTr("Copied.")
                timeout: 1500
            }
        }
    }

    // No Close of its own: Back, a tap beside it or pulling it down put it
    // away, as every sheet does.
}
