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
            const chips = findByName(root, "toolboxCategories")
            verify(chips !== null)
            compare(box.category, "")
            box.category = "has_private_key"
            compare(box.category, "has_private_key")
            compare(findByName(root, "keyDatabaseButton"), null)
            // Begun again: all keys.
            box.begin("sign", {"op": "sign", "label": "Sign now", "run": () => {}})
            compare(box.category, "")
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
