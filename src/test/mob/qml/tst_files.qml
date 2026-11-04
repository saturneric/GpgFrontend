import QtQuick
import QtTest
import GpgFrontend.Mobile

// The files page, with no core behind it: what it offers before anything is
// done, and choosing files. (The settings page reads the profile's settings, so it is loaded in
// gf_mob_tests instead, which has one.)
Item {
    width: 400
    height: 800

    FilesPage {
        id: files
        anchors.fill: parent
    }

    TestCase {
        name: "FilesPage"
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

        function test_an_empty_list_offers_to_add_files() {
            verify(findByName(files, "emptyFiles").visible)
            verify(findByName(files, "addFilesButton").enabled)
            // The operations are there, as on the Text page, and nothing can
            // be done with no file chosen.
            verify(findByName(files, "selectionBar").visible)
            for (const name of ["encryptSelected", "decryptSelected",
                                "signSelected", "verifySelected"])
                verify(!findByName(files, name).enabled, name)
            verify(!files.selecting)
        }

        function test_choosing_files_is_a_mode_back_leaves() {
            const path = Qt.resolvedUrl("tst_files.qml")
            files.workspace.addFiles([path])
            compare(files.workspace.count, 1)
            verify(!findByName(files, "emptyFiles").visible)
            verify(findByName(files, "selectionBar").visible)
            verify(findByName(files, "addFilesBar").visible)

            // A long press starts it ...
            let row = null
            tryVerify(() => (row = findByName(files, "file_tst_files.qml")) !== null)
            row.pressAndHold()
            verify(files.selecting)
            compare(files.workspace.selectedCount, 1)
            // ... with what can be done to them, and only that: no keys
            // there, they are asked for when an operation needs them ...
            for (const name of ["encryptSelected", "decryptSelected",
                                "signSelected", "verifySelected"])
                verify(findByName(files, name).visible, name)
            compare(findByName(files, "keysButton"), null)
            // ... each when it can be done, as on the desktop: a file that
            // is not OpenPGP data is encrypted or signed, not decrypted or
            // verified ...
            verify(findByName(files, "encryptSelected").enabled)
            verify(findByName(files, "signSelected").enabled)
            verify(!findByName(files, "decryptSelected").enabled)
            verify(!findByName(files, "verifySelected").enabled)
            // ... a tap then chooses and unchooses ...
            row.clicked()
            verify(!files.selecting)
            row.pressAndHold()
            // ... and Back ends it, keeping the files.
            verify(files.back())
            verify(!files.selecting)
            verify(!files.back())
            compare(files.workspace.count, 1)

            // No long press needed: every row has its box, and all of them
            // can be chosen at once.
            const box = findByName(files, "fileCheck_tst_files.qml")
            verify(box !== null && box.visible)
            box.toggle()
            box.toggled()
            verify(files.selecting)
            compare(files.workspace.selectedCount, 1)
            verify(box.checked)
            files.workspace.clearSelection()
            verify(!box.checked)
            const all = findByName(files, "selectAll")
            verify(all.visible)
            all.clicked()
            compare(files.workspace.selectedCount, 1)
            verify(!all.visible)
            files.workspace.clearSelection()

            files.workspace.clear()
            compare(files.workspace.count, 0)
        }

        function test_the_list_is_searched_and_narrowed() {
            files.workspace.addFiles([Qt.resolvedUrl("tst_files.qml"),
                                      Qt.resolvedUrl("tst_text.qml")])
            compare(files.shown.count, 2)
            verify(findByName(files, "fileFilters").visible)

            // Searching: by name ...
            files.openSearch()
            verify(files.searching)
            findByName(files, "fileSearch").text = "files"
            compare(files.shown.count, 1)
            // ... Back empties the field, then closes it.
            verify(files.back())
            verify(files.searching)
            compare(files.shown.count, 2)
            verify(files.back())
            verify(!files.searching)

            // Narrowed by what the files are, by name: .qml is neither.
            compare(files.workspace.entry(files.workspace.idAt(0)).content, "")
            findByName(files, "fileFilter_encrypted").clicked()
            compare(files.shown.category, "encrypted")
            compare(files.shown.count, 0)
            verify(findByName(files, "noMatchingFiles").visible)
            // Showing a file lets go of the filter that hides it.
            files.showFile(files.workspace.idAt(0))
            compare(files.shown.category, "all")
            compare(files.shown.count, 2)

            files.workspace.clear()
        }

        // The desktop's rule for file operations, by what the names say.
        function test_operations_follow_what_the_files_are() {
            const plain = {"available": true, "content": ""}
            const message = {"available": true, "content": "message"}
            const signature = {"available": true, "content": "signature"}
            const gone = {"available": false, "content": ""}
            verify(files.allowed("encrypt", [plain]))
            verify(files.allowed("encrypt_sign", [plain]))
            verify(files.allowed("symmetric", [plain]))
            verify(!files.allowed("encrypt", [plain, message]))
            verify(files.allowed("decrypt", [message, message]))
            verify(files.allowed("decrypt_verify", [message]))
            verify(!files.allowed("decrypt", [message, plain]))
            verify(files.allowed("sign", [plain]))
            verify(!files.allowed("sign", [signature]))
            verify(files.allowed("verify", [signature]))
            verify(!files.allowed("verify", [signature, message]))
            verify(!files.allowed("verify", [plain]))
            // Nothing for nothing, or for a file that cannot be opened.
            verify(!files.allowed("encrypt", []))
            verify(!files.allowed("encrypt", [gone]))
        }

        function test_a_share_that_went_nowhere_points_to_save() {
            files.shared("unsupported")
            verify(files.notice.text.indexOf("Save") >= 0)
            files.shared("dismissed")
            compare(files.notice.text, "")
        }
    }
}
