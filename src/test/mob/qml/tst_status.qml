import QtQuick
import QtTest
import GpgFrontend.Mobile

// The status sheet, with no core behind it: what it offers. The engine's
// report is the one thing it copies; the result itself is copied or shared
// where it is, on its page.
Item {
    width: 400
    height: 800

    StatusSheet {
        id: sheet
    }

    TestCase {
        name: "StatusSheet"
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

        function test_only_the_report_is_copied() {
            const body = sheet.body
            compare(findByName(body, "statusCopy"), null)
            const box = findByName(body, "statusReportBox")
            verify(box !== null)
            // The copy button is the report box's own.
            const copy = findByName(box, "statusCopyReport")
            verify(copy !== null)
            compare(copy.Accessible.name, "Copy the report")
            copy.clicked()
            verify(copy.copied)
            compare(copy.Accessible.name, "Copied")
            tryCompare(copy, "copied", false, 3000)
        }

        // Status or Details, as on the desktop: the cards or the report,
        // never both, and Status first.
        function test_status_or_details() {
            const body = sheet.body
            const status = findByName(body, "statusViewStatus")
            const details = findByName(body, "statusViewDetails")
            verify(status !== null && details !== null)
            compare(status.text, "Status")
            compare(details.text, "Details")
            verify(status.chosen)
            verify(!details.chosen)
            details.clicked()
            verify(details.chosen)
            verify(!status.chosen)
            status.clicked()
            verify(status.chosen)
        }
    }
}
