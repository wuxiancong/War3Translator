import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.12
import Qt.labs.platform 1.1 // 用于系统托盘

// 导入单例
import War3Translator.ThemeManager 1.0
import War3Translator.SettingsManager 1.0
import War3Translator.ChatManager 1.0
import War3Translator.TranslateManager 1.0

ApplicationWindow {
    id: mainWindow
    width: 500
    height: 750
    visible: true
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint

    SystemTrayIcon {
        id: trayIcon
        visible: true
        // 确保你的资源文件中有这个图标，路径要写对
        icon.source: (typeof appDirPath !== "undefined")
                     ? appDirPath + "/images/translator-app.ico"
                     : "qrc:/images/translator-app.ico"
        tooltip: qsTr("War3 翻译助手")

        // 托盘右键菜单
        menu: Menu {
            MenuItem {
                text: qsTr("显示窗口")
                onTriggered: {
                    mainWindow.show()
                    mainWindow.raise()
                    mainWindow.requestActivate()
                }
            }
            MenuItem {
                text: qsTr("退出程序")
                onTriggered: Qt.quit()
            }
        }

        // 点击托盘图标逻辑
        onActivated: {
            // reason 是点击类型（1是触发/单击，2是右键，3是双击）
            if (reason === SystemTrayIcon.Trigger) {
                if (mainWindow.visible) {
                    mainWindow.hide() // 如果窗口可见，点击托盘则隐藏
                } else {
                    mainWindow.show() // 如果窗口隐藏，点击托盘则显示
                    mainWindow.raise()
                    mainWindow.requestActivate()
                }
            }
        }
    }

    // 拦截关闭按钮，改为隐藏到托盘
    onClosing: {
        close.accepted = false
        mainWindow.hide() // 隐藏窗口，任务栏图标会消失，但托盘图标还在
    }

    // --- 业务逻辑保持不变 ---
    Connections {
        target: ChatManager
        function onMessageReceived(sender, text) {
            var targetLang = languageModel.get(targetLangCombo.currentIndex).value
            TranslateManager.translate(text, "auto", targetLang)

            chatLogModel.insert(0, {
                                    "sender": sender,
                                    "origin": text,
                                    "translated": qsTr("正在翻译..."),
                                    "isDone": false
                                })
        }
    }

    Connections {
            target: TranslateManager
            function onTranslationTaskFinished(pid, flag, extraScope, originalMessage, translatedMessage) {
                for(var i = 0; i < chatLogModel.count; i++) {
                    if(chatLogModel.get(i).pid === pid || chatLogModel.get(i).origin === originalMessage) {
                        chatLogModel.setProperty(i, "translated", translatedMessage)
                        chatLogModel.setProperty(i, "isDone", true)
                        break
                    }
                }
            }
        }

    ListModel { id: chatLogModel }

    ListModel {
        id: languageModel
        ListElement { text: qsTr("简体中文"); value: "zh_CN" }
        ListElement { text: qsTr("繁體中文"); value: "zh_TW" }
        ListElement { text: qsTr("English"); value: "en" }
        ListElement { text: qsTr("Русский"); value: "ru" }
        ListElement { text: qsTr("Español"); value: "es" }
        ListElement { text: qsTr("Deutsch"); value: "de" }
        ListElement { text: qsTr("Français"); value: "fr" }
        ListElement { text: qsTr("Italiano"); value: "it" }
        ListElement { text: qsTr("日本語"); value: "ja" }
        ListElement { text: qsTr("한국어"); value: "ko" }
        ListElement { text: qsTr("Polski"); value: "pl" }
        ListElement { text: qsTr("Português"); value: "pt" }
        ListElement { text: qsTr("Українська"); value: "uk" }
        ListElement { text: qsTr("العربية"); value: "ar" }
        ListElement { text: qsTr("Български"); value: "bg" }
        ListElement { text: qsTr("Català"); value: "ca" }
        ListElement { text: qsTr("Čeština"); value: "cs" }
        ListElement { text: qsTr("Dansk"); value: "da" }
        ListElement { text: qsTr("Suomi"); value: "fi" }
        ListElement { text: qsTr("Gàidhlig"); value: "gd" }
        ListElement { text: qsTr("עברית"); value: "he" }
        ListElement { text: qsTr("Magyar"); value: "hu" }
        ListElement { text: qsTr("Latviešu"); value: "lv" }
        ListElement { text: qsTr("Slovenčina"); value: "sk" }
        ListElement { text: qsTr("Türkçe"); value: "tr" }
    }

    // --- 界面布局 ---
    Rectangle {
        id: windowContainer
        anchors.fill: parent
        anchors.margins: 10
        color: ThemeManager.primaryColor
        radius: 12
        border.color: ThemeManager.borderColor
        border.width: 1

        layer.enabled: true
        layer.effect: DropShadow {
            radius: 12.0
            samples: 25
            color: ThemeManager.shadowColor
            verticalOffset: 4
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                id: titleBar
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: ThemeManager.secondaryColor
                radius: 12
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 10
                    color: parent.color
                }
                MouseArea {
                    anchors.fill: parent
                    onPressed: mainWindow.startSystemMove()
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 10
                    spacing: 10
                    Text { text: "⚔"; font.pixelSize: 20; color: ThemeManager.accentColor }
                    Column {
                        Text { text: qsTr("WAR3 翻译助手"); font.pixelSize: 14; font.bold: true; color: ThemeManager.textColor }
                        Row {
                            spacing: 4
                            Rectangle {
                                width: 6; height: 6; radius: 3
                                color: ThemeManager.successColor
                                anchors.verticalCenter: parent.verticalCenter
                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    NumberAnimation { from: 1; to: 0.3; duration: 800 }
                                    NumberAnimation { from: 0.3; to: 1; duration: 800 }
                                }
                            }
                            Text { text: qsTr("正在监控游戏聊天..."); font.pixelSize: 10; color: ThemeManager.textMutedColor }
                        }
                    }
                    Item { Layout.fillWidth: true }
                    CustomIconButton {
                        text: ThemeManager.currentTheme === "dark" ? "🌙" : "☀"
                        onClicked: ThemeManager.toggleTheme()
                    }
                    ComboBox {
                        id: targetLangCombo
                        model: languageModel
                        textRole: "text"
                        implicitWidth: 100; implicitHeight: 30
                        background: Rectangle { color: ThemeManager.tertiaryColor; radius: 4 }
                        contentItem: Text { text: targetLangCombo.displayText; color: ThemeManager.textColor; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        popup: Popup {
                            y: targetLangCombo.height + 5; width: 140; padding: 5
                            background: Rectangle { color: ThemeManager.tertiaryColor; radius: 8; border.color: ThemeManager.borderColor }
                            contentItem: ListView { implicitHeight: contentHeight; model: targetLangCombo.delegateModel; clip: true }
                        }
                        delegate: ItemDelegate {
                            width: 130; height: 35
                            background: Rectangle { color: highlighted ? ThemeManager.alpha(ThemeManager.accentColor, 0.2) : "transparent"; radius: 4 }
                            contentItem: Text { text: model.text; color: highlighted ? ThemeManager.accentColor : ThemeManager.textColor; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    // 按钮修改：最小化是收起，关闭是隐藏到托盘
                    CustomIconButton { text: "−"; onClicked: mainWindow.showMinimized() }
                    CustomIconButton { text: "✕"; onClicked: mainWindow.hide(); isClose: true }
                }
            }

            ListView {
                id: logListView
                Layout.fillWidth: true; Layout.fillHeight: true
                model: chatLogModel; clip: true; spacing: 15
                topMargin: 20; bottomMargin: 20; leftMargin: 15; rightMargin: 15
                add: Transition { NumberAnimation { properties: "opacity,scale"; from: 0; duration: 300 } }
                delegate: Column {
                    width: logListView.width - 30; spacing: 6
                    Text { text: model.sender; font.pixelSize: 11; font.bold: true; color: ThemeManager.accentColor; leftPadding: 5 }
                    Rectangle {
                        width: parent.width; height: innerCol.height + 20
                        color: ThemeManager.cardBackgroundColor; radius: 10; border.color: ThemeManager.borderColor
                        Column {
                            id: innerCol; width: parent.width - 24; anchors.centerIn: parent; spacing: 8
                            Text { width: parent.width; text: model.origin; font.pixelSize: 11; color: ThemeManager.textMutedColor; wrapMode: Text.Wrap }
                            Rectangle { width: parent.width; height: 1; color: ThemeManager.borderColor; opacity: 0.5 }
                            Text { width: parent.width; text: model.translated; font.pixelSize: 14; font.bold: true; color: ThemeManager.textColor; wrapMode: Text.Wrap }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 30; color: "transparent"
                Text { anchors.centerIn: parent; text: qsTr("War3 Translator © 2026"); font.pixelSize: 10; color: ThemeManager.textDisabledColor }
            }
        }
    }

    component CustomIconButton : AbstractButton {
        id: iconBtn
        property bool isClose: false
        implicitWidth: 32; implicitHeight: 32
        contentItem: Text { text: iconBtn.text; font.pixelSize: 16; color: iconBtn.hovered ? (isClose ? ThemeManager.errorColor : ThemeManager.accentColor) : ThemeManager.textColor; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { radius: 8; color: iconBtn.hovered ? ThemeManager.alpha(ThemeManager.textColor, 0.1) : "transparent" }
    }
}