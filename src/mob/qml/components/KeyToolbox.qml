// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The keys an operation needs, asked for after the operation is chosen:
// for Encrypt, who can read it; for Encrypt & Sign, who can read it, then
// the key that signs; for Sign, the key that signs. Whether to sign is the
// operation's, chosen in the menu, never asked here. Encrypting with a
// passphrase needs no keys and does not come here. What is chosen is the
// session's (WorkSelection) and stays chosen; Reset keys starts afresh.
//
// The keys are one list that fills the sheet and scrolls on its own; the
// sheet is dragged taller by its header. What is chosen is said in one line
// ("3 keys selected"), which opens them in a sheet of their own to take one
// out. Searching and choosing a category never drop what is chosen.
//
// Opened by an operation, it offers to run it ("Encrypt now"); closed any
// other way, the operation is dropped.
BottomSheet {
    id: sheet

    // What an action that waits for keys runs once they are chosen:
    // {op, signs, label, run()}; null when the toolbox was only opened.
    // An encryption that signs (Encrypt & Sign) has signs true.
    property var pending: null
    // 0: who to encrypt to; 1: the signing key.
    property int step: 0
    // A key shown in full, over the list; null for the list.
    property var infoKey: null
    // While a key is shown: the sheet as tall as its facts, and expanded
    // when they need more than half the window (at half, long facts went on
    // below the foot with nothing saying they did). Back to the list, the
    // list as it was.
    Binding {
        target: sheet
        property: "expanded"
        when: sheet.infoKey !== null
        value: sheet.canExpand
        restoreMode: Binding.RestoreValue
    }
    // The key category shown: "" for all keys.
    property string category: ""

    readonly property bool signing: step === 1
    // The signer list shows whenever the step is the signer's.
    readonly property bool signerList: signing
    readonly property bool encrypting: pending !== null && pending.op === "encrypt"
    // Encrypt & Sign: the recipients, then the key that signs.
    readonly property bool signsToo: encrypting && pending.signs === true
    // The reason the pending action cannot run yet; empty when it can.
    readonly property string blocking: pending === null ? ""
        : pending.op === "sign" ? WorkSelection.signBlock
                                : WorkSelection.encryptBlock
    // The recipients are enough to go on to signing.
    readonly property bool recipientsReady: WorkSelection.recipientCount > 0
        && WorkSelection.encryptBlock !== "no_recipients"
        && WorkSelection.encryptBlock !== "recipient_issue"

    // The categories offered here: the person's own, then the ready-made
    // ones that help -- signing lists only keys of one's own; encrypting
    // never to an expired or revoked one.
    readonly property var categoryEntries: categoryStore.categories.concat(
        sheet.signing ? [] : categoryStore.builtins.filter(
                                 b => b.id !== "expired" && b.id !== "revoked"))
    // The search field (KeyFinder's).
    readonly property alias search: finder.field

    readonly property alias keyList: list
    readonly property alias selectedSheet: selected
    // The key shown in full, as the key's page reads it: its parts, each
    // with what it does itself.
    readonly property var infoDetails: infoKey !== null && (infoKey.fingerprint || "").length > 0
                                       ? describer.describe(infoKey.fingerprint) : ({})

    signal openKeyRequested(var key)

    // Open on @p which ("encrypt" or "sign"), for @p action (or null).
    function begin(which, action) {
        step = which === "sign" ? 1 : 0
        pending = action || null
        infoKey = null
        category = ""
        search.text = ""
        open()
    }

    function runPending() {
        const action = pending
        pending = null
        close()
        if (action) action.run()
    }

    objectName: "keyToolbox"
    expandable: true
    listMode: true
    title: !signing ? qsTr("Encrypt to") : qsTr("Sign with")
    stepBack: function() {
        if (infoKey !== null) {
            infoKey = null
            return true
        }
        if (search.text.length > 0) {
            search.clear()
            return true
        }
        // From signing back to the recipients, while encrypting.
        if (signing && encrypting) {
            step = 0
            return true
        }
        return false
    }
    onClosed: {
        pending = null
        infoKey = null
        search.text = ""
        // Opened again, it starts on the list, not in the search with the
        // keyboard up because the search had the focus when it closed.
        search.focus = false
        selected.close()
    }
    // Typing a search wants the room.
    Connections {
        target: search
        function onActiveFocusChanged() {
            if (search.activeFocus) sheet.expanded = true
        }
    }

    KeyActionsController {
        id: describer
    }

    KeyCategoriesController {
        id: categoryStore
        // A membership changed: the list follows. (Signals named Signal...
        // have no on-handler in QML.)
        Component.onCompleted: SignalChanged.connect(filtered.refreshCategory)
    }

    KeyFilterModel {
        id: filtered
        sourceModel: KeyListController.keys
        searchText: search.text
        onlySecret: sheet.signing
        onlySignCapable: sheet.signing
        category: sheet.category
        // A key group is a recipient too, never a signer.
        withGroups: !sheet.signing
    }

    // Finding a key: by what it says, and by its category.
    KeyFinder {
        id: finder

        Layout.fillWidth: true
        visible: sheet.infoKey === null
        searchName: "toolboxSearch"
        categoryName: "toolboxCategory"
        entries: sheet.categoryEntries
        category: sheet.category
        onCategoryChosen: id => sheet.category = id
    }

    // What is chosen, in one row that is always there and always as tall
    // (none, one or many chosen, the list under it never moves), with what
    // still holds the action up under it: the recipients first, then the
    // signer. Tapped, the chosen recipients, to take one out.
    KeyChoiceSummary {
        id: summary

        objectName: "selectedSummary"
        visible: sheet.infoKey === null
        // Only what this step is about.
        readonly property string concern: sheet.signing ? sheet.blocking
            : (sheet.recipientsReady ? "" : sheet.blocking)
        // A chosen key that cannot be used, saying how many.
        readonly property int issues: sheet.signing ? 0 : WorkSelection.recipientIssueCount
        readonly property int count: WorkSelection.recipientCount

        enabled: !sheet.signing && count > 0
        text: sheet.signing
              ? (WorkSelection.preferredSigner.length > 0
                 ? qsTr("Signing as %1").arg(Wording.keyName(WorkSelection.signerKey))
                 : qsTr("No signing key chosen yet"))
              : count > 0 ? Wording.selectedCount(count) : qsTr("No keys chosen yet")
        detail: issues > 0 ? qsTr("%n of them cannot be used. Tap to see why.", "", issues)
              : concern.length > 0 ? Wording.block(concern) : ""
        detailColor: issues > 0 ? Theme.danger : Theme.warning
        symbol: issues > 0 ? "warning"
              : (sheet.signing ? WorkSelection.preferredSigner.length > 0 : count > 0)
                ? "check_circle" : ""
        symbolColor: issues > 0 ? Theme.danger : Material.accentColor
        onClicked: selected.open()

        Icon {
            visible: summary.enabled
            symbol: "expand_more"
            color: Material.secondaryTextColor
            sourceSize: Qt.size(20, 20)
        }
    }

    // The keys: a list of its own that fills the sheet and makes only the
    // rows in view (a thousand keys made a thousand rows in a column).
    ListView {
        id: list

        objectName: "toolboxKeys"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        // What it would take at most, for the sheet's own height: its rows,
        // and the room at its end.
        Layout.preferredHeight: Math.max(contentHeight, Theme.touch * 2) + bottomMargin
        Layout.minimumHeight: Theme.touch * 2
        visible: sheet.infoKey === null
        model: filtered
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        reuseItems: true
        // The last row comes clear of the sheet's foot.
        bottomMargin: Theme.gapSmall
        // Where in a long list one is; finding a key is the search's.
        ScrollIndicator.vertical: ScrollIndicator {}

        delegate: KeyChoiceRow {
            id: choice

            required property string fingerprint
            required property bool hasSecret
            required property bool canEncrypt
            required property bool canSign
            required property string status
            required property string statusText
            required property string encryptNote
            required property string signNote
            required property var model

            // A key that cannot be used stays listed, saying why; so does a
            // key group with nobody in it. The reasons are the desktop's.
            readonly property string reason: sheet.signing ? signNote : encryptNote

            chosen: sheet.signing
                ? WorkSelection.preferredSigner.toUpperCase() === fingerprint.toUpperCase()
                : WorkSelection.revision >= 0
                  && WorkSelection.isRecipient(fingerprint)
            isKey: model.isKey
            usable: model.usable
            members: model.members
            exclusive: sheet.signing
            note: reason
            problem: reason.length > 0
            // One that cannot be used is not chosen; one chosen before it
            // could no longer be is let go of here, not only in the summary.
            enabled: reason.length === 0 || chosen
            Accessible.description: [email, keyId, reason,
                                     isKey ? qsTr("Long-press for the key's details") : ""]
                                    .filter(t => t.length > 0).join(", ")

            onChooseRequested: {
                if (sheet.signing) WorkSelection.setPreferredSigner(fingerprint)
                else WorkSelection.toggleRecipient(fingerprint)
            }

            // A long press shows the key in full, once the finger is
            // lifted: shown at once, the list went while the row was still
            // pressed, and the row stayed pressed.
            property bool held: false

            onPressAndHold: held = isKey
            onCanceled: held = false
            onReleased: {
                if (!held) return
                held = false
                showInfo()
            }

            function showInfo() {
                sheet.infoKey = {
                    "name": name, "email": email, "keyId": keyId,
                    "fingerprint": fingerprint, "hasSecret": hasSecret,
                    "secretState": secretState, "capabilities": capabilities,
                    "canEncrypt": canEncrypt,
                    "canSign": canSign, "usable": usable, "status": status,
                    "statusText": statusText, "created": model.created,
                    "expires": model.expires, "algorithm": model.algorithm,
                    "comment": model.comment, "revoked": model.revoked,
                    "expired": model.expired
                }
            }
        }

        Label {
            objectName: "toolboxEmpty"
            anchors.fill: parent
            anchors.margins: Theme.gap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            color: Material.secondaryTextColor
            visible: filtered.count === 0
            text: search.text.length > 0
                  ? qsTr("No key matches “%1”.").arg(search.text)
                  : sheet.category.length > 0 ? qsTr("No keys in this category.")
                  : sheet.signing ? qsTr("You have no key of your own to sign with.")
                                  : qsTr("No keys yet. Import the keys of the "
                                         + "people you write to on the Keys page.")
        }
    }

    // ---- One key, in full: who it is, and what it can do
    Flickable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: info.implicitHeight
        visible: sheet.infoKey !== null
        contentHeight: info.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollIndicator.vertical: ScrollIndicator {}

        ColumnLayout {
            id: info

            width: parent.width
            spacing: Theme.gapSmall

            // Back, then who the key is: its name, its address under it.
            RowLayout {
                Layout.fillWidth: true
                // The arrow's own room, not the sheet's inset, at the left.
                Layout.leftMargin: -12
                spacing: 4

                BarButton {
                    objectName: "toolboxInfoBack"
                    Layout.alignment: Qt.AlignTop
                    text: qsTr("Back to the list")
                    icon.source: Theme.icon("arrow_back")
                    onClicked: sheet.infoKey = null
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 0

                    Label {
                        objectName: "toolboxInfoName"
                        Layout.fillWidth: true
                        text: Wording.keyName(sheet.infoKey)
                        elide: Text.ElideRight
                        font.pixelSize: Theme.bodySize
                        font.weight: Font.Medium
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: sheet.infoKey && (sheet.infoKey.name || "").length > 0
                              ? (sheet.infoKey.email || "") : ""
                        elide: Text.ElideRight
                        font.pixelSize: Theme.secondarySize
                        color: Material.secondaryTextColor
                    }
                }
            }

            KeyFacts {
                objectName: "toolboxInfoFacts"
                Layout.fillWidth: true
                key: sheet.infoKey || ({})
                parts: sheet.infoDetails.subkeys || []
            }
        }
    }

    footer: [
        // At the foot, with the sheet's other buttons: never under them.
        ActionButton {
            objectName: "toolboxOpenKey"
            kind: "text"
            visible: sheet.infoKey !== null
            text: qsTr("Open on the Keys page")
            onClicked: {
                const key = sheet.infoKey
                sheet.close()
                sheet.openKeyRequested(key)
            }
        },
        ActionButton {
            objectName: "resetKeys"
            kind: "text"
            text: qsTr("Reset keys")
            visible: sheet.infoKey === null && !sheet.signing
            enabled: WorkSelection.recipientCount > 0
            // The chosen keys only: whether the operation signs is the
            // operation's (Encrypt & Sign), so it still asks for a signer.
            onClicked: WorkSelection.clearRecipients()
        },
        ActionButton {
            objectName: "toolboxBack"
            kind: "text"
            text: qsTr("Back")
            visible: sheet.infoKey === null && sheet.signing && sheet.encrypting
            onClicked: sheet.step = 0
        },
        Item {
            Layout.fillWidth: true
        },
        // Encrypt & Sign: the recipients first, then on to the signer.
        ActionButton {
            objectName: "toolboxNext"
            visible: sheet.signsToo && !sheet.signing
            text: qsTr("Next")
            enabled: sheet.recipientsReady
            onClicked: {
                search.clear()
                sheet.category = ""
                sheet.step = 1
            }
        },
        ActionButton {
            objectName: "toolboxRun"
            visible: sheet.pending !== null
                     && (sheet.signing || !sheet.encrypting || !sheet.signsToo)
            text: sheet.pending ? sheet.pending.label : ""
            enabled: sheet.blocking.length === 0
            onClicked: sheet.runPending()
        }
    ]

    SelectedKeysSheet {
        id: selected
        // The key in full, over the list: this sheet makes way for it (it
        // was left over the details, which then seemed not to open). A key
        // group has no details of its own, as with a long press.
        onAboutKeyRequested: key => {
            if (key.isKey === false) return
            selected.close()
            sheet.infoKey = key
        }
    }
}
