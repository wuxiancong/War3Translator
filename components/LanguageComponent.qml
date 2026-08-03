// LanguageComponent.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import War3Translator.ThemeManager 1.0
import War3Translator.SettingsManager 1.0

Rectangle {
    id: languageComponent

    property bool compactMode: false
    property bool globalSafeMode: false
    property real preferredWidth: compactMode ? 140 : 320

    implicitWidth: preferredWidth
    implicitHeight: 20

    radius: ThemeManager.borderRadiusSm
    color: compactMode ? "transparent" : ThemeManager.tertiaryColor
    border.color: compactMode ? "transparent" : ThemeManager.borderColor
    border.width: compactMode ? 0 : 1

    ListModel {
        id: languageModel
        ListElement {
            text: qsTr("简体中文")
            value: "zh_CN"
        }
        ListElement {
            text: qsTr("繁體中文")
            value: "zh_TW"
        }
        ListElement {
            text: qsTr("English")
            value: "en"
        }
        ListElement {
            text: qsTr("Русский")
            value: "ru"
        }
        ListElement {
            text: qsTr("Español")
            value: "es"
        }
        ListElement {
            text: qsTr("Deutsch")
            value: "de"
        }
        ListElement {
            text: qsTr("Français")
            value: "fr"
        }
        ListElement {
            text: qsTr("Italiano")
            value: "it"
        }
        ListElement {
            text: qsTr("日本語")
            value: "ja"
        }
        ListElement {
            text: qsTr("한국어")
            value: "ko"
        }
        ListElement {
            text: qsTr("Polski")
            value: "pl"
        }
        ListElement {
            text: qsTr("Português")
            value: "pt"
        }
        ListElement {
            text: qsTr("Українська")
            value: "uk"
        }
        ListElement {
            text: qsTr("العربية")
            value: "ar"
        }
        ListElement {
            text: qsTr("Български")
            value: "bg"
        }
        ListElement {
            text: qsTr("Català")
            value: "ca"
        }
        ListElement {
            text: qsTr("Čeština")
            value: "cs"
        }
        ListElement {
            text: qsTr("Dansk")
            value: "da"
        }
        ListElement {
            text: qsTr("Suomi")
            value: "fi"
        }
        ListElement {
            text: qsTr("Gàidhlig")
            value: "gd"
        }
        ListElement {
            text: qsTr("עברית")
            value: "he"
        }
        ListElement {
            text: qsTr("Magyar")
            value: "hu"
        }
        ListElement {
            text: qsTr("Latviešu")
            value: "lv"
        }
        ListElement {
            text: qsTr("Slovenčina")
            value: "sk"
        }
        ListElement {
            text: qsTr("Türkçe")
            value: "tr"
        }
    }

    Connections {
        target: SettingsManager
        function onLanguageCodeChanged() {
            console.log("🌐 [LanguageComponent]: 检测到语言切换信号 (LanguageCodeChanged)");
            syncIndex()
        }
    }

    function syncIndex() {
        var currentCode = SettingsManager.languageCode
        if (!currentCode) return;
        for (var i = 0; i < languageModel.count; i++) {
            if (languageModel.get(i).value === currentCode) {
                if (languageComboBox.currentIndex !== i) {
                    languageComboBox.currentIndex = i
                }
                return
            }
        }
    }
    Component.onCompleted: syncIndex()

    ComboBox {
        id: languageComboBox
        anchors.fill: parent
        model: languageModel
        textRole: "text"

        background: Rectangle {
            color: "transparent"
        }

        contentItem: Text {
            text: globalSafeMode ? "" : languageComboBox.displayText
            font.pixelSize: ThemeManager.fontSizeBody
            font.family: ThemeManager.fontFamilyMain
            color: ThemeManager.textColor
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: compactMode ? Text.AlignRight : Text.AlignLeft
            leftPadding: compactMode ? 0 : 10
            rightPadding: 30
            elide: globalSafeMode ? Text.ElideNone : Text.ElideRight
            visible: text !== ""
            clip: true
        }

        indicator: Canvas {
            x: languageComboBox.width - width - 10
            y: languageComboBox.height / 2 - height / 2
            width: 10
            height: 6
            contextType: "2d"
            onPaint: {
                context.reset()
                context.moveTo(0, 0)
                context.lineTo(width, 0)
                context.lineTo(width / 2, height)
                context.closePath()
                context.fillStyle = ThemeManager.textColor
                context.fill()
            }
        }

        popup: Popup {
            y: languageComboBox.height + 3
            width: Math.max(languageComboBox.width, 180)
            x: compactMode ? languageComboBox.width - width : 0
            implicitHeight: Math.min(400, languageListView.contentHeight + 4)
            padding: 2
            z: 150

            background: Rectangle {
                color: ThemeManager.secondaryColor
                border.color: ThemeManager.borderColor
                radius: 4
                layer.enabled: true
            }

            contentItem: ListView {
                id: languageListView
                clip: true
                model: languageComboBox.delegateModel
                currentIndex: languageComboBox.highlightedIndex

                ScrollBar.vertical: ScrollBar {
                    id: customScrollBar
                    width: 10
                    policy: ScrollBar.AsNeeded
                    active: true

                    contentItem: Rectangle {
                        implicitWidth: 10
                        radius: 5
                        color: customScrollBar.pressed ? ThemeManager.accentColor : ThemeManager.alpha(
                                                             ThemeManager.textColor,
                                                             0.3)
                    }

                    background: Rectangle {
                        implicitWidth: 10
                        color: "transparent"
                    }
                }
            }
        }

        delegate: ItemDelegate {
            width: languageListView.width
            height: 40
            highlighted: languageComboBox.highlightedIndex === index
            rightPadding: customScrollBar.visible ? customScrollBar.width + 8 : 10

            contentItem: Text {
                text: globalSafeMode ? "" : model.text
                color: highlighted ? ThemeManager.accentColor : ThemeManager.textColor
                font.pixelSize: ThemeManager.fontSizeBody
                verticalAlignment: Text.AlignVCenter
                leftPadding: 10
                elide: globalSafeMode ? Text.ElideNone : Text.ElideRight
                visible: text !== ""
                clip: true
            }

            background: Rectangle {
                color: highlighted ? ThemeManager.alpha(
                                         ThemeManager.accentColor,
                                         0.1) : "transparent"
            }

            onClicked: {
                var selectedLang = languageModel.get(index).value
                languageComboBox.popup.close()
                Qt.callLater(function() {
                    SettingsManager.setLanguageCode(selectedLang)
                })
            }
        }
    }
}
