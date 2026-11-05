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
            // Said for a moment, then gone by itself.
            const said = copy.resources.find(o => o.objectName === "statusReportCopied")
            verify(said, "the copied tip")
            // Up, as it is whenever its button can be reached.
            sheet.open()
            tryVerify(() => sheet.opened)
            copy.clicked()
            tryVerify(() => said.visible)
            compare(said.text, "Copied.")
            tryVerify(() => !said.visible, 3000)
            sheet.close()
            tryVerify(() => !sheet.visible)
        }

        // The report's box: as tall as a short report, and no more than a
        // little under half the window for a long one, which scrolls in it.
        function test_the_report_box_is_as_tall_as_its_report() {
            const box = findByName(sheet.body, "statusReportBox")
            const report = findByName(box, "statusReport")
            report.text = "One line"
            fuzzyCompare(box.wanted, report.implicitHeight + 24, 0.5)
            report.text = "A line\n".repeat(200)
            fuzzyCompare(box.wanted, sheet.windowHeight * 0.45, 0.5)
            verify(sheet.windowHeight > 0)
            report.text = ""
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
            verify(status.checked)
            verify(!details.checked)
            details.click()
            verify(details.checked)
            verify(!status.checked)
            status.click()
            verify(status.checked)
            verify(!details.checked)
        }
    }
}
