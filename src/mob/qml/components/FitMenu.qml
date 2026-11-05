// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import GpgFrontend.Mobile

// A menu as wide as its widest row needs, and never wider than the window
// allows. The style's Menu does not size itself to its items (its implicit
// width is only its background's, 200): longer names, as translations have,
// were cut short ("Verschlüsseln & Si…").
Menu {
    id: menu

    readonly property real widest: {
        let result = 0
        for (let i = 0; i < count; ++i) {
            const item = itemAt(i)
            // A row with no height is one not in use.
            if (item && item.visible && item.height > 0)
                result = Math.max(result, item.implicitWidth)
        }
        return result
    }

    // Clear of the window's edges, as the platform's menus are.
    leftMargin: Theme.gapSmall
    rightMargin: Theme.gapSmall
    width: Math.min(Math.max(implicitWidth, widest + leftPadding + rightPadding),
                    (Overlay.overlay ? Overlay.overlay.width : 360) - 2 * Theme.gap)
}
