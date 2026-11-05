// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Every choice one key or subkey is made of, as the desktop's algorithm
// editor offers them: the algorithm, the second half of a hybrid, what it may
// be used for -- only what the algorithm lets change can be switched -- and
// how long it lasts. What is chosen is read from `choices`.
ColumnLayout {
    id: chooser

    // KeyGenProfilesController: what pairs with what, what may be used for what.
    property KeyGenProfilesController profiles
    // [{id, type, name, ...}] to choose from.
    property var algos: []
    property bool subkey: false
    // [{id, name}], as KeyActionsController.validities gives them.
    property var validities: []
    property bool showUsage: true
    property bool showValidity: true

    property var algo: algos.length > 0 ? ({"id": algos[0].id, "type": algos[0].type}) : ({})
    property var pair: ({})
    property string validity: "2y"
    property var usage: ({})

    // The hybrid's possible second halves, and what this algorithm allows.
    readonly property var pairings: profiles && algo.id ? profiles.pairings(algo) : []
    readonly property var allowed: profiles && algo.id ? profiles.usage(algo, subkey) : ({})
    // A subkey is for something: one with no usage would be of no use
    // (a primary key always certifies, so it may have none of these).
    readonly property bool hasUsage: usage.encrypt === true || usage.sign === true
                                     || usage.authenticate === true
    readonly property bool usageMissing: subkey && showUsage && algo.id !== undefined
                                         && !hasUsage
    readonly property bool complete: algo.id !== undefined
                                     && (pairings.length === 0 || pair.id !== undefined)
                                     && !usageMissing
    readonly property var choices: ({"algo": algo, "pair": pair, "usage": usage,
                                     "validity": validity})

    // Choose @p ref ({id, type}), with its own defaults for what follows.
    function choose(ref) {
        algo = ref
        const pairs = profiles ? profiles.pairings(ref) : []
        pair = pairs.length > 0 ? {"id": pairs[0].id, "type": pairs[0].type} : ({})
        const a = profiles ? profiles.usage(ref, subkey) : ({})
        usage = {"encrypt": a.encrypt === true, "sign": a.sign === true,
                 "authenticate": a.authenticate === true}
    }

    function indexIn(list, ref) {
        for (let i = 0; i < list.length; ++i) {
            if (list[i].id === ref.id && (!ref.type || list[i].type === ref.type)) return i
        }
        return -1
    }

    function setUsage(key, on) {
        const next = Object.assign({}, usage)
        next[key] = on
        usage = next
    }

    Component.onCompleted: if (algo.id) choose(algo)
    onAlgosChanged: if (indexIn(algos, algo) < 0 && algos.length > 0)
                        choose({"id": algos[0].id, "type": algos[0].type})

    spacing: 4

    component Caption: Label {
        Layout.fillWidth: true
        Layout.topMargin: 4
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
    }

    Caption {
        text: qsTr("Algorithm")
    }

    ChoiceBox {
        objectName: "algoChoice"
        Layout.fillWidth: true
        model: chooser.algos
        textRole: "name"
        currentIndex: count > 0 ? chooser.indexIn(chooser.algos, chooser.algo) : -1
        onActivated: index => chooser.choose({"id": chooser.algos[index].id,
                                             "type": chooser.algos[index].type})
        Accessible.name: qsTr("Algorithm")
    }

    Caption {
        visible: chooser.pairings.length > 0
        text: qsTr("Paired with")
    }

    ChoiceBox {
        objectName: "pairChoice"
        Layout.fillWidth: true
        visible: chooser.pairings.length > 0
        model: chooser.pairings
        textRole: "name"
        currentIndex: count > 0 ? chooser.indexIn(chooser.pairings, chooser.pair) : -1
        onActivated: index => chooser.pair = {"id": chooser.pairings[index].id,
                                              "type": chooser.pairings[index].type}
        Accessible.name: qsTr("Paired with")
    }

    Label {
        Layout.fillWidth: true
        visible: chooser.pairings.length > 0
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
        text: qsTr("A hybrid combines a post-quantum algorithm with a classic "
                   + "one: it holds as long as either does.")
    }

    Caption {
        visible: chooser.showUsage
        text: qsTr("Usage")
    }

    Flow {
        objectName: "usageChoice"
        Layout.fillWidth: true
        visible: chooser.showUsage
        spacing: 0

        Repeater {
            model: [
                {"key": "encrypt", "change": "changeEncrypt", "text": qsTr("Encrypt")},
                {"key": "sign", "change": "changeSign", "text": qsTr("Sign")},
                {"key": "authenticate", "change": "changeAuthenticate",
                 "text": qsTr("Authenticate")}
            ]

            CheckBox {
                required property var modelData
                text: modelData.text
                checked: chooser.usage[modelData.key] === true
                enabled: chooser.allowed[modelData.change] === true
                onToggled: chooser.setUsage(modelData.key, checked)
            }
        }
    }

    ProblemLabel {
        objectName: "usageProblem"
        text: chooser.usageMissing ? qsTr("Choose at least one usage.") : ""
    }

    Caption {
        visible: chooser.showValidity
        text: qsTr("Expires after")
    }

    ChoiceBox {
        objectName: "validityChoice"
        Layout.fillWidth: true
        visible: chooser.showValidity
        model: chooser.validities
        textRole: "name"
        valueRole: "id"
        currentIndex: count > 0 ? indexOfValue(chooser.validity) : -1
        onActivated: chooser.validity = currentValue
        Accessible.name: qsTr("Expires after")
    }
}
