// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import GpgFrontend.Mobile

// What is asked before an encryption to keys goes ahead, in the desktop's
// words (WorkSelection.encryptQuestions): several recipients, or post-quantum
// and classical ones together. An OperationFlow's ask.
MessageAlert {
    id: alert

    // Called when the person goes ahead; dropped otherwise.
    property var goOn: null

    function ask(question, onGoOn) {
        title = question.title
        message = question.text + "\n\n" + question.detail
        acceptText = question.accept
        goOn = onGoOn
        open()
    }

    objectName: "encryptQuestion"
    onAccepted: {
        const next = goOn
        goOn = null
        if (next) next()
    }
    onRejected: goOn = null
}
