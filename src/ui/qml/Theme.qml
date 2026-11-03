pragma Singleton

import QtQuick

QtObject {
    property string mode: "dark"
    readonly property bool darkMode: mode === "dark" || mode === "low-light"
    readonly property bool contrastMode: mode === "high-contrast"
    readonly property color windowBackground: contrastMode ? "#000000" : darkMode ? "#0b1020" : mode === "sepia" ? "#f2e7d1" : mode === "paper" ? "#f6f1e6" : mode === "gray" ? "#e8e8e8" : "#f7f8fa"
    readonly property color surface: contrastMode ? "#000000" : darkMode ? "#111a2e" : mode === "sepia" ? "#fbf1dc" : mode === "paper" ? "#fffaf0" : mode === "gray" ? "#f4f4f4" : "#ffffff"
    readonly property color surfaceRaised: contrastMode ? "#101010" : darkMode ? "#17243d" : "#ffffff"
    readonly property color surfaceMuted: contrastMode ? "#181818" : darkMode ? "#162139" : mode === "sepia" ? "#e8d9bd" : "#eef1f5"
    readonly property color sidebar: contrastMode ? "#050505" : darkMode ? "#0d1627" : mode === "sepia" ? "#e9dcc5" : "#f1f3f7"
    readonly property color coverBackground: darkMode ? "#242255" : "#e9e8ff"
    readonly property color readerBackground: contrastMode ? "#000000" : darkMode ? "#0a1020" : mode === "sepia" ? "#e8dcc5" : "#e7e9ee"
    readonly property color readerPaper: contrastMode ? "#000000" : darkMode ? "#191d24" : mode === "sepia" ? "#f7ecd5" : mode === "paper" ? "#fffaf0" : "#fffdf8"
    readonly property color readerText: contrastMode ? "#ffffff" : darkMode ? "#edf1f7" : mode === "sepia" ? "#443522" : "#272b34"
    readonly property color border: contrastMode ? "#ffffff" : darkMode ? "#2a3a5a" : mode === "sepia" ? "#d4c2a3" : "#d9dee7"
    readonly property color textPrimary: contrastMode ? "#ffffff" : darkMode ? "#f5f7ff" : mode === "sepia" ? "#3c2e1d" : "#20242b"
    readonly property color textSecondary: contrastMode ? "#ffffff" : darkMode ? "#93a4c2" : mode === "sepia" ? "#735f42" : "#5f6875"
    readonly property color accent: "#8b7cff"
    readonly property color accentSoft: darkMode ? "#3b3476" : "#c7d2fe"
    readonly property color accentPressed: "#6d5ce7"
    readonly property color highlight: "#f6c945"
    readonly property color success: darkMode ? "#6fe0c0" : "#157f5b"
    readonly property color warning: darkMode ? "#f3b477" : "#a15c00"
    readonly property color toast: darkMode ? "#eaf0ff" : "#20242b"
    readonly property color toastText: darkMode ? "#10182a" : "#ffffff"
    readonly property string readerFont: "Segoe UI"
}
