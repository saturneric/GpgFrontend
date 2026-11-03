// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.impl
import GpgFrontend.Mobile

// One of the application's line icons, by name, in one colour.
IconImage {
    property string symbol

    source: symbol.length > 0 ? Theme.icon(symbol) : ""
    sourceSize: Qt.size(24, 24)
}
