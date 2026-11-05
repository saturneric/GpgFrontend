// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

Page {
    id: page

    // The key's roles, as KeysPage passes them.
    property var key: ({})

    // Everything about the key, as the desktop's details show it
    // (KeyActionsController.describe()); empty while the key cannot be read,
    // and then the list's roles above stand in.
    property var details: ({})
    readonly property bool described: details.fingerprint !== undefined
    readonly property var can: details.can || ({})
    readonly property var subkeys: details.subkeys || []
    readonly property var uids: details.uids || []

    // While a key is being changed, deleted or exported, the page stays:
    // leaving it would drop the answer, and with it the refresh of the list.
    readonly property bool busy: actions.busy

    signal back()
    // The key is gone; the list wants a refresh.
    signal deleted()

    // Read the key again, as it is now.
    function reload() {
        details = actions.describe(key.fingerprint || "")
    }

    onKeyChanged: reload()

    function finished(action, succeeded, detail) {
        // The sheet it was asked from says how it went, and keeps what was
        // typed for another try.
        if (action === "add_uid") addUidSheet.settle(succeeded, detail)
        if (action === "add_subkey") addSubkeySheet.settle(succeeded, detail)
        if (!succeeded) {
            // Stopped by the person: nothing to say. Otherwise what the
            // engine reported, never a guess; in the sheet it was asked
            // from, when there is one.
            if (detail.reason === "cancelled") notice.show("")
            else if (action !== "add_uid" && action !== "add_subkey")
                notice.show(detail.error)
            return
        }
        switch (action) {
        case "export":
            exported = detail.armored
            exportedName = detail.fileName || "public.asc"
            notice.show("")
            exportActions.open()
            return
        case "delete":
            page.deleted()
            return
        }
        // A change to the key: shown at once, and in the list.
        reload()
        KeyListController.refresh()
        const said = {
            "add_subkey": qsTr("Subkey added."),
            "expiration": qsTr("Expiration changed."),
            "revoke_subkey": qsTr("Subkey revoked."),
            "delete_subkey": qsTr("Subkey deleted."),
            "add_uid": qsTr("User ID added."),
            "primary_uid": qsTr("Primary user ID changed."),
            "revoke_uid": qsTr("User ID revoked."),
            "delete_uid": qsTr("User ID deleted."),
            "passphrase": qsTr("Passphrase changed.")
        }
        notice.show(said[action] || "")
    }

    // What of the key's secret is here (KeyRoleModel's secretState): the
    // list's word, else the key's own, read as the list reads it.
    readonly property string secretState: (key.secretState || "").length > 0
        ? key.secretState : (details.secretState || "")

    function shared(outcome) {
        notice.showShared(outcome,
                          qsTr("This key could not be shared. Copy it instead."))
    }

    // The public key, armored, once exported, and the name of its file.
    property string exported: ""
    property string exportedName: ""

    header: AppBar {
        title: qsTr("Key")
        canGoBack: !page.busy
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
            width: parent.width
            spacing: 0

            // Whose key it is, and its state, in one line each.
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.gap
                spacing: Theme.gap

                KeyAvatar {
                    Layout.alignment: Qt.AlignTop
                    size: 56
                    name: page.key.name || page.key.email || ""
                    usable: page.key.usable === true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        objectName: "keyName"
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.titleSize
                        text: page.key.name || page.key.email || page.key.keyId || ""
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WrapAnywhere
                        visible: (page.key.name || "").length > 0
                                 && (page.key.email || "").length > 0
                        color: Material.secondaryTextColor
                        text: page.key.email || ""
                    }

                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 6

                        Chip {
                            visible: (page.key.status || "ok") !== "ok"
                            text: page.key.statusText || ""
                            tone: page.key.status === "expiring_soon"
                                  ? Theme.warning : Theme.danger
                        }

                        // What the whole key can do, as the list says it;
                        // each part's own is under Keys below.
                        Chip {
                            objectName: "keyCapabilities"
                            visible: text.length > 0
                            text: page.key.capabilities || ""
                            font.family: Theme.mono
                            tone: Material.secondaryTextColor
                            Accessible.name: Wording.capabilityWords(text)
                        }

                        // GnuPG's mark for what of the key is here.
                        Chip {
                            objectName: "keyTag"
                            visible: text.length > 0
                            text: Wording.keyTag(page.secretState)
                            tone: page.secretState === "none" ? Material.secondaryTextColor
                                                              : Material.accentColor
                            Accessible.name: Wording.keyTagDescription(page.secretState)
                        }
                    }
                }
            }

            // Before the key is read (or where it cannot be): what the list
            // knows of it.
            KeyFacts {
                id: keyFacts
                Layout.fillWidth: true
                Layout.margins: Theme.gap
                visible: !page.described
                key: page.key
            }

            // ---- Owner, as on the desktop's first tab
            SectionHeader {
                Layout.fillWidth: true
                visible: page.described
                text: qsTr("Owner")
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                visible: page.described
                columns: 1
                rowSpacing: 8

                Fact {
                    title: qsTr("Name")
                    value: page.details.name || ""
                }
                Fact {
                    title: qsTr("Email")
                    value: page.details.email || ""
                }
                Fact {
                    title: qsTr("Comment")
                    value: page.details.comment || ""
                }
            }

            // ---- The primary key
            SectionHeader {
                Layout.fillWidth: true
                visible: page.described
                text: qsTr("Primary key")
            }

            FingerprintView {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                visible: page.described
                fingerprint: page.details.fingerprint || ""
            }

            KeyPartFacts {
                objectName: "primaryKeyFacts"
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                Layout.topMargin: 8
                visible: page.described
                // The primary key's own usage, not the whole key's: an
                // Ed25519 primary key does not encrypt, its subkey does.
                part: page.subkeys.length > 0
                      ? Object.assign({}, page.details, {"usage": page.subkeys[0].usage})
                      : page.details
                kindTitle: qsTr("Key format")
                kindValue: page.details.version > 0 ? qsTr("Version %1").arg(page.details.version) : ""
                expiresText: page.described ? (Theme.date(page.details.expires) || qsTr("Never")) : ""
                expiresColor: page.details.expired === true ? Theme.danger : Material.foreground
                lastUpdate: Theme.date(page.details.lastUpdate)
                secretText: page.details.hasSecret !== true ? qsTr("Not here")
                          : page.details.primarySecret === true ? qsTr("On this device")
                          : qsTr("Subkeys only: the primary key's is not here")
                cardText: page.details.cardKey === true ? qsTr("Some parts are on a smart card") : ""
            }

            // ---- User IDs
            SectionHeader {
                Layout.fillWidth: true
                visible: page.described
                text: qsTr("User IDs (%1)").arg(page.uids.length)
            }

            Repeater {
                model: page.uids

                ListRow {
                    required property var modelData

                    objectName: "uid_" + index
                    required property int index
                    Layout.fillWidth: true
                    symbol: "info"
                    onClicked: uidSheet.begin(modelData, page.can)
                    text: modelData.name || modelData.uid
                    detail: [modelData.email || "", modelData.comment || ""]
                            .filter(t => t.length > 0).join(" · ")

                    Chip {
                        visible: modelData.primary === true
                        text: qsTr("Primary")
                        tone: Material.accentColor
                    }
                    Chip {
                        visible: modelData.revoked === true || modelData.invalid === true
                        text: modelData.revoked === true ? qsTr("Revoked") : qsTr("Invalid")
                        tone: Theme.danger
                    }
                }
            }

            Button {
                objectName: "addUid"
                Layout.leftMargin: Theme.gap
                visible: page.can.addUid === true
                enabled: !actions.busy
                flat: true
                text: qsTr("Add user ID")
                icon.source: Theme.icon("add")
                onClicked: addUidSheet.begin()
            }

            // ---- The primary key and its subkeys, each a tap from its details
            SectionHeader {
                Layout.fillWidth: true
                visible: page.described
                text: qsTr("Keys (%1)").arg(page.subkeys.length)
            }

            Repeater {
                objectName: "subkeyList"
                model: page.subkeys

                ListRow {
                    required property var modelData

                    objectName: "subkey_" + modelData.index
                    Layout.fillWidth: true
                    symbol: modelData.primary === true ? "key"
                          : modelData.usage.encrypt === true ? "lock"
                          : modelData.usage.sign === true ? "signature" : "verified"
                    symbolColor: modelData.revoked === true || modelData.expired === true
                                 ? Theme.danger : Material.secondaryTextColor
                    text: (modelData.primary === true ? qsTr("Primary key") : qsTr("Subkey"))
                          + " · " + (modelData.keyId || "").slice(-8)
                    detail: [modelData.algorithm, Wording.usage(modelData.usage),
                             Wording.keyState(modelData)]
                            .filter(t => t && t.length > 0).join(" · ")
                    detailColor: modelData.revoked === true || modelData.expired === true
                                 ? Theme.danger : Material.secondaryTextColor
                    detailLines: 2
                    onClicked: subkeySheet.begin(modelData, page.can)

                    // GnuPG's mark: sec/pub, ssb/sub, by its secret here.
                    Label {
                        // The primary key's mark is the key's (sec# when
                        // only its subkeys' secrets are here, sec> on a card).
                        text: modelData.primary === true
                              ? Wording.keyTag(page.secretState)
                              : Wording.subkeyTag(modelData.secret === true,
                                                  modelData.card === true)
                        font.family: Theme.mono
                        font.pixelSize: Theme.labelSize
                        color: modelData.secret === true ? Material.accentColor
                                                         : Material.secondaryTextColor
                    }

                    Icon {
                        symbol: "chevron_right"
                        color: Material.secondaryTextColor
                    }
                }
            }

            Button {
                objectName: "addSubkey"
                Layout.leftMargin: Theme.gap
                visible: page.can.addSubkey === true
                enabled: !actions.busy
                flat: true
                text: qsTr("Add subkey")
                icon.source: Theme.icon("add")
                onClicked: addSubkeySheet.begin()
            }

            // ---- The categories the key is in, each a tap to change
            SectionHeader {
                Layout.fillWidth: true
                visible: (page.key.keyId || "").length > 0
                text: qsTr("Categories")
            }

            Flow {
                objectName: "keyCategories"
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                visible: (page.key.keyId || "").length > 0
                spacing: 8

                Repeater {
                    model: categories.categories

                    CategoryTag {
                        required property var modelData
                        // Read again whenever a membership changes.
                        readonly property bool member: categories.revision >= 0
                            && categories.categoriesOf(page.key.keyId || "")
                                         .indexOf(modelData.id) >= 0

                        objectName: "keyCategory_" + modelData.id
                        name: modelData.name
                        tone: modelData.color
                        checked: member
                        onClicked: categories.setMember(modelData.id, page.key.keyId,
                                                        !member)
                        Accessible.name: member ? qsTr("Take out of %1").arg(modelData.name)
                                                : qsTr("Put in %1").arg(modelData.name)
                    }
                }

                Button {
                    objectName: "newKeyCategory"
                    flat: true
                    text: qsTr("New category")
                    icon.source: Theme.icon("label")
                    font.pixelSize: Theme.secondarySize
                    topInset: 4
                    bottomInset: 4
                    onClicked: {
                        page.addToNew = true
                        categorySheet.begin(null)
                    }
                }
            }

            // ---- What can be done with the key, apart from what it is
            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Operations")
            }

            ColumnLayout {
                objectName: "keyOperations"
                Layout.fillWidth: true
                spacing: 0

                ListRow {
                    objectName: "exportKey"
                    Layout.fillWidth: true
                    enabled: !actions.busy
                    symbol: "share"
                    text: qsTr("Share public key")
                    onClicked: actions.exportPublic(page.key.fingerprint)
                }

                ListRow {
                    objectName: "changeExpiration"
                    Layout.fillWidth: true
                    visible: page.can.setExpiration === true
                    enabled: !actions.busy
                    symbol: "settings"
                    text: qsTr("Change expiration")
                    detail: qsTr("Of the primary key, and so of the whole key.")
                    onClicked: page.askExpiration({"primary": true,
                                                   "fingerprint": page.details.fingerprint})
                }

                ListRow {
                    objectName: "changePassphrase"
                    Layout.fillWidth: true
                    visible: page.can.changePassphrase === true
                    enabled: !actions.busy
                    symbol: "visibility_off"
                    text: qsTr("Change passphrase")
                    detail: qsTr("You are asked for the current one, then the new one.")
                    onClicked: actions.changePassphrase(page.key.fingerprint)
                }
            }

            // Apart from everything else, and last: what cannot be undone.
            MenuSeparator {
                Layout.fillWidth: true
                Layout.topMargin: 8
            }

            ItemDelegate {
                objectName: "deleteKey"
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.gap
                leftPadding: Theme.gap + 4
                text: qsTr("Delete key")
                enabled: !actions.busy
                icon.source: Theme.icon("delete")
                icon.color: Theme.danger
                Material.foreground: Theme.danger
                onClicked: confirmDelete.open()
            }
        }
    }

    Snackbar {
        id: notice
        objectName: "keyNotice"
    }

    KeyActionsController {
        id: actions
    }

    OutputActions {
        id: output
    }

    Component.onCompleted: {
        reload()
        categories.SignalAdded.connect(page.categoryAdded)
        actions.SignalFinished.connect(page.finished)
        output.SignalShared.connect(page.shared)
    }

    // What to do with the exported public key.
    BottomSheet {
        id: exportActions

        title: qsTr("Public key")
        inset: 0

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.sheetPad
            Layout.rightMargin: Theme.sheetPad
            wrapMode: Text.Wrap
            color: Material.secondaryTextColor
            text: qsTr("Give it to the people who want to encrypt to this key "
                       + "or check its signatures. It holds nothing secret.")
        }

        // As a file first: an .asc attachment others can save and import,
        // named after the key; its text, to paste, apart.
        ListRow {
            objectName: "shareKeyFile"
            Layout.fillWidth: true
            leftPadding: Theme.sheetPad
            visible: output.canShare
            symbol: "share"
            symbolColor: Material.accentColor
            text: qsTr("Share as a file")
            detail: page.exportedName
            onClicked: {
                output.shareFile(page.exported, page.exportedName, "application/pgp-keys")
                exportActions.close()
            }
        }

        ListRow {
            objectName: "copyKeyText"
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.gapSmall
            leftPadding: Theme.sheetPad
            symbol: "content_copy"
            text: qsTr("Copy the key as text")
            onClicked: {
                output.copy(page.exported, false)
                notice.show(qsTr("Copied."))
                exportActions.close()
            }
        }
    }

    // A category made from this page takes this key in at once.
    property bool addToNew: false

    KeyCategoriesController {
        id: categories
    }

    function categoryAdded(id) {
        if (page.addToNew) categories.setMember(id, page.key.keyId, true)
        page.addToNew = false
    }

    CategorySheet {
        id: categorySheet
        categories: categories
        onClosed: Qt.callLater(() => page.addToNew = false)
    }

    // The subkey (or primary key) whose expiry is being chosen.
    property var expiring: ({})

    function askExpiration(subkey) {
        expiring = subkey
        validitySheet.begin(qsTr("Change expiration"),
                            subkey.primary === true
                            ? qsTr("How long the key lasts, from today.")
                            : qsTr("How long subkey %1 lasts, from today.")
                              .arg((subkey.keyId || "").slice(-8)))
    }

    SubkeySheet {
        id: subkeySheet
        busy: actions.busy
        keySecretState: page.secretState
        onExpirationRequested: subkey => page.askExpiration(subkey)
        onRevokeRequested: subkey => revokeSheet.begin(subkey)
        onDeleteRequested: function(subkey) {
            confirmDeleteSubkey.subkey = subkey
            confirmDeleteSubkey.open()
        }
    }

    ValiditySheet {
        id: validitySheet
        choices: actions.validities
        onAccepted: validity => actions.setExpiration(
                        page.details.fingerprint,
                        page.expiring.primary === true ? "" : page.expiring.fingerprint,
                        validity)
    }

    AddSubkeySheet {
        id: addSubkeySheet
        busy: actions.busy
        validities: actions.validities
        fingerprint: page.details.fingerprint || ""
        onCustomAccepted: (choices, withPassphrase) => actions.addCustomSubkey(
                              page.details.fingerprint, choices, withPassphrase)
        onAccepted: (purpose, validity, withPassphrase) => actions.addSubkey(
                        page.details.fingerprint, purpose, validity, withPassphrase)
    }

    RevokeSubkeySheet {
        id: revokeSheet
        onAccepted: function(item, reason, text) {
            if (revokeSheet.forUid)
                actions.revokeUid(page.details.fingerprint, item.uid, reason, text)
            else
                actions.revokeSubkey(page.details.fingerprint, item.fingerprint, reason, text)
        }
    }

    UidSheet {
        id: uidSheet
        busy: actions.busy
        onPrimaryRequested: uid => actions.setPrimaryUid(page.details.fingerprint, uid.uid)
        onRevokeRequested: uid => revokeSheet.beginForUid(uid)
        onDeleteRequested: function(uid) {
            confirmDeleteUid.uid = uid
            confirmDeleteUid.open()
        }
    }

    AddUidSheet {
        id: addUidSheet
        busy: actions.busy
        onAccepted: (name, email, comment) => actions.addUid(
                        page.details.fingerprint, name, email, comment)
    }

    MessageAlert {
        id: confirmDeleteUid

        property var uid: ({})

        objectName: "confirmDeleteUid"
        title: qsTr("Delete this user ID?")
        message: qsTr("“%1” is deleted from this device only. Copies of the key "
                   + "others have keep it: to withdraw it, revoke it instead.")
                 .arg(uid.uid || "")
        acceptText: qsTr("Delete")
        onAccepted: actions.deleteUid(page.details.fingerprint, uid.uid)
    }

    MessageAlert {
        id: confirmDeleteSubkey

        property var subkey: ({})

        objectName: "confirmDeleteSubkey"
        title: qsTr("Delete subkey %1?").arg((subkey.keyId || "").slice(-8))
        message: qsTr("It is deleted from this device only. Copies of the key "
                   + "others have keep it: to end its use, revoke it instead. "
                   + "This cannot be undone.")
        acceptText: qsTr("Delete")
        onAccepted: actions.deleteSubkey(page.details.fingerprint, subkey.fingerprint)
    }

    MessageAlert {
        id: confirmDelete

        objectName: "confirmDelete"
        title: qsTr("Delete this key?")

        message: page.key.hasSecret
              ? qsTr("This is your own key. Without it you can no "
                     + "longer read messages encrypted to it or sign "
                     + "with it. This cannot be undone.")
              : qsTr("You will no longer be able to encrypt to this "
                     + "key or check its signatures, until you import "
                     + "it again.")

        acceptText: qsTr("Delete")
        acceptObjectName: "confirmDeleteRun"

        onAccepted: actions.remove(page.key.fingerprint)
    }
}
