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
            // The add button floats over a list once there is one.
            verify(!findByName(files, "addFiles").visible)
            verify(!files.selecting)
        }

        function test_choosing_files_is_a_mode_back_leaves() {
            const path = Qt.resolvedUrl("tst_files.qml")
            files.workspace.addFiles([path])
            compare(files.workspace.count, 1)
            verify(!findByName(files, "emptyFiles").visible)
            verify(findByName(files, "addFiles").visible)

            // A long press starts it ...
            let row = null
            tryVerify(() => (row = findByName(files, "file_tst_files.qml")) !== null)
            row.pressAndHold()
            verify(files.selecting)
            compare(files.workspace.selectedCount, 1)
            verify(findByName(files, "encryptSelected").visible)
            verify(!findByName(files, "addFiles").visible)
            // ... a tap then chooses and unchooses ...
            row.clicked()
            verify(!files.selecting)
            row.pressAndHold()
            // ... and Back ends it, keeping the files.
            verify(files.back())
            verify(!files.selecting)
            verify(!files.back())
            compare(files.workspace.count, 1)

            files.workspace.clear()
            compare(files.workspace.count, 0)
        }

        function test_a_share_that_went_nowhere_points_to_save() {
            files.shared("unsupported")
            verify(files.notice.text.indexOf("Save") >= 0)
            files.shared("dismissed")
            compare(files.notice.text, "")
        }
    }
}
