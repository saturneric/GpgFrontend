// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton

import QtQuick

// What the platform's style does not give: GpgFrontend's own touches, kept
// few (the desktop's status colours in src/ui/function/UIStyle.cpp, its
// monospace font and icons), the spacing pages share, and how much of the
// window the on-screen keyboard covers. Everything else is the style's own.
QtObject {
    readonly property int gap: 16
    readonly property int gapSmall: 8

    // The platform's measures: a touch target, a list row of one line and
    // of two.
    readonly property int touch: 48
    readonly property int rowHeight: 56
    readonly property int rowHeightTwoLine: 72

    // The type scale: body, secondary lines, small labels, titles. Fixed,
    // as the style's own controls are: Qt on Android gives every application
    // the same default font whatever the system's font size.
    readonly property int bodySize: 16
    readonly property int secondarySize: 14
    readonly property int labelSize: 12
    readonly property int titleSize: 22

    readonly property bool dark: Application.styleHints.colorScheme === Qt.ColorScheme.Dark

    // The accent, as qtquickcontrols2.conf gives it to the style.
    readonly property color accent: "#2A82DA"

    // A surface a step off @p base toward the accent, as the platform tints
    // its bars and fields: the higher @p level, the deeper the tint.
    function surface(base, level) {
        const a = (dark ? 0.07 : 0.05) * level
        return Qt.rgba(base.r + (accent.r - base.r) * a,
                       base.g + (accent.g - base.g) * a,
                       base.b + (accent.b - base.b) * a, 1)
    }

    // What marks the chosen one: the navigation's pill, a selected row.
    readonly property color indicator: alpha(accent, dark ? 0.32 : 0.18)

    // The desktop's colours for how an operation went.
    readonly property color positive: dark ? Qt.rgba(102 / 255, 187 / 255, 106 / 255, 1)
                                           : Qt.rgba(46 / 255, 125 / 255, 50 / 255, 1)
    readonly property color warning: dark ? Qt.rgba(227 / 255, 179 / 255, 65 / 255, 1)
                                          : Qt.rgba(155 / 255, 100 / 255, 0, 1)
    readonly property color danger: dark ? Qt.rgba(239 / 255, 83 / 255, 80 / 255, 1)
                                         : Qt.rgba(198 / 255, 40 / 255, 40 / 255, 1)

    // The bundled JetBrains Mono NL the desktop uses for keys and armored
    // text, from gf_res.
    readonly property FontLoader monoFont: FontLoader {
        source: "qrc:/fonts/JetBrainsMonoNL-Regular.ttf"
    }
    readonly property string mono: monoFont.status === FontLoader.Ready
                                   ? monoFont.name : "monospace"

    // The window's height and pixel ratio, from the window, for the keyboard.
    property real windowHeight: 0
    property real pixelRatio: 1

    // How much of the window the on-screen keyboard covers. Pages end where
    // it begins (the window counts it as a safe area); popups are laid over
    // the whole window and make room themselves. The keyboard's rectangle
    // comes in device pixels.
    readonly property real keyboard: Qt.inputMethod.visible
                                     ? Math.max(0, windowHeight
                                                - Qt.inputMethod.keyboardRectangle.y
                                                / pixelRatio)
                                     : 0

    // How an operation went, from a result's report: an icon and its colour,
    // in the desktop's colours; @p neutral for one that was cancelled.
    function reportSymbol(report) {
        if (report.cancelled === true) return "info"
        if (report.status > 0) return "check_circle"
        return report.status === 0 ? "warning" : "error"
    }

    function reportColor(report, neutral) {
        if (report.cancelled === true) return neutral
        if (report.status > 0) return positive
        return report.status === 0 ? warning : danger
    }

    // "4.2 MB", in the decimal units the platform's own storage pages use.
    function fileSize(bytes) {
        if (bytes < 1000) return qsTr("%1 B").arg(bytes)
        if (bytes < 1000 * 1000) return qsTr("%1 kB").arg(Math.round(bytes / 1000))
        if (bytes < 1000 * 1000 * 1000)
            return qsTr("%1 MB").arg((bytes / 1000 / 1000).toFixed(1))
        return qsTr("%1 GB").arg((bytes / 1000 / 1000 / 1000).toFixed(1))
    }

    function icon(name) {
        return Qt.resolvedUrl("icons/" + name + ".svg")
    }

    function alpha(color, a) {
        return Qt.rgba(color.r, color.g, color.b, a)
    }
}
