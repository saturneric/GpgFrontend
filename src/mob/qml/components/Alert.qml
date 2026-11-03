// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import GpgFrontend.Mobile

// A question that has to be answered before going on: the style's dialog, in
// the middle of the window.
Dialog {
    anchors.centerIn: parent
    // Qt keeps a popup clear of its margins, so the dialog moves up out of the
    // keyboard's way and its buttons stay in sight.
    bottomMargin: Theme.keyboard + 8
    width: Math.min(parent.width - 48, 400)
    modal: true
    // Back answers the dialog, not the page under it.
    focus: true
}
