// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton

import QtQuick
import GpgFrontend.Mobile

// What the platform's style does not give: GpgFrontend's own touches, kept
// few (the desktop's status colours in src/ui/function/UIStyle.cpp, its
// monospace font and icons), the spacing pages share, and how much of the
// window the on-screen keyboard covers. Everything else is the style's own.
QtObject {
    readonly property int gap: 16
    readonly property int gapSmall: 8
    // A bottom sheet's layout, the same in every sheet: the inset of its
    // content from its sides, the room between a title and the line that
    // explains it, between that line and the content, and between the
    // content and the buttons at the foot.
    readonly property int sheetPad: 24
    readonly property int sheetTitleGap: 12
    readonly property int sheetIntroGap: 16
    readonly property int sheetActionGap: 24

    // The platform's measures: a touch target, a list row of one line and
    // of two.
    readonly property int touch: 48
    readonly property int rowHeight: 56
    readonly property int rowHeightTwoLine: 72

    // The type scale: body, secondary lines, small labels, titles, times
    // the system's font scale. Qt on Android gives every application the same
    // default font whatever the system's font size, so the scale is read from
    // the system (AppLifecycle.fontScale) and capped where a phone's width
    // runs out. Not read-only only for the tests, which lay pages out at
    // other scales than the host's (always 1); nothing else sets it.
    property real fontScale: Math.max(1, Math.min(2,
        AppLifecycle.fontScale > 0 ? AppLifecycle.fontScale : 1))
    readonly property int bodySize: Math.round(16 * fontScale)
    readonly property int secondarySize: Math.round(14 * fontScale)
    readonly property int labelSize: Math.round(12 * fontScale)
    readonly property int titleSize: Math.round(22 * fontScale)
    readonly property int headlineSize: Math.round(18 * fontScale)
    readonly property int sheetTitleSize: Math.round(20 * fontScale)
    // Titles -- a page's, a sheet's -- a step heavier than what they head;
    // the rows under them keep the regular weight.
    readonly property int titleWeight: Font.DemiBold
    // Corners: small for chips and fields, large for sheets and the
    // floating buttons.
    readonly property int radiusSmall: 8
    readonly property int radiusLarge: 16

    // The floating buttons at the foot of a page: their size, their icon's,
    // the room around them, and the band they take, which a list under
    // them leaves free at its end (content padding, not a shorter view).
    readonly property int fabSize: 64
    readonly property int fabIcon: 28
    readonly property int fabMargin: 16
    readonly property int fabBand: fabSize + 2 * fabMargin
    // How high they float: one soft shadow, at rest and deeper while
    // pressed. It fits in the room around them (fabMargin), so nothing
    // clips it. A shadow shows less on a dark page, so there it is denser,
    // never an outline or a glow.
    readonly property int fabShadowBlur: 14
    readonly property int fabShadowBlurPressed: 18
    readonly property int fabShadowOffset: 3
    readonly property int fabShadowOffsetPressed: 6
    readonly property color fabShadow: Qt.rgba(0, 0, 0, dark ? 0.8 : 0.3)

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

    // The bottom sheets open, in the order they opened: the last one is the
    // topmost, the one Back and typing are for (BottomSheet).
    property var openSheets: []

    // The window's height and pixel ratio, from the window, for the keyboard.
    property real windowHeight: 0
    property real pixelRatio: 1
    // The window's safe areas at the top (the status bar) and the foot (the
    // gesture bar), from the window: what popups over the whole window, as a
    // bottom sheet, keep clear of. Counted once, here.
    property real safeTop: 0
    property real safeBottom: 0
    // And at the sides, where a landscape window has the camera's cutout or
    // the navigation buttons.
    property real safeLeft: 0
    property real safeRight: 0

    // How much of the window the on-screen keyboard covers. Pages end where
    // it begins (the window counts it as a safe area); popups are laid over
    // the whole window and make room themselves. The keyboard's rectangle
    // comes in device pixels.
    readonly property real keyboard: keyboardCover(Qt.inputMethod.visible,
                                                   Qt.inputMethod.keyboardRectangle,
                                                   windowHeight, pixelRatio)

    // What a keyboard of @p rect (device pixels) covers of a window
    // @p height tall. An input method can be visible with no keyboard on
    // screen: typed on a hardware keyboard, Android shows only a floating
    // toolbar and reports an empty rectangle, which covers nothing.
    function keyboardCover(visible, rect, height, ratio) {
        if (!visible || rect.height <= 0) return 0
        return Math.max(0, Math.min(rect.height / ratio,
                                    height - rect.y / ratio))
    }

    // How an operation went, from a result's report: an icon and its colour,
    // in the desktop's colours; @p neutral for one that was cancelled. The
    // severity is the desktop's reading of the result (ResultPresenter).
    function reportSymbol(report) {
        if (report.cancelled === true) return "info"
        switch (report.severity) {
        case "ok": return "check_circle"
        case "warn": return "warning"
        default: return "error"
        }
    }

    // The report seal's kind (ReportSeal) for a report.
    function sealKind(report) {
        if (report.cancelled === true) return "cancelled"
        switch (report.severity) {
        case "ok": return "success"
        case "warn": return "warning"
        default: return "failure"
        }
    }

    // @p a over @p b, @p weight of a.
    function mix(a, b, weight) {
        return Qt.rgba(a.r * weight + b.r * (1 - weight),
                       a.g * weight + b.g * (1 - weight),
                       a.b * weight + b.b * (1 - weight), 1)
    }

    // The stamp of one card, by its status ("ok", "warn", "critical",
    // "neutral"), as the desktop puts it beside the card's title.
    function cardStamp(status) {
        switch (status) {
        case "ok": return "qrc:/icons/stamp_success.png"
        case "warn": return "qrc:/icons/stamp_warning.png"
        case "critical": return "qrc:/icons/stamp_failed.png"
        default: return "qrc:/icons/stamp_info.png"
        }
    }

    function reportColor(report, neutral) {
        if (report.cancelled === true) return neutral
        switch (report.severity) {
        case "ok": return positive
        case "warn": return warning
        default: return danger
        }
    }

    // "4.2 MB", in the decimal units the platform's own storage pages use.
    function fileSize(bytes) {
        if (bytes < 1000) return qsTr("%1 B").arg(bytes)
        if (bytes < 1000 * 1000) return qsTr("%1 kB").arg(Math.round(bytes / 1000))
        if (bytes < 1000 * 1000 * 1000)
            return qsTr("%1 MB").arg((bytes / 1000 / 1000).toFixed(1))
        return qsTr("%1 GB").arg((bytes / 1000 / 1000 / 1000).toFixed(1))
    }

    // A date nobody can read two ways: 2026-11-03.
    function date(value) {
        return value && !isNaN(value) ? Qt.formatDate(value, "yyyy-MM-dd") : ""
    }

    // A fingerprint in groups of four, however long it is (40 hex digits for
    // a v4 key, 64 for a v6 one), for a Flow to wrap.
    function fingerprintGroups(fpr) {
        return (fpr || "").replace(/\s+/g, "").match(/.{1,4}/g) || []
    }

    function icon(name) {
        return Qt.resolvedUrl("icons/" + name + ".svg")
    }

    // An operation's icon as the desktop's toolbar has it, in its own
    // colours: the same lock, unlock, signature and seal everywhere.
    function operationIcon(op) {
        switch (op) {
        case "encrypt": return "qrc:/icons/lock.png"
        case "decrypt": return "qrc:/icons/unlock.png"
        case "sign": return "qrc:/icons/signature.png"
        case "verify": return "qrc:/icons/verify.png"
        case "symmetric": return "qrc:/icons/symmetric_encryption.png"
        case "encrypt_sign": return "qrc:/icons/encr-sign.png"
        case "decrypt_verify": return "qrc:/icons/decr-verify.png"
        default: return ""
        }
    }

    function alpha(color, a) {
        return Qt.rgba(color.r, color.g, color.b, a)
    }
}
