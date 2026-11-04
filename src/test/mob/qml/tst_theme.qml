import QtQuick
import QtTest
import GpgFrontend.Mobile

// How much of the window the keyboard covers, which decides whether the
// sections' bar is shown.
TestCase {
    name: "Theme"

    function test_a_hidden_keyboard_covers_nothing() {
        compare(Theme.keyboardCover(false, Qt.rect(0, 1200, 1080, 1200), 800, 3), 0)
    }

    function test_a_docked_keyboard_covers_its_height() {
        // 2400 device pixels at 3x: an 800 high window, the lower 400 of it.
        compare(Theme.keyboardCover(true, Qt.rect(0, 1200, 1080, 1200), 800, 3), 400)
    }

    function test_no_keyboard_on_screen_covers_nothing() {
        // A hardware keyboard: the input method is visible, its rectangle
        // empty. Counted from y = 0 this used to cover the whole window.
        compare(Theme.keyboardCover(true, Qt.rect(0, 0, 0, 0), 800, 3), 0)
    }

    function test_never_more_than_the_keyboard_itself() {
        // A floating keyboard high up in the window.
        compare(Theme.keyboardCover(true, Qt.rect(0, 300, 900, 600), 800, 3), 200)
    }

    // The operations wear the desktop toolbar's icons.
    function test_operations_have_the_desktop_s_icons() {
        compare(Theme.operationIcon("encrypt"), "qrc:/icons/lock.png")
        compare(Theme.operationIcon("decrypt"), "qrc:/icons/unlock.png")
        compare(Theme.operationIcon("sign"), "qrc:/icons/signature.png")
        compare(Theme.operationIcon("verify"), "qrc:/icons/verify.png")
        compare(Theme.operationIcon("nothing"), "")
    }
}
