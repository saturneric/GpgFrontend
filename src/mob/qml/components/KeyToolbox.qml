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
    // A key shown in full, over the list; null for the list. Shown with the
    // sheet at its full height: at half of it the key's facts went on below
    // the foot, with nothing saying they did.
    property var infoKey: null
    onInfoKeyChanged: if (infoKey !== null && canExpand) expanded = true
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
        selected.close()
    }
    // Typing a search wants the room.
    Connections {
        target: search
        function onActiveFocusChanged() {
            if (search.activeFocus) sheet.expanded = true
        }
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

    // What the action waiting for keys needs, said once.
    Label {
        objectName: "toolboxConcern"
        Layout.fillWidth: true
        // Only what this step is about: the recipients first, then the signer.
        readonly property string concern: sheet.signing ? sheet.blocking
            : (sheet.recipientsReady ? "" : sheet.blocking)
        visible: concern.length > 0 && sheet.infoKey === null
        wrapMode: Text.Wrap
        color: Theme.warning
        font.pixelSize: Theme.secondarySize
        text: Wording.block(concern)
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

    // What is chosen, in one line; tapped, the chosen keys to take one out.
    ItemDelegate {
        id: summary

        objectName: "selectedSummary"
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        leftPadding: Theme.sheetPad
        rightPadding: 8
        topPadding: 4
        bottomPadding: 4
        visible: !sheet.signing && sheet.infoKey === null
                 && WorkSelection.recipientCount > 0
        text: Wording.selectedCount(WorkSelection.recipientCount)
        onClicked: selected.open()
        Accessible.description: issues > 0 ? qsTr("Some cannot be used") : ""

        // Ordinary state in ordinary words; a warning only when a chosen
        // key cannot be used, saying how many.
        readonly property int issues: WorkSelection.recipientIssueCount

        contentItem: RowLayout {
            spacing: 8

            Icon {
                visible: summary.issues > 0
                symbol: "warning"
                color: Theme.danger
                sourceSize: Qt.size(20, 20)
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Label {
                    Layout.fillWidth: true
                    text: summary.text
                    elide: Text.ElideRight
                    font.pixelSize: Theme.secondarySize
                    font.weight: Font.Medium
                }

                Label {
                    objectName: "selectedProblem"
                    Layout.fillWidth: true
                    visible: summary.issues > 0
                    text: qsTr("%n of them cannot be used. Tap to see why.", "",
                               summary.issues)
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.labelSize
                    color: Theme.danger
                }
            }

            Icon {
                symbol: "expand_more"
                color: Material.secondaryTextColor
                sourceSize: Qt.size(20, 20)
            }
        }
    }

    // The signer, while it is the step: the one chosen, in one line.
    Label {
        objectName: "signerSummary"
        Layout.fillWidth: true
        visible: sheet.signing && sheet.infoKey === null
                 && WorkSelection.preferredSigner.length > 0
        text: qsTr("Signing as %1").arg(Wording.keyName(WorkSelection.signerKey))
        elide: Text.ElideRight
        font.pixelSize: Theme.secondarySize
        color: Material.secondaryTextColor
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

        delegate: ItemDelegate {
            id: choice

            required property string name
            required property string email
            required property string keyId
            required property string fingerprint
            required property string secretState
            required property string capabilities
            required property bool hasSecret
            required property bool canEncrypt
            required property bool canSign
            required property bool usable
            required property string status
            required property string statusText
            required property bool isKey
            required property var members
            required property var model

            // A key that cannot be used stays listed, saying why; so does a
            // key group with nobody in it.
            readonly property string reason: !usable ? statusText
                : !isKey && (members || []).length === 0
                  ? Wording.keyIssue("empty_group")
                : sheet.signing ? (canSign ? "" : Wording.keyIssue("cannot_sign"))
                                : (canEncrypt ? "" : Wording.keyIssue("cannot_encrypt"))
            readonly property bool chosen: sheet.signing
                ? WorkSelection.preferredSigner.toUpperCase() === fingerprint.toUpperCase()
                : WorkSelection.revision >= 0
                  && WorkSelection.isRecipient(fingerprint)

            width: ListView.view.width
            leftPadding: Theme.sheetPad
            rightPadding: 4
            topPadding: 6
            bottomPadding: 6
            enabled: reason.length === 0
            Accessible.role: sheet.signing ? Accessible.RadioButton
                                           : Accessible.CheckBox
            Accessible.checked: chosen
            Accessible.name: Wording.keyTitle(name, email, keyId)
            Accessible.description: [email, keyId, reason,
                                     isKey ? qsTr("Long-press for the key's details") : ""]
                                    .filter(t => t.length > 0).join(", ")

            function toggle() {
                if (sheet.signing) WorkSelection.setPreferredSigner(fingerprint)
                else WorkSelection.toggleRecipient(fingerprint)
            }

            // The whole row chooses; a long press shows the key in full,
            // once the finger is lifted: shown at once, the list went while
            // the row was still pressed, and the row stayed pressed.
            property bool held: false

            onClicked: toggle()
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

            contentItem: KeyChoiceFace {
                idBelow: true
                opacity: choice.enabled ? 1 : 0.6
                name: choice.name
                email: choice.email
                keyId: choice.keyId
                isKey: choice.isKey
                usable: choice.usable
                members: choice.members
                secretState: choice.secretState
                capabilities: choice.capabilities
                note: choice.reason
                problem: choice.reason.length > 0

                // One mark at the end, showing the choice; a tap on it is a
                // tap on the row (never a second toggle).
                CheckBox {
                    visible: !sheet.signing
                    checkable: false
                    checked: choice.chosen
                    focusPolicy: Qt.NoFocus
                    onClicked: choice.toggle()
                    Accessible.ignored: true
                }

                RadioButton {
                    visible: sheet.signing
                    checkable: false
                    checked: choice.chosen
                    focusPolicy: Qt.NoFocus
                    onClicked: choice.toggle()
                    Accessible.ignored: true
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
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: -12
                spacing: 4

                BarButton {
                    objectName: "toolboxInfoBack"
                    text: qsTr("Back to the list")
                    icon.source: Theme.icon("arrow_back")
                    onClicked: sheet.infoKey = null
                }

                Label {
                    Layout.fillWidth: true
                    text: Wording.keyName(sheet.infoKey)
                    elide: Text.ElideRight
                    font.pixelSize: Theme.headlineSize
                }
            }

            KeyFacts {
                Layout.fillWidth: true
                key: sheet.infoKey || ({})
            }

            ActionButton {
                kind: "text"
                text: qsTr("Open on the Keys page")
                onClicked: {
                    const key = sheet.infoKey
                    sheet.close()
                    sheet.openKeyRequested(key)
                }
            }
        }
    }

    footer: [
        ActionButton {
            objectName: "resetKeys"
            kind: "text"
            text: qsTr("Reset keys")
            visible: sheet.infoKey === null && !sheet.signing
            enabled: WorkSelection.recipientCount > 0
            onClicked: WorkSelection.reset()
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
        ActionButton {
            objectName: "toolboxDone"
            visible: sheet.pending === null
            text: qsTr("Done")
            onClicked: sheet.close()
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
