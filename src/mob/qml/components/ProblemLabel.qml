// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GpgFrontend.Mobile

// What is wrong with what was typed into a sheet or a form: shown only while
// there is something to say.
Label {
    Layout.fillWidth: true
    visible: text.length > 0
    wrapMode: Text.Wrap
    color: Theme.danger
}
