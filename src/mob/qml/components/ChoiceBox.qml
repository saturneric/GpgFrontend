// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import GpgFrontend.Mobile

// A choice from a short list in a sheet's form: the style's drop-down, a
// touch target tall (48) but drawn less (40), as the rows around it are;
// the style's own is a 56 tall form field, the tallest thing in a sheet.
// Placed without a width, it is as wide as its longest option.
ComboBox {
    implicitContentWidthPolicy: ComboBox.WidestText
    implicitHeight: Theme.touch
    topInset: 4
    bottomInset: 4
    font.pixelSize: Theme.secondarySize
}
