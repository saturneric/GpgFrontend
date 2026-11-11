// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtTest
import GpgFrontend.Mobile

// The prompts that take a secret, as the person meets them: the profile's
// PIN, asked for by the thread opening the profile, and a key's passphrase,
// asked for by the core while an operation runs. Ready to type into when
// they come up; Enter gives the answer; only a deliberate choice turns them
// down, never a tap beside them.
Item {
    id: root

    width: 400
    height: 800

    PinSheet {
        id: pinSheet
    }

    PassphraseSheet {
        id: passphraseSheet
    }

    ConfirmSheet {
        id: confirmSheet
    }

    TestCase {
        name: "Secrets"
        when: windowShown

        function findByName(parent, name) {
            for (let i = 0; i < parent.children.length; ++i) {
                const child = parent.children[i]
                if (child.objectName === name) return child
                const found = findByName(child, name)
                if (found) return found
            }
            return null
        }

        // The item the window's keys go to is @p item.
        function focused(item) {
            return root.Window.activeFocusItem === item
        }

        // @p ask (answer, passphraseAnswer) comes to @p expected.
        function answered(ask, expected) {
            tryVerify(() => ask() === expected, 3000, "answer " + ask())
        }

        function typeText(text) {
            for (const c of text) keyClick(c)
        }

        // ---- The PIN

        function openPin(failures) {
            TestSupport.askPin(false, failures)
            tryVerify(() => pinSheet.opened, 3000, "the PIN prompt came up")
            return findByName(pinSheet.contentItem, "pinField")
        }

        // Up with the field ready; Enter unlocks with what was typed.
        function test_the_pin_is_typed_and_entered() {
            const field = openPin(0)
            tryVerify(() => focused(field), 2000, "the PIN field has the keys")
            typeText("2468")
            keyClick(Qt.Key_Return)
            tryVerify(() => TestSupport.answer() !== undefined, 3000)
            const answer = TestSupport.answer()
            compare(answer.action, "pin")
            compare(answer.pin, "2468")
            tryVerify(() => !pinSheet.visible)
            // Nothing typed stays behind in the field.
            compare(field.text, "")
        }

        // A tap beside it, Escape or Back do not answer it.
        function test_the_pin_prompt_stays_until_answered() {
            openPin(0)
            mouseClick(root, 5, 5)
            keyClick(Qt.Key_Escape)
            keyClick(Qt.Key_Back)
            wait(200)
            verify(pinSheet.visible)
            compare(TestSupport.answer(), undefined)
            // Quit is the safe answer.
            const quit = findByName(pinSheet.contentItem, "pinQuit")
            verify(quit, "the Quit button")
            mouseClick(quit)
            answered(() => TestSupport.answer(), "cancelled")
        }

        // A new PIN typed twice: two that differ are said to, and cannot be
        // set; the same twice can.
        function test_two_different_pins_are_said_to_differ() {
            TestSupport.askPin(true, 0)
            tryVerify(() => pinSheet.opened, 3000, "the PIN prompt came up")
            const pin = findByName(pinSheet.contentItem, "pinField")
            const repeat = findByName(pinSheet.contentItem, "pinRepeat")
            const problem = findByName(pinSheet.contentItem, "pinMismatch")
            verify(repeat.visible)
            pin.text = "24682468"
            verify(!problem.visible)
            repeat.text = "24682469"
            verify(problem.visible)
            repeat.text = "24682468"
            verify(!problem.visible)
            mouseClick(findByName(pinSheet.contentItem, "pinQuit"))
            answered(() => TestSupport.answer(), "cancelled")
        }

        // A new PIN as short as the desktop refuses one: said so, and it
        // cannot be set, the same twice or not.
        function test_a_short_new_pin_cannot_be_set() {
            TestSupport.askPin(true, 0)
            tryVerify(() => pinSheet.opened, 3000, "the PIN prompt came up")
            const pin = findByName(pinSheet.contentItem, "pinField")
            const repeat = findByName(pinSheet.contentItem, "pinRepeat")
            const problem = findByName(pinSheet.contentItem, "pinMismatch")
            const set = findByName(pinSheet.contentItem, "pinUnlock")
            verify(set, "the Set PIN button")
            compare(InteractionBridge.minPinLength, 8)
            pin.text = "1234"
            repeat.text = "1234"
            verify(problem.visible)
            verify(!set.enabled)
            pin.text = "12345678"
            repeat.text = "12345678"
            verify(!problem.visible)
            verify(set.enabled)
            mouseClick(findByName(pinSheet.contentItem, "pinQuit"))
            answered(() => TestSupport.answer(), "cancelled")
        }

        // After a few wrong ones, a way out for a forgotten PIN.
        function test_a_forgotten_pin_can_be_reset_after_failures() {
            openPin(0)
            verify(!findByName(pinSheet.contentItem, "forgotPin").visible)
            mouseClick(findByName(pinSheet.contentItem, "pinQuit"))
            tryVerify(() => !pinSheet.visible)
            openPin(3)
            verify(findByName(pinSheet.contentItem, "forgotPin").visible)
            mouseClick(findByName(pinSheet.contentItem, "pinQuit"))
            tryVerify(() => !pinSheet.visible)
        }

        // ---- Forcing open, resetting

        function test_the_safe_choice_declines_a_confirmation_data() {
            return [{"tag": "force open", "force": true},
                    {"tag": "reset key", "force": false}]
        }

        // The safe choice changes nothing; a tap beside it, Escape or Back
        // do not answer it at all.
        function test_the_safe_choice_declines_a_confirmation(data) {
            TestSupport.askConfirmation(data.force)
            tryVerify(() => confirmSheet.opened, 3000)
            mouseClick(root, 5, 5)
            keyClick(Qt.Key_Escape)
            keyClick(Qt.Key_Back)
            wait(200)
            verify(confirmSheet.visible)
            compare(TestSupport.answer(), undefined)
            mouseClick(findByName(confirmSheet.contentItem, "safeChoice"))
            answered(() => TestSupport.answer(), "cancelled")
            tryVerify(() => !confirmSheet.visible)
        }

        // Only the deliberate choice goes ahead.
        function test_the_destructive_choice_confirms() {
            TestSupport.askConfirmation(true)
            tryVerify(() => confirmSheet.opened, 3000)
            mouseClick(findByName(confirmSheet.contentItem, "destructiveChoice"))
            answered(() => TestSupport.answer(), true)
            tryVerify(() => !confirmSheet.visible)
        }

        // ---- A passphrase

        function openPassphrase() {
            TestSupport.askPassphrase()
            tryVerify(() => passphraseSheet.opened, 3000, "the passphrase prompt came up")
            return findByName(passphraseSheet.contentItem, "passphraseField")
        }

        function test_the_passphrase_is_typed_and_entered() {
            const field = openPassphrase()
            tryVerify(() => focused(field), 2000, "the passphrase field has the keys")
            typeText("correct horse")
            keyClick(Qt.Key_Return)
            answered(() => TestSupport.passphraseAnswer(), "answered:correct horse")
            tryVerify(() => !passphraseSheet.visible)
            compare(field.text, "")
        }

        // A tap beside it leaves it up; Back turns it down, as Cancel does.
        function test_back_cancels_the_passphrase_a_tap_beside_does_not() {
            openPassphrase()
            mouseClick(root, root.width / 2, 20)
            wait(200)
            verify(passphraseSheet.opened)
            compare(TestSupport.passphraseAnswer(), "waiting")
            keyClick(Qt.Key_Back)
            answered(() => TestSupport.passphraseAnswer(), "cancelled")
            tryVerify(() => !passphraseSheet.visible)
        }

        // Two operations ask at once: turning the first down by Back leaves
        // the second asked, the sheet comes back up for it.
        function test_a_second_request_is_not_left_unseen() {
            openPassphrase()
            const second = TestSupport.askPassphrase()
            keyClick(Qt.Key_Back)
            answered(() => TestSupport.passphraseAnswer(second - 1), "cancelled")
            compare(TestSupport.passphraseAnswer(second), "waiting")
            tryVerify(() => passphraseSheet.opened, 3000, "up again for the second")
            mouseClick(findByName(passphraseSheet.contentItem, "passphraseCancel"))
            answered(() => TestSupport.passphraseAnswer(second), "cancelled")
            tryVerify(() => !passphraseSheet.visible)
        }

        // OK tapped twice answers once: the second request waiting behind is
        // not answered with nothing.
        function test_ok_twice_answers_once() {
            const field = openPassphrase()
            const second = TestSupport.askPassphrase()
            tryVerify(() => focused(field), 2000)
            typeText("first")
            const ok = findByName(passphraseSheet.contentItem, "passphraseOk")
            ok.clicked()
            ok.clicked()
            answered(() => TestSupport.passphraseAnswer(second - 1), "answered:first")
            wait(100)
            compare(TestSupport.passphraseAnswer(second), "waiting")
            verify(passphraseSheet.opened)
            // Nothing typed: no answer from the keyboard either.
            keyClick(Qt.Key_Return)
            wait(100)
            compare(TestSupport.passphraseAnswer(second), "waiting")
            keyClick(Qt.Key_Back)
            answered(() => TestSupport.passphraseAnswer(second), "cancelled")
            tryVerify(() => !passphraseSheet.visible)
        }

        // A new passphrase, typed twice: the keyboard's action goes on to the
        // second field, and from there answers.
        function test_enter_goes_on_then_answers() {
            TestSupport.askPassphrase(true)
            tryVerify(() => passphraseSheet.opened, 3000)
            const field = findByName(passphraseSheet.contentItem, "passphraseField")
            const repeat = findByName(passphraseSheet.contentItem, "passphraseRepeat")
            tryVerify(() => focused(field), 2000)
            typeText("long enough passphrase")
            keyClick(Qt.Key_Return)
            tryVerify(() => focused(repeat), 2000, "on to the second field")
            compare(TestSupport.passphraseAnswer(), "waiting")
            typeText("long enough passphrase")
            keyClick(Qt.Key_Return)
            answered(() => TestSupport.passphraseAnswer(), "answered:long enough passphrase")
            tryVerify(() => !passphraseSheet.visible)
        }

        function test_cancel_turns_the_passphrase_down() {
            openPassphrase()
            mouseClick(findByName(passphraseSheet.contentItem, "passphraseCancel"))
            answered(() => TestSupport.passphraseAnswer(), "cancelled")
        }
    }
}
