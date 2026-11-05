// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key group's name, new or to change, by the desktop's rule: five
// characters at least. Its keys are chosen after, on its page.
BottomSheet {
    id: sheet

    property KeyGroupsController groups
    // The group being renamed; empty for a new one.
    property string groupId: ""

    // A new group was made: its id.
    signal created(string id)

    function begin(group) {
        groupId = group ? group.id : ""
        name.text = group ? group.name : ""
        problem.text = ""
        open()
    }

    // The one thing to type: ready for it once open, not before. Opened
    // from another sheet, that one's closing would take the focus back.
    onOpened: name.forceActiveFocus()

    function save() {
        if (groupId.length > 0) {
            const why = groups.rename(groupId, name.text)
            problem.text = why
            if (why.length === 0) close()
            return
        }
        const why = groups.nameProblem(name.text)
        if (why.length > 0) {
            problem.text = why
            return
        }
        const id = groups.create(name.text)
        if (id.length === 0) {
            problem.text = qsTr("The group could not be made.")
            return
        }
        close()
        created(id)
    }

    objectName: "keyGroupSheet"
    title: groupId.length > 0 ? qsTr("Rename key group") : qsTr("New key group")
    description: groupId.length > 0 ? ""
                 : qsTr("Encrypting to a key group encrypts to every key in it. "
                        + "The desktop shows the same groups.")

    TextField {
        id: name
        objectName: "keyGroupName"
        Layout.fillWidth: true
        placeholderText: qsTr("Name, for example Project team")
        maximumLength: 64
        onAccepted: sheet.save()
    }

    ProblemLabel {
        id: problem
        objectName: "keyGroupProblem"
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Cancel")
            flat: true
            onClicked: sheet.close()
        },
        Button {
            objectName: "keyGroupSave"
            text: sheet.groupId.length > 0 ? qsTr("Save") : qsTr("Create")
            highlighted: true
            enabled: name.text.trim().length > 0
            onClicked: sheet.save()
        }
    ]
}
