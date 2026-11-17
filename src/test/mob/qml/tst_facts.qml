// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtTest
import GpgFrontend.Mobile

// The facts pages (build, engine, profile status): a row's sentence shows
// under it, a fallback is marked, a path is set in the mono face.
Item {
    id: root

    width: 360
    height: 640

    FactsPage {
        id: facts

        anchors.fill: parent
        title: "Status"
        sections: [{"title": "Needs Attention", "rows": [
                        {"label": "System Credential Store", "value": "Unavailable",
                         "detail": "No D-Bus session.", "degraded": true}]},
                    {"title": "At a Glance", "rows": [
                        {"label": "Secure Level", "value": "Strong"},
                        {"label": "Workspace", "value": "None", "dimmed": true},
                        {"label": "Profile ID", "value": "1b4e28ba", "mono": true}]}]
    }

    TestCase {
        name: "Facts"
        when: windowShown

        function find(parent, name) {
            for (let i = 0; i < parent.children.length; ++i) {
                const child = parent.children[i]
                if (child.objectName === name) return child
                const found = find(child, name)
                if (found) return found
            }
            return null
        }

        function rowOf(label) {
            return find(facts, "fact_" + label)
        }

        function test_a_rows_sentence_shows_under_it() {
            const row = rowOf("System Credential Store")
            verify(row)
            const detail = find(row, "infoDetail")
            verify(detail.visible)
            compare(detail.text, "No D-Bus session.")
            // Under the value, not beside it.
            verify(detail.y > find(row, "infoValue").y)
            // A row without one keeps its height.
            verify(!find(rowOf("Secure Level"), "infoDetail").visible)
        }

        function test_a_fallback_is_marked_and_nothing_is_quiet() {
            compare(find(rowOf("System Credential Store"), "infoValue").color,
                    Theme.warning)
            compare(find(rowOf("Workspace"), "infoValue").color,
                    facts.Material.secondaryTextColor)
            compare(find(rowOf("Secure Level"), "infoValue").color,
                    facts.Material.foreground)
            verify(find(rowOf("Profile ID"), "infoValue").font.family === Theme.mono)
        }

    }
}
