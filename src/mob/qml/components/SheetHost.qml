// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import GpgFrontend.Mobile

// The two sheets every working page shares -- the keys for this work, and
// how operations went -- one at a time: opening one closes the other, and
// each keeps what it held. Also what decides whether a result is shown when
// its operation ends: when the person who asked for it still waits for it,
// the sheet opens; when they went on to something else, only the status
// line says so, and nothing takes the screen from them.
Item {
    id: host

    // The Text and Files pages, for what the status sheet's actions do with
    // their results: {holds, relation, retry} and {saveOutput,
    // saveToFolder, retry, retryFailed, stopWaiting, workspace}.
    property var textSurface: null
    property var filesSurface: null

    readonly property alias toolbox: toolbox
    readonly property alias status: status
    readonly property bool sheetOpen: toolbox.opened || status.opened

    // Bumped whenever the log changes, for what reads records by call.
    property int revision: 0
    // The batch asked for last, and whether the person still waits for it.
    property int tracked: 0
    property bool attentive: false
    // A batch that ended without being looked at; 0 when none.
    property int unseen: 0
    // The running operation has run long enough to be worth a line.
    property bool lineDue: false

    // The status line's operation: what runs, else what ended unseen.
    readonly property var lineRecord: {
        // Read again whenever the log changes.
        if (revision < 0) return ({})
        if (OperationQueue.current > 0) return OperationQueue.log.record(OperationQueue.current)
        if (unseen > 0) return OperationQueue.log.record(unseen)
        return ({})
    }
    // How the line's batch ended, as the status sheet's stamp says it: every
    // file, not the first.
    readonly property var lineOutcome: {
        if (revision < 0) return ({})
        const record = lineRecord
        if (record.operationId === undefined) return ({})
        const batch = OperationQueue.log.batch(record.batchId)
        if (batch.length <= 1) {
            if (Object.keys(record.report || {}).length > 0) return record.report
            if (record.phase === "cancelled") return {"cancelled": true}
            return record.phase === "failed" ? {"status": -1, "severity": "critical"}
                                             : ({})
        }
        const counts = countsOf(batch)
        if (counts.failed + counts.cancelled === 0) return {"status": 1, "severity": "ok"}
        if (counts.done > 0) return {"status": 0, "severity": "warn"}
        return counts.failed > 0 ? {"status": -1, "severity": "critical"}
                                 : {"cancelled": true}
    }
    readonly property bool lineShown: (OperationQueue.busy && lineDue)
                                      || (!OperationQueue.busy && unseen > 0
                                          && lineRecord.operationId !== undefined)
    readonly property string lineText: {
        if (revision < 0) return ""
        const record = lineRecord
        if (record.operationId === undefined) return ""
        const batch = OperationQueue.log.batch(record.batchId)
        const counts = countsOf(batch)
        const target = Wording.target(record)
        if (counts.active > 0) {
            if (record.phase === "waiting_passphrase")
                return qsTr("Waiting for the passphrase: %1").arg(target)
            if (batch.length > 1)
                return qsTr("%1 %2 of %3").arg(Wording.running(record.operation))
                       .arg(counts.total - counts.active + 1).arg(counts.total)
            return qsTr("%1 %2").arg(Wording.running(record.operation)).arg(target)
        }
        if (batch.length > 1) return batchTitle(batch, counts)
        return qsTr("%1 · %2").arg(Wording.title(record)).arg(target)
    }

    signal showFileRequested(int fileId)
    signal openKeyRequested(var key)

    function openKeys(which, pending) {
        status.close()
        toolbox.begin(which, pending)
    }

    // Operation @p id, its whole batch; with @p alone, that one only.
    function openStatus(id, alone) {
        if (!id || id <= 0) return
        toolbox.close()
        const record = OperationQueue.log.record(id)
        if (record.batchId !== undefined && record.batchId === unseen) unseen = 0
        status.show(id, alone === true)
    }

    function closeAll() {
        toolbox.close()
        status.close()
    }

    // Back, for whichever sheet is open; false when none is.
    function back() {
        if (toolbox.opened) {
            toolbox.back()
            return true
        }
        if (status.opened) {
            status.back()
            return true
        }
        return false
    }

    // The person asked for operation @p id and now waits for it.
    function track(id) {
        const record = OperationQueue.log.record(id)
        if (record.batchId === undefined) return
        tracked = record.batchId
        attentive = true
        if (unseen === tracked) unseen = 0
    }

    // The newest operation of @p surface ("text" or "files"), tracked.
    function trackLatest(surface) {
        track(OperationQueue.log.latest(surface))
    }

    // The person went on to something else: what ends is not pushed at them.
    function distract() {
        attentive = false
    }

    function dismissLine() {
        unseen = 0
    }

    // How many of @p records ended how.
    function countsOf(records) {
        const counts = {"total": records.length, "done": 0, "failed": 0,
                        "cancelled": 0, "queued": 0, "active": 0}
        for (const r of records) {
            if (r.phase === "done") ++counts.done
            else if (r.phase === "failed") ++counts.failed
            else if (r.phase === "cancelled") ++counts.cancelled
            else {
                ++counts.active
                if (r.phase === "queued") ++counts.queued
            }
        }
        return counts
    }

    function countsText(counts) {
        const parts = []
        if (counts.done > 0) parts.push(qsTr("%1 done").arg(counts.done))
        if (counts.failed > 0) parts.push(qsTr("%1 failed").arg(counts.failed))
        if (counts.cancelled > 0) parts.push(qsTr("%1 cancelled").arg(counts.cancelled))
        if (counts.active > 0) parts.push(qsTr("%1 to go").arg(counts.active))
        return parts.join(" · ")
    }

    function batchTitle(records, counts) {
        const op = records.length > 0 ? records[0].operation : ""
        if (counts.active > 0) return Wording.running(op)
        if (counts.failed + counts.cancelled === 0)
            return qsTr("%1: %2 files").arg(Wording.done(op)).arg(counts.done)
        return qsTr("%1: %2 of %3 files").arg(Wording.done(op)).arg(counts.done)
               .arg(counts.total)
    }

    // The keys of a record, as they were asked: to whom, signed by whom.
    function keysOf(record) {
        if (!record || record.operation === undefined) return ""
        if (record.operation === "encrypt") {
            if (record.symmetric) return qsTr("With a passphrase")
            const found = ((record.report || {}).recipients || [])
                          .map(r => r.uid || r.keyId)
            const to = found.length > 0 ? found.join(", ")
                     : (record.recipients || []).map(f => f.slice(-16)).join(", ")
            return (record.signer || "").length > 0
                   ? qsTr("To %1 · signed").arg(to) : qsTr("To %1").arg(to)
        }
        if (record.operation === "sign") {
            const signed = ((record.report || {}).signatures || [])
            // A sign result lists the new signature's maker, when it does.
            return signed.length > 0 && (signed[0].uid || "").length > 0
                   ? qsTr("Signed as %1").arg(signed[0].uid) : ""
        }
        return ""
    }

    function retry(record) {
        if (record.surface === "text") {
            if (textSurface && textSurface.retry(record.operationId)) status.close()
            return
        }
        // The sheet makes way: the new record is shown when it ends.
        if (filesSurface && filesSurface.retry(record.operationId)) status.close()
    }

    function retryFailed(batchId) {
        if (filesSurface && filesSurface.retryFailed(batchId)) status.close()
    }

    KeyToolbox {
        id: toolbox
        onOpenKeyRequested: key => host.openKeyRequested(key)
    }

    StatusSheet {
        id: status
        host: host
    }

    Timer {
        id: due
        interval: 400
        onTriggered: host.lineDue = OperationQueue.busy
    }

    Connections {
        target: OperationQueue.log
        function onDataChanged() { ++host.revision }
        function onRowsInserted() { ++host.revision }
        function onRowsRemoved() { ++host.revision }
    }

    function queueBusyChanged() {
        ++host.revision
        if (OperationQueue.busy) {
            host.lineDue = false
            due.restart()
        } else {
            due.stop()
            host.lineDue = false
        }
    }
    function queueFinished(id) {
        ++host.revision
        const record = OperationQueue.log.record(id)
        if (record.batchId === undefined || record.batchId !== host.tracked) return
        const counts = host.countsOf(OperationQueue.log.batch(record.batchId))
        if (counts.active > 0) return
        const batch = record.batchId
        host.tracked = 0
        if (host.attentive && !host.sheetOpen) host.openStatus(batch)
        else host.unseen = batch
    }

    // Signals named Signal... have no on-handler in QML, not even in a
    // Connections. The queue outlives the host: let go of it with the host.
    Component.onCompleted: {
        OperationQueue.SignalBusyChanged.connect(host.queueBusyChanged)
        OperationQueue.SignalFinished.connect(host.queueFinished)
    }
    Component.onDestruction: {
        OperationQueue.SignalBusyChanged.disconnect(host.queueBusyChanged)
        OperationQueue.SignalFinished.disconnect(host.queueFinished)
    }
}
