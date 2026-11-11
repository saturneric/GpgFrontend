// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import GpgFrontend.Mobile

// A new category made with one key already in it: from the key's page and
// from the sheet of a key's categories, one way. Holds nothing on screen
// itself; its sheet asks for the name and colour.
Item {
    id: maker

    property KeyCategoriesController categories
    // The key that goes in (KeyRoleModel's keyId).
    property string keyId: ""
    // A sheet of this one's is asking: the next category added takes the key.
    property bool pending: false
    readonly property alias editor: editor

    function begin(key_id) {
        keyId = key_id
        pending = true
        editor.begin(null)
    }

    function added(id) {
        if (pending && keyId.length > 0) categories.setMember(id, keyId, true)
        pending = false
    }

    // Signals named Signal... have no handler property in QML. Once per
    // controller: an initial value raises no change.
    property QtObject listening: null
    function listen() {
        if (!categories || listening === categories) return
        if (listening) listening.SignalAdded.disconnect(maker.added)
        categories.SignalAdded.connect(maker.added)
        listening = categories
    }
    onCategoriesChanged: listen()
    Component.onCompleted: listen()
    Component.onDestruction: if (listening) listening.SignalAdded.disconnect(maker.added)

    CategorySheet {
        id: editor
        categories: maker.categories
        // Put away without making one: the next category is not this key's.
        onClosed: Qt.callLater(() => maker.pending = false)
    }
}
