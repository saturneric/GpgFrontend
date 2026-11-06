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

    // The verify sheet on its own: what it asks to verify is only heard.
    DetachedVerifySheet {
        id: verifier
        workspace: FileWorkspace
    }

    SignalSpy {
        id: verifyAsked
        target: verifier
        signalName: "verifyRequested"
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
            // A sharp line icon on a tint, not a scaled bitmap.
            compare(findByName(files, "emptyFiles").symbol, "folder")
            verify(findByName(findByName(files, "emptyFiles"), "emptySymbol").visible)
            // Adding is the + at the foot, the same with files or without;
            // the empty list explains, it has no button of its own.
            verify(findByName(files, "addFilesButton").enabled)
            // Alone, adding files takes the corner.
            compare(findByName(files, "addFilesButton").offsetX, 0)
            compare(findByName(files, "addFilesBar"), null)
            compare(findByName(files, "selectionBar"), null)
            // No operations without files.
            verify(!findByName(files, "operationButton").visible)
            verify(!files.selecting)
        }

        // Adding offers files one by one, or a folder's; what a folder
        // added is said once, with what it left out.
        function test_add_offers_files_or_a_folder() {
            findByName(files, "addFilesButton").clicked()
            const sheet = files.addChoice
            tryVerify(() => sheet.opened)
            verify(findByName(sheet.contentItem, "addPickFiles").visible)
            verify(findByName(sheet.contentItem, "addPickFolder").visible)
            sheet.close()
            tryVerify(() => !sheet.visible)

            // Counted as the language counts (English: one file, files).
            TestSupport.useLanguage("en_US")
            files.folderAdded(3, 2, 0, false)
            compare(files.notice.text, "3 files added. 2 folders in it not included.")
            files.folderAdded(0, 0, 0, false)
            compare(files.notice.text, "No new files in this folder.")
            files.folderAdded(500, 0, 12, false)
            compare(files.notice.text, "500 files added. 12 more files not added.")
            files.folderAdded(1, 1, 0, false)
            compare(files.notice.text, "1 file added. 1 folder in it not included.")
            files.folderAdded(0, 0, 0, true)
            compare(files.notice.text, "This folder could not be read.")
        }

        function test_choosing_files_is_a_mode_back_leaves() {
            const path = Qt.resolvedUrl("tst_files.qml")
            files.workspace.addFiles([path])
            compare(files.workspace.count, 1)
            verify(!findByName(files, "emptyFiles").visible)
            verify(findByName(files, "operationButton").visible)
            // The PGP button in the corner, adding files to its left.
            const add = findByName(files, "addFilesButton")
            const pgp = findByName(files, "operationButton")
            verify(add.x + add.width <= pgp.x)

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

        // Choosing a file moves nothing: not the list under the bar, not a
        // row's icon or text, not its height; and the icons have one size
        // and one place in every row.
        function test_choosing_moves_nothing() {
            files.workspace.addFiles([Qt.resolvedUrl("tst_files.qml"),
                                      Qt.resolvedUrl("tst_text.qml")])
            const id = files.workspace.idNamed("tst_files.qml")
            let row = null
            // This file's row, not one left over from a test before.
            tryVerify(() => (row = findByName(files, "file_tst_files.qml")) !== null
                            && row.fileId === id)
            const list = findByName(files, "fileList")
            const icon = findByName(row, "listRowSymbol")
            verify(icon !== null)
            compare(icon.width, 28)
            const shape = () => {
                const at = list.mapToItem(files, 0, 0)
                const iconAt = icon.mapToItem(row, 0, 0)
                const title = findByName(row, "listRowTitle").mapToItem(row, 0, 0)
                return [at.y, list.height, row.height, iconAt.x, iconAt.y,
                        title.x, title.y].join(",")
            }
            // With a larger font (the phone's), a text button in the bar is
            // taller than an icon: it must not make the bar taller.
            const selectAll = findByName(files, "selectAll")
            selectAll.font.pixelSize = 30
            const before = shape()
            files.workspace.setSelected(id, true)
            verify(files.selecting)
            wait(0)
            compare(shape(), before)
            files.workspace.selectAll()
            wait(0)
            compare(shape(), before)
            files.workspace.clearSelection()
            wait(0)
            compare(shape(), before)
            selectAll.font.pixelSize = Theme.bodySize
            files.workspace.clear()
        }

        // What an added file is: its name a hint, its first bytes the
        // proof, as for a file made here; nothing is decrypted to tell.
        function test_an_added_file_is_what_it_holds() {
            const ws = files.workspace
            const vectors = "../../../../resource/lfs/test/rpgp_vectors/"
            const kinds = {
                "enc_v1seipd_mdc.pgp": "message",       // binary, PKESK first
                "enc_symmetric_v1.pgp": "message",      // binary, SKESK first
                "sig_good_detached.sig": "signature",   // binary signature
                "sig_good_cleartext.asc": "message",    // armored, signed
                "garbage.bin": "",                      // nothing in particular
            }
            const urls = Object.keys(kinds).map(n => Qt.resolvedUrl(vectors + n))
            urls.push(Qt.resolvedUrl("data/not_really.gpg"),  // text, named .gpg
                      Qt.resolvedUrl("data/picture.gpg"),     // a PNG, named .gpg
                      Qt.resolvedUrl("data/alice_key.txt"))   // a key, named .txt
            kinds["not_really.gpg"] = ""
            kinds["picture.gpg"] = ""
            kinds["alice_key.txt"] = "key"
            ws.addFiles(urls)
            for (const name of Object.keys(kinds)) {
                const entry = ws.entry(ws.idNamed(name))
                compare(entry.content, kinds[name], name)
            }
            // The icon follows what it is.
            compare(files.symbolOf({"content": "key"}), "key")
            ws.clear()
        }

        // A signature whose file is the one name that matches is verified at
        // once, from either of the two; with two candidates the person
        // chooses, the fields filled in as far as the names go.
        function test_a_certain_pair_is_verified_at_once() {
            const ws = files.workspace
            ws.addFiles([Qt.resolvedUrl("data/notes.txt"),
                         Qt.resolvedUrl("data/notes.txt.sig")])
            const data = ws.idNamed("notes.txt")
            const sig = ws.idNamed("notes.txt.sig")
            tryVerify(() => verifier.usable(data) && verifier.usable(sig))

            verifyAsked.clear()
            verifier.begin(sig)
            compare(verifyAsked.count, 1)
            compare(verifyAsked.signalArguments[0][0], sig)
            compare(verifyAsked.signalArguments[0][1], data)
            verify(!verifier.visible)

            verifier.begin(data)
            compare(verifyAsked.count, 2)
            compare(verifyAsked.signalArguments[1][0], sig)
            verify(!verifier.visible)

            // A second signature of the same name: no longer certain.
            ws.addFiles([Qt.resolvedUrl("data/notes.txt.asc")])
            const asc = ws.idNamed("notes.txt.asc")
            tryVerify(() => verifier.usable(asc))
            verifyAsked.clear()
            verifier.begin(data)
            tryVerify(() => verifier.opened)
            compare(verifyAsked.count, 0)
            compare(verifier.originalId, data)
            verify(verifier.signatureId === sig || verifier.signatureId === asc)
            compare(verifier.signatureHow, "suggested")
            verify(findByName(verifier.contentItem, "verifySignatureFieldBrowse") !== null)
            verifier.close()
            tryVerify(() => !verifier.visible)

            // A file with no partner at all: the sheet, the other field empty.
            ws.remove([sig, asc])
            verifier.begin(data)
            tryVerify(() => verifier.opened)
            compare(verifyAsked.count, 0)
            compare(verifier.signatureId, -1)
            verifier.close()
            tryVerify(() => !verifier.visible)
            ws.clear()
        }

        // Two files of the name a signature is made for, from two folders:
        // which one it is over is not known, so the person chooses.
        function test_two_files_of_one_name_are_never_paired_at_once() {
            const ws = files.workspace
            ws.addFiles([Qt.resolvedUrl("data/notes.txt"),
                         Qt.resolvedUrl("data/other/notes.txt"),
                         Qt.resolvedUrl("data/notes.txt.sig")])
            const data = ws.idNamed("notes.txt")
            const other = ws.idNamed("notes (2).txt")
            const sig = ws.idNamed("notes.txt.sig")
            verify(other >= 0, "the second is told apart by its name")
            tryVerify(() => verifier.usable(data) && verifier.usable(other)
                             && verifier.usable(sig))
            verifyAsked.clear()
            verifier.begin(sig)
            tryVerify(() => verifier.opened)
            compare(verifyAsked.count, 0)
            compare(verifier.signatureId, sig)
            compare(verifier.originalHow, "suggested")
            verifier.close()
            tryVerify(() => !verifier.visible)
            ws.clear()
        }

        // Signed in the clear, an .asc is verified alone even beside a file
        // named like the data a detached signature would be over.
        function test_a_message_beside_a_namesake_is_verified_alone() {
            const ws = files.workspace
            const vectors = "../../../../resource/lfs/test/rpgp_vectors/"
            ws.addFiles([Qt.resolvedUrl(vectors + "sig_good_cleartext.asc"),
                         Qt.resolvedUrl("data/other/sig_good_cleartext")])
            const id = ws.idNamed("sig_good_cleartext.asc")
            tryVerify(() => verifier.usable(id))
            verifyAsked.clear()
            verifier.begin(id)
            compare(verifyAsked.count, 1)
            compare(verifyAsked.signalArguments[0][0], id)
            compare(verifyAsked.signalArguments[0][1], -1)
            verify(!verifier.visible)
            ws.clear()
        }

        // A file that holds its signature (signed inline, or in the clear)
        // is verified alone, at once, with no second file asked for.
        function test_an_inline_signed_file_is_verified_alone() {
            const ws = files.workspace
            const vectors = "../../../../resource/lfs/test/rpgp_vectors/"
            ws.addFiles([Qt.resolvedUrl(vectors + "sig_good_cleartext.asc"),
                         Qt.resolvedUrl(vectors + "two_signer.pgp")])
            for (const name of ["sig_good_cleartext.asc", "two_signer.pgp"]) {
                const id = ws.idNamed(name)
                tryVerify(() => verifier.usable(id))
                verify(files.allowed("verify", [ws.entry(id)]), name)
                verifyAsked.clear()
                verifier.begin(id)
                compare(verifyAsked.count, 1, name)
                compare(verifyAsked.signalArguments[0][0], id)
                compare(verifyAsked.signalArguments[0][1], -1)
                verify(!verifier.visible)
            }
            ws.clear()
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

        // A search that hides a chosen file lets it go: nothing is acted on
        // out of sight. One with no operation run offers no result to show.
        function test_what_a_search_hides_is_not_chosen() {
            const ws = files.workspace
            ws.addFiles([Qt.resolvedUrl("tst_files.qml"), Qt.resolvedUrl("tst_text.qml")])
            const a = ws.idNamed("tst_files.qml")
            const b = ws.idNamed("tst_text.qml")
            ws.setSelected(a, true)
            ws.setSelected(b, true)
            compare(ws.selectedCount, 2)
            files.openSearch()
            findByName(files, "fileSearch").text = "files"
            tryVerify(() => ws.selectedCount === 1)
            compare(ws.selectedIds(), [a])

            files.fileDetails.begin(a)
            tryVerify(() => files.fileDetails.opened)
            verify(!findByName(files.fileDetails.contentItem, "detailsResult").visible)
            files.fileDetails.close()
            tryVerify(() => !files.fileDetails.visible)
            files.closeSearch()
            ws.clear()
        }

        // Taken off the list, a file's details go with it: nothing is left
        // open about a file that is not there.
        function test_details_go_with_their_file() {
            const ws = files.workspace
            ws.addFiles([Qt.resolvedUrl("tst_files.qml")])
            const id = ws.idNamed("tst_files.qml")
            files.fileDetails.begin(id)
            tryVerify(() => files.fileDetails.opened)
            ws.remove([id])
            tryVerify(() => !files.fileDetails.visible)
            ws.clear()
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
            // Shown by its two ends until asked for whole; copied whole.
            const hash = files.fileDetails.sha256
            const shown = findByName(findByName(files.fileDetails.contentItem,
                                                "fileSha256"), "infoValue")
            compare(shown.text, hash.slice(0, 8) + "…" + hash.slice(-8))
            files.fileDetails.hashExpanded = true
            compare(shown.text, hash)
            // Name, size and hash: three lines of one height, the copy
            // button still a full touch target.
            const content = files.fileDetails.contentItem
            const heightOf = n => findByName(content, n).height
            compare(heightOf("fileSha256"), heightOf("fileName"))
            compare(heightOf("fileSize"), heightOf("fileName"))
            compare(findByName(content, "fileSha256").parent.height, heightOf("fileName"))
            compare(findByName(content, "copySha256").height, Theme.touch)
            // Taking it off the list stays, apart from what is done with it.
            verify(findByName(files.fileDetails.contentItem, "detailsRemove").visible)
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
