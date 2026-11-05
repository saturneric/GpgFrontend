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
            // Adding is the + at the foot, the same with files or without;
            // the empty list explains, it has no button of its own.
            verify(findByName(files, "addFilesButton").enabled)
            compare(findByName(files, "addFilesBar"), null)
            compare(findByName(files, "selectionBar"), null)
            // No operations without files.
            verify(!findByName(files, "operationButton").visible)
            verify(!files.selecting)
        }

        function test_choosing_files_is_a_mode_back_leaves() {
            const path = Qt.resolvedUrl("tst_files.qml")
            files.workspace.addFiles([path])
            compare(files.workspace.count, 1)
            verify(!findByName(files, "emptyFiles").visible)
            verify(findByName(files, "operationButton").visible)

            // Nothing chosen: the operations say so, none runs on all files.
            compare(files.operationMenu.unavailable,
                    "Select files first, or use Select all.")

            let row = null
            tryVerify(() => (row = findByName(files, "file_tst_files.qml")) !== null)
            const height = row.height
            const textWidth = row.contentItem.width
            const end = findByName(row, "fileRowEnd")
            const endX = end.x

            // A long press starts choosing ...
            row.pressAndHold()
            verify(files.selecting)
            compare(files.workspace.selectedCount, 1)
            compare(files.operationMenu.unavailable, "")
            // ... and the row keeps its shape: same height, text as wide,
            // the end slot where it was.
            compare(row.height, height)
            compare(row.contentItem.width, textWidth)
            compare(end.x, endX)
            // ... each operation when it can be done, as on the desktop, and
            // why not when it cannot: a file that is not OpenPGP data is
            // encrypted or signed, not decrypted or verified.
            compare(files.reasonFor("encrypt"), "")
            compare(files.reasonFor("sign"), "")
            compare(files.reasonFor("decrypt"), "Only for encrypted files.")
            verify(files.reasonFor("verify").length > 0)
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

        function test_the_list_is_searched() {
            files.workspace.addFiles([Qt.resolvedUrl("tst_files.qml"),
                                      Qt.resolvedUrl("tst_text.qml")])
            compare(files.shown.count, 2)
            // No kinds of file to choose from: searching is enough.
            compare(findByName(files, "fileFilters"), null)

            files.openSearch()
            verify(files.searching)
            findByName(files, "fileSearch").text = "files"
            compare(files.shown.count, 1)
            findByName(files, "fileSearch").text = "nothing"
            verify(findByName(files, "noMatchingFiles").visible)
            findByName(files, "fileSearch").text = "files"
            // Back empties the field, then closes it.
            verify(files.back())
            verify(files.searching)
            compare(files.shown.count, 2)
            verify(files.back())
            verify(!files.searching)

            // Rows say the size, not where the file came from.
            const entry = files.workspace.entry(files.workspace.idAt(0))
            verify(files.detailOf(entry).indexOf("Added") < 0)
            files.workspace.clear()
        }

        // A file's details: its name, size and SHA-256, worked out once the
        // sheet is open.
        function test_details_show_the_file_s_hash() {
            files.workspace.addFiles([Qt.resolvedUrl("tst_files.qml")])
            const id = files.workspace.idAt(0)
            files.fileDetails.begin(id)
            tryVerify(() => files.fileDetails.opened)
            tryVerify(() => files.fileDetails.sha256.length === 64, 10000)
            compare(files.fileDetails.hashError, "")
            // "Copied." is about this sheet's copy, not one before.
            const copied = findByName(files.fileDetails.contentItem, "fileCopied")
            copied.visible = true
            files.fileDetails.close()
            tryVerify(() => !files.fileDetails.visible)
            files.fileDetails.begin(id)
            verify(!copied.visible)
            files.fileDetails.close()
            tryVerify(() => !files.fileDetails.visible)
            files.workspace.clear()
        }

        // The desktop's rule for file operations, by what the names say.
        function test_operations_follow_what_the_files_are() {
            const plain = {"available": true, "name": "notes.txt", "content": ""}
            const message = {"available": true, "name": "notes.txt.gpg", "content": "message"}
            const signature = {"available": true, "name": "notes.txt.sig", "content": "signature"}
            const gone = {"available": false, "name": "gone.txt", "content": ""}
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
