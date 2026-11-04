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
// passphrase needs no keys and does not come here. What is chosen is the session's
// (WorkSelection) and stays chosen; Reset keys starts afresh.
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
    // The key category shown: "" for all keys.
    property string category: ""

    readonly property bool signing: step === 1
    readonly property bool encrypting: pending !== null && pending.op === "encrypt"
    // Encrypt & Sign: the recipients, then the key that signs.
    readonly property bool signsToo: encrypting && pending.signs === true
    // The signer list shows whenever the step is the signer's.
    readonly property bool signerList: signing
    // The reason the pending action cannot run yet; empty when it can.
    readonly property string blocking: pending === null ? ""
        : pending.op === "sign" ? WorkSelection.signBlock
                                : WorkSelection.encryptBlock
    // The recipients are enough to go on to signing.
    readonly property bool recipientsReady: WorkSelection.recipientCount > 0
        && WorkSelection.encryptBlock !== "no_recipients"
        && WorkSelection.encryptBlock !== "recipient_issue"

    signal openKeyRequested(var key)

    // The chosen keys shown as rows (WorkSelection.shownRecipientKeys: those
    // with a problem, then a few more), and how many are not.
    readonly property var chosenShown: WorkSelection.shownRecipientKeys
    readonly property int chosenHidden: WorkSelection.recipientCount
                                        - chosenShown.length

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
    }
    // Typing a search wants the room.
    Connections {
        target: search
        function onActiveFocusChanged() {
            if (search.activeFocus) sheet.expanded = true
        }
    }

    // What the action waiting for keys needs, said once.
    Label {
        Layout.fillWidth: true
        // Only what this step is about: the recipients first, then the signer.
        readonly property string concern: sheet.signing ? sheet.blocking
            : (sheet.recipientsReady ? "" : sheet.blocking)
        visible: concern.length > 0 && sheet.infoKey === null
        wrapMode: Text.Wrap
        color: Theme.warning
        text: Wording.block(concern)
    }

    // ---- Encrypt to

    ColumnLayout {
        Layout.fillWidth: true
        visible: !sheet.signing && sheet.infoKey === null
        spacing: 4

        // The chosen keys, each with what is wrong with it, if anything:
        // every one with a problem, and a few more; the rest are said in a
        // line, and ticked in the list below. Hundreds of rows, made again
        // on every tick, were what a long list of recipients cost.
        Repeater {
            model: sheet.chosenShown

            ListRow {
                required property var modelData

                Layout.fillWidth: true
                Layout.leftMargin: -24
                Layout.rightMargin: -24
                leftPadding: 24
                symbol: modelData.issue.length > 0 ? "warning" : "check_circle"
                symbolColor: modelData.issue.length > 0 ? Theme.danger
                                                        : Material.accentColor
                text: Wording.keyName(modelData)
                detail: modelData.issue.length > 0
                        ? Wording.keyIssue(modelData.issue)
                        : (modelData.email || "")
                detailColor: modelData.issue.length > 0
                             ? Theme.danger : Material.secondaryTextColor
                onClicked: if (modelData.issue !== "missing") sheet.infoKey = modelData

                BarButton {
                    text: qsTr("Remove %1").arg(Wording.keyName(modelData))
                    icon.source: Theme.icon("close")
                    onClicked: WorkSelection.removeRecipient(modelData.fingerprint)
                }
            }
        }

        Label {
            objectName: "moreRecipients"
            Layout.fillWidth: true
            visible: sheet.chosenHidden > 0
            wrapMode: Text.Wrap
            color: Material.secondaryTextColor
            text: qsTr("and %n more key(s), ticked below", "", sheet.chosenHidden)
        }
    }

    // ---- The keys to choose from

    ColumnLayout {
        Layout.fillWidth: true
        visible: sheet.infoKey === null && (!sheet.signing || sheet.signerList)
        spacing: 8

        SearchField {
            id: search
            objectName: "toolboxSearch"
            Layout.fillWidth: true
            hint: qsTr("Search name, email or key ID")
        }

        // The key categories, as the Keys page shows them: the person's own
        // first, then the ready-made ones that help here. A key is found
        // by the group it is in; the key database is the Keys page's.
        ListView {
            id: categoryChips

            objectName: "toolboxCategories"
            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            // Taller than its chips: the chosen one's shadow falls below it,
            // and the list clips what is outside.
            Layout.preferredHeight: 36 + 12
            orientation: ListView.Horizontal
            spacing: 8
            leftMargin: 24
            rightMargin: 24
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: [{"id": "", "name": qsTr("All keys"), "color": ""}]
                   .concat(categoryStore.categories)
                   // Signing lists only keys of one's own; encrypting never
                   // to an expired or revoked one.
                   .concat(sheet.signing ? [] : categoryStore.builtins.filter(
                       b => b.id !== "expired" && b.id !== "revoked"))

            delegate: Button {
                id: chip

                required property var modelData

                objectName: "toolboxCategory_" + (modelData.id || "all")
                // Near the top, the room for the shadow under it.
                y: 2
                text: modelData.name
                flat: sheet.category !== modelData.id
                highlighted: sheet.category === modelData.id
                checkable: true
                checked: sheet.category === modelData.id
                font.pixelSize: Theme.secondarySize
                topInset: 0
                bottomInset: 0
                implicitHeight: 36
                leftPadding: modelData.color ? 28 : 16
                onClicked: sheet.category = modelData.id
                Accessible.name: qsTr("Show %1").arg(modelData.name)

                Rectangle {
                    visible: chip.modelData.color.length > 0
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 10
                    height: 10
                    radius: 5
                    color: chip.modelData.color
                }
            }
        }

        KeyCategoriesController {
            id: categoryStore
            // A membership changed: the list follows. (Signals named
            // Signal... have no on-handler in QML.)
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

        // A list of its own, scrolling, that makes only the rows in view: a
        // thousand keys made a thousand rows when it was laid out in the
        // sheet's column. Its height is its content's, up to about half the
        // window; neither depends on the sheet's height, so it cannot loop.
        ListView {
            id: list

            objectName: "toolboxKeys"
            // Where in a long list one is; finding a key is the search's.
            ScrollIndicator.vertical: ScrollIndicator {}
            model: filtered
            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            Layout.preferredHeight: Math.min(
                contentHeight,
                // What the keyboard leaves: with it up the list is
                // shorter, so the field typed into stays in view.
                Math.max(Theme.rowHeightTwoLine * 2,
                         ((Window.height || 800) - Theme.keyboard) * 0.4))
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            reuseItems: true

            delegate: ItemDelegate {
                id: choice

                required property string name
                required property string email
                required property string keyId
                required property string fingerprint
                required property bool hasSecret
                required property bool canEncrypt
                required property bool canSign
                required property bool usable
                required property string status
                required property string statusText
                required property bool isKey
                required property var members
                required property var model

                // A key that cannot be used stays listed, saying why; so
                // does a key group with nobody in it.
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
                leftPadding: 24
                rightPadding: 4
                enabled: reason.length === 0
                Accessible.role: sheet.signing ? Accessible.RadioButton
                                               : Accessible.CheckBox
                Accessible.checked: chosen
                Accessible.name: Wording.keyTitle(name, email, keyId)
                Accessible.description: [email, keyId, reason]
                                        .filter(t => t.length > 0).join(", ")

                function toggle() {
                    if (sheet.signing) WorkSelection.setPreferredSigner(fingerprint)
                    else WorkSelection.toggleRecipient(fingerprint)
                }

                onClicked: toggle()

                contentItem: KeyChoiceFace {
                    opacity: choice.enabled ? 1 : 0.6
                    name: choice.name
                    email: choice.email
                    keyId: choice.keyId
                    isKey: choice.isKey
                    usable: choice.usable
                    members: choice.members
                    note: choice.reason
                    problem: choice.reason.length > 0

                    BarButton {
                        visible: choice.isKey
                        text: qsTr("About %1").arg(choice.name || choice.email)
                        icon.source: Theme.icon("info")
                        enabled: true
                        onClicked: sheet.infoKey = {
                            "name": choice.name, "email": choice.email,
                            "keyId": choice.keyId, "fingerprint": choice.fingerprint,
                            "hasSecret": choice.hasSecret,
                            "canEncrypt": choice.canEncrypt,
                            "canSign": choice.canSign, "usable": choice.usable,
                            "status": choice.status, "statusText": choice.statusText,
                            "created": choice.model.created,
                            "expires": choice.model.expires,
                            "algorithm": choice.model.algorithm,
                            "comment": choice.model.comment, "revoked": choice.model.revoked,
                            "expired": choice.model.expired
                        }
                    }

                    CheckBox {
                        visible: !sheet.signing
                        checked: choice.chosen
                        focusPolicy: Qt.NoFocus
                        onToggled: choice.toggle()
                    }

                    RadioButton {
                        visible: sheet.signing
                        checked: choice.chosen
                        focusPolicy: Qt.NoFocus
                        onToggled: choice.toggle()
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: Theme.gap
            Layout.bottomMargin: Theme.gap
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: Material.secondaryTextColor
            visible: filtered.count === 0
            text: search.text.length > 0
                  ? qsTr("No key matches “%1”.").arg(search.text)
                  : sheet.signing ? qsTr("You have no key of your own to sign with.")
                                  : qsTr("No keys yet. Import the keys of the "
                                         + "people you write to on the Keys page.")
        }
    }

    // ---- One key, in full: who it is, and what it can do

    ColumnLayout {
        Layout.fillWidth: true
        visible: sheet.infoKey !== null
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: -16
            spacing: 8

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

        Button {
            text: qsTr("Open on the Keys page")
            flat: true
            onClicked: {
                const key = sheet.infoKey
                sheet.close()
                sheet.openKeyRequested(key)
            }
        }
    }

    footer: [
        Button {
            objectName: "resetKeys"
            text: qsTr("Reset keys")
            flat: true
            visible: sheet.infoKey === null && !sheet.signing
            onClicked: WorkSelection.reset()
        },
        Button {
            objectName: "toolboxBack"
            text: qsTr("Back")
            flat: true
            visible: sheet.infoKey === null && sheet.signing && sheet.encrypting
            onClicked: sheet.step = 0
        },
        Item {
            Layout.fillWidth: true
        },
        Button {
            objectName: "toolboxDone"
            visible: sheet.pending === null
            text: qsTr("Done")
            highlighted: true
            onClicked: sheet.close()
        },
        // Encrypt & Sign: the recipients first, then on to the signer.
        Button {
            objectName: "toolboxNext"
            visible: sheet.signsToo && !sheet.signing
            text: qsTr("Next")
            highlighted: true
            enabled: sheet.recipientsReady
            onClicked: {
                search.clear()
                sheet.category = ""
                sheet.step = 1
            }
        },
        Button {
            objectName: "toolboxRun"
            visible: sheet.pending !== null
                     && (sheet.signing || !sheet.encrypting || !sheet.signsToo)
            text: sheet.pending ? sheet.pending.label : ""
            highlighted: true
            enabled: sheet.blocking.length === 0
            onClicked: sheet.runPending()
        }
    ]
}
