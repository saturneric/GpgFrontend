import QtQuick
import QtTest
import GpgFrontend.Mobile

// The key toolbox, with no core behind it: the steps an operation goes
// through once it is chosen. Encrypt asks who can read it, and runs;
// Encrypt & Sign asks who, then the key that signs; Sign asks only for the
// key that signs. Whether to sign is the operation's, never asked here, and
// a passphrase is an operation of its own, chosen before.
Item {
    width: 400
    height: 800

    KeyToolbox {
        id: box
    }

    TestCase {
        name: "KeyToolbox"
        when: windowShown

        function findByName(parent, name) {
            for (var i = 0; i < parent.children.length; ++i) {
                var child = parent.children[i]
                if (child.objectName === name)
                    return child
                var found = findByName(child, name)
                if (found)
                    return found
            }
            return null
        }

        function cleanup() {
            box.close()
            WorkSelection.reset()
        }

        // Closed while its search had the focus, it opens again on the list:
        // not in the search, which brought the keyboard up by itself.
        function test_reopened_it_is_not_in_the_search() {
            box.begin("encrypt", {"op": "encrypt", "signs": false,
                                  "label": "Encrypt now", "run": () => {}})
            tryVerify(() => box.opened)
            box.search.forceActiveFocus()
            verify(box.search.activeFocus)
            box.close()
            tryVerify(() => !box.visible)
            box.begin("sign", {"op": "sign", "label": "Sign now", "run": () => {}})
            tryVerify(() => box.opened)
            wait(50)
            verify(!box.search.activeFocus, "the search has the focus again")
        }

        function test_encrypting_asks_only_for_recipients() {
            box.begin("encrypt", {"op": "encrypt", "signs": false,
                                  "label": "Encrypt now", "run": () => {}})
            compare(box.step, 0)
            compare(box.title, "Encrypt to")
            const root = box.contentItem
            // Nothing about a passphrase, tabs or signing here.
            compare(findByName(root, "modePassphrase"), null)
            compare(findByName(root, "toolboxTabs"), null)
            compare(findByName(root, "signEncryptions"), null)
            // It runs from here: there is no next step.
            verify(!findByName(root, "toolboxNext").visible)
            verify(findByName(root, "toolboxRun").visible)
            // No recipients yet: it cannot run.
            verify(!findByName(root, "toolboxRun").enabled)
        }

        function test_encrypting_and_signing_asks_for_recipients_then_signer() {
            box.begin("encrypt", {"op": "encrypt", "signs": true,
                                  "label": "Encrypt and sign now", "run": () => {}})
            compare(box.step, 0)
            const root = box.contentItem
            verify(findByName(root, "toolboxNext").visible)
            verify(!findByName(root, "toolboxRun").visible)
            // No recipients yet: not on to signing.
            verify(!findByName(root, "toolboxNext").enabled)

            box.step = 1
            compare(box.title, "Sign with")
            verify(box.signerList)
            verify(findByName(root, "toolboxRun").visible)

            // Back goes to the recipients, not out.
            box.back()
            compare(box.step, 0)
            verify(box.visible)
        }

        // Keys are found by category, as on the Keys page; no key database.
        function test_keys_are_narrowed_by_category() {
            box.begin("encrypt", {"op": "encrypt", "label": "Encrypt now",
                                  "run": () => {}})
            const root = box.contentItem
            // A button that opens the categories, one under the other.
            verify(findByName(root, "toolboxCategory") !== null)
            compare(findByName(root, "toolboxCategories"), null)
            compare(box.category, "")
            // Chosen from the list of categories: the toolbox shows it, and
            // the button says which.
            const finder = findByName(root, "keyFinder")
            finder.categoryChosen("has_private_key")
            compare(box.category, "has_private_key")
            compare(finder.currentCategory.id, "has_private_key")
            compare(findByName(root, "toolboxCategory").category.id, "has_private_key")
            compare(findByName(root, "keyDatabaseButton"), null)
            // Begun again: all keys.
            box.begin("sign", {"op": "sign", "label": "Sign now", "run": () => {}})
            compare(box.category, "")
        }

        // What is chosen is one line, not a row per key; it opens the chosen
        // keys, where one is taken out, and the line follows. Searching and
        // choosing a category keep what is chosen.
        function test_the_chosen_keys_are_summed_up_and_kept() {
            box.begin("encrypt", {"op": "encrypt", "label": "Encrypt now",
                                  "run": () => {}})
            tryVerify(() => box.opened)
            const root = box.contentItem
            const summary = findByName(root, "selectedSummary")
            verify(!summary.visible)
            WorkSelection.addRecipient("A".repeat(40))
            WorkSelection.addRecipient("B".repeat(40))
            verify(summary.visible)
            compare(summary.text, Wording.selectedCount(2))
            verify(summary.text.indexOf("2") >= 0)
            // Ordinary words: a warning only when a chosen key cannot be
            // used, and nothing here says these cannot.
            compare(WorkSelection.recipientIssueCount, 0)
            verify(!findByName(root, "selectedProblem").visible)

            box.category = "has_private_key"
            findByName(root, "toolboxSearch").text = "nobody"
            compare(WorkSelection.recipientCount, 2)
            findByName(root, "toolboxSearch").text = ""
            box.category = ""

            summary.clicked()
            const chosen = box.selectedSheet
            tryVerify(() => chosen.opened)
            compare(chosen.count, 2)
            WorkSelection.removeRecipient("A".repeat(40))
            compare(chosen.count, 1)
            compare(summary.text, Wording.selectedCount(1))
            verify(summary.text.indexOf("1") >= 0)
            // Back closes the chosen keys only; the picker stays as it was.
            chosen.back()
            tryVerify(() => !chosen.opened)
            verify(box.opened)
            compare(box.step, 0)
        }

        // A chosen key asked about from the chosen keys: they make way for
        // its details. A key group has none.
        function test_a_chosen_key_s_details_come_over_the_list() {
            box.begin("encrypt", {"op": "encrypt", "label": "Encrypt now",
                                  "run": () => {}})
            tryVerify(() => box.opened)
            const chosen = box.selectedSheet
            chosen.open()
            tryVerify(() => chosen.opened)
            chosen.aboutKeyRequested({"isKey": false, "name": "Team"})
            verify(chosen.opened)
            compare(box.infoKey, null)
            chosen.aboutKeyRequested({"isKey": true, "name": "Alice",
                                      "fingerprint": "A".repeat(40)})
            tryVerify(() => !chosen.visible)
            compare(box.infoKey.name, "Alice")
            verify(box.opened)
            // Opening it on the Keys page is at the foot, beside the
            // operation: never under the buttons there.
            const open = findByName(box.contentItem, "toolboxOpenKey")
            verify(open.visible)
            verify(findByName(box.contentItem, "toolboxRun").visible)
            // However long its words, a sheet stays as wide as the window:
            // its buttons go one under the other and their words wrap.
            open.text = "A very long label for this button that cannot fit beside "
                        + "the others on one line, however wide the window is"
            waitForRendering(box.contentItem)
            verify(box.contentItem.width <= box.width + 0.5)
            verify(open.width <= box.width)
            verify(open.height > 48)
            box.infoKey = null
            verify(!open.visible)
        }

        // The list fills the sheet and scrolls itself; the sheet is never
        // taller than the window.
        function test_the_list_scrolls_in_the_sheet() {
            box.begin("encrypt", {"op": "encrypt", "label": "Encrypt now",
                                  "run": () => {}})
            tryVerify(() => box.opened)
            verify(box.listMode)
            verify(box.height <= 800)
            verify(box.keyList.height > 0)
        }

        function test_signing_asks_only_for_the_signer() {
            box.begin("sign", {"op": "sign", "label": "Sign now", "run": () => {}})
            compare(box.step, 1)
            compare(box.title, "Sign with")
            const root = box.contentItem
            verify(box.signerList)
            verify(findByName(root, "toolboxRun").visible)
            verify(!findByName(root, "toolboxNext").visible)
        }
    }
}
