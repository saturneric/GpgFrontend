// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key group's name, email and comment, new or to change, as the
// desktop's dialogs have them: a name of five characters at least, an email
// and a comment if wanted, the email only one that looks like one. Its keys
// are chosen after, on its page.
BottomSheet {
    id: sheet

    property KeyGroupsController groups
    // The group being edited; empty for a new one.
    property string groupId: ""

    // A new group was made: its id.
    signal created(string id)

    function begin(group) {
        groupId = group ? group.id : ""
        name.text = group ? group.name : ""
        email.text = group ? (group.email || "") : ""
        comment.text = group ? (group.comment || "") : ""
        nameProblem.text = ""
        emailProblem.text = ""
        problem.text = ""
        open()
    }

    // The one thing to type: ready for it once open, not before. Opened
    // from another sheet, that one's closing would take the focus back.
    onOpened: name.forceActiveFocus()

    function save() {
        // What is wrong, under the field it is about.
        const why = groups.problemOf(name.text, email.text)
        nameProblem.text = why.field === "name" ? why.text : ""
        emailProblem.text = why.field === "email" ? why.text : ""
        problem.text = ""
        if (why.field.length > 0) return
        if (groupId.length > 0) {
            const failed = groups.edit(groupId, name.text, email.text, comment.text)
            problem.text = failed
            if (failed.length === 0) close()
            return
        }
        const id = groups.create(name.text, email.text, comment.text)
        if (id.length === 0) {
            problem.text = qsTr("The group could not be made.")
            return
        }
        close()
        created(id)
    }

    objectName: "keyGroupSheet"
    title: groupId.length > 0 ? qsTr("Edit key group") : qsTr("New key group")
    description: groupId.length > 0 ? ""
                 : qsTr("Encrypting to a key group encrypts to every key in it. "
                        + "The desktop shows the same groups.")

    TextField {
        id: name
        objectName: "keyGroupName"
        Layout.fillWidth: true
        placeholderText: qsTr("Name, for example Project team")
        maximumLength: 64
        onAccepted: email.forceActiveFocus()
    }

    ProblemLabel {
        id: nameProblem
        objectName: "keyGroupNameProblem"
    }

    TextField {
        id: email
        objectName: "keyGroupEmail"
        Layout.fillWidth: true
        placeholderText: qsTr("Email (optional)")
        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase
                          | Qt.ImhNoPredictiveText
        maximumLength: 128
        onAccepted: comment.forceActiveFocus()
    }

    ProblemLabel {
        id: emailProblem
        objectName: "keyGroupEmailProblem"
    }

    TextField {
        id: comment
        objectName: "keyGroupComment"
        Layout.fillWidth: true
        placeholderText: qsTr("Comment (optional)")
        maximumLength: 128
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
