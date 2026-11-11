// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import GpgFrontend.Mobile

// The window. It is up before anything is opened: the splash follows the
// startup, the error page ends a startup that cannot go on, and the main page
// takes over once the core is ready. Questions from the startup worker, and
// passphrases an operation needs, come up over whichever page is showing.
ApplicationWindow {
    id: window

    width: 400
    height: 800
    visible: true
    title: qsTr("GpgFrontend")

    // The system's light or dark, followed while the application runs. Left
    // to the style (Theme=System), each item takes the scheme once, when it
    // is made, and what is up already -- a sheet -- stays as it was. Set
    // here, every item and popup follows the window.
    Material.theme: Theme.dark ? Material.Dark : Material.Light

    // Back on Android: close what is open, step back, return to the first
    // section, and only then leave -- to the background, as Android 12 and
    // later leave every application: closing would end the session, and
    // with it unsaved files, the text and any operation under way. Popups
    // take Back themselves.
    onClosing: function(close) {
        if (shell.item && shell.item.back && shell.item.back()) close.accepted = false
        else if (StartupController.state === StartupController.State.kREADY
                 && AppLifecycle.toBackground()) close.accepted = false
    }

    // The window's measures, for the keyboard.
    Binding {
        target: Theme
        property: "windowHeight"
        value: window.height
    }

    Binding {
        target: Theme
        property: "pixelRatio"
        value: Screen.devicePixelRatio
    }

    Binding {
        target: Theme
        property: "safeTop"
        value: window.SafeArea.margins.top
    }

    Binding {
        target: Theme
        property: "safeBottom"
        value: window.SafeArea.margins.bottom
    }

    Binding {
        target: Theme
        property: "safeLeft"
        value: window.SafeArea.margins.left
    }

    Binding {
        target: Theme
        property: "safeRight"
        value: window.SafeArea.margins.right
    }

    // Larger text, as the system asks: the style's own controls follow the
    // window's font. At the default size they keep the style's sizes.
    Binding {
        when: Theme.fontScale > 1
        target: window
        property: "font.pixelSize"
        value: Theme.secondarySize
    }

    Loader {
        id: shell

        anchors.fill: parent
        sourceComponent: {
            switch (StartupController.state) {
            case StartupController.State.kREADY:
                return mainPage
            case StartupController.State.kFAILED:
                return errorPage
            default:
                return splashPage
            }
        }
    }

    Component {
        id: splashPage
        SplashPage {}
    }

    Component {
        id: errorPage
        ErrorPage {}
    }

    Component {
        id: mainPage
        MainPage {}
    }

    PinSheet {}

    PassphraseSheet {}

    ConfirmSheet {}
}
