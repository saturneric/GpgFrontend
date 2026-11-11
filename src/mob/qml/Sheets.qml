// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton

import QtQuick

// The bottom sheets open, in the order they opened: the last one is the
// topmost, the one Back and typing are for. Kept by every BottomSheet as it
// opens and goes; read by the main page (Back, a share arriving, a key
// database switched). The application's state, not its looks (Theme).
QtObject {
    property var open: []

    // Closes every open sheet but one at work (it may not be let go) or one
    // that asks what only the person may answer (keepOpen: a passphrase);
    // true when such a sheet stays.
    function closeAll() {
        let stays = false
        for (const sheet of open.slice()) {
            if (sheet.interactive === false || sheet.keepOpen === true) stays = true
            else sheet.close()
        }
        return stays
    }

    // @p sheet on top: it is the last to have opened.
    function push(sheet) {
        open = open.filter(o => o !== sheet).concat([sheet])
    }

    // @p sheet gone, or going.
    function remove(sheet) {
        open = open.filter(o => o !== sheet)
    }
}
