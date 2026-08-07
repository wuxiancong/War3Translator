import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.12
import Qt.labs.platform 1.1

import War3Translator.IpcManager 1.0
import War3Translator.ThemeManager 1.0
import War3Translator.SettingsManager 1.0
import War3Translator.TranslateManager 1.0

ApplicationWindow {
    id: mainWindow
    width: 500
    height: 750
    visible: true
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint

    // ===== 托盘图标 =====
    SystemTrayIcon {
        id: trayIcon
        visible: true
        icon.source: (typeof appDirPath !== "undefined")
                     ? appDirPath + "/images/translator-app.ico"
                     : "qrc:/images/translator-app.ico"
        tooltip: qsTr("War3 翻译助手")
        menu: Menu {
            MenuItem { text: qsTr("显示窗口"); onTriggered: { mainWindow.show(); mainWindow.raise(); mainWindow.requestActivate() } }
            MenuItem { text: qsTr("退出程序"); onTriggered: Qt.quit() }
        }
        onActivated: {
            if (reason === SystemTrayIcon.Trigger) {
                if (mainWindow.visible) mainWindow.hide()
                else { mainWindow.show(); mainWindow.raise(); mainWindow.requestActivate() }
            }
        }
    }

    onClosing: { close.accepted = false; mainWindow.hide() }

    // ===== 数据模型 =====
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

    // ===== 业务逻辑 =====
    Connections {
        target: IpcManager
        function onIncomingMessageIntercepted(pid, sender, text, direction) {
            var currentTime = Qt.formatDateTime(new Date(), "hh:mm:ss")
            chatLogModel.insert(0, {
                                    "pid": pid,
                                    "sender": sender,
                                    "origin": text,
                                    "time": currentTime,
                                    "translated": "",
                                    "isDone": false,
                                    "direction": direction
                                })
        }
    }

    Connections {
        target: TranslateManager

        function onTranslationTaskFinished(msgId, pid, flag, extraScope, direction, originalMessage, translatedMessage, targetLang) {
            console.log("▶ [QML 翻译回调] ID: " + msgId);
            console.log("   ├─ 玩家PID:", pid, " | 方向:", direction, " (1=发送/本地, 0=接收/他人)");
            console.log("   ├─ 目标语种:", targetLang);
            console.log("   └─ 文本内容: \"" + originalMessage + "\" -> \"" + translatedMessage + "\"");

            var foundMatch = false;

            for(var i = 0; i < chatLogModel.count; i++) {
                var item = chatLogModel.get(i);

                var isMatch = (item.msgId !== undefined && item.msgId !== 0)
                        ? (item.msgId === msgId)
                        : (item.pid === pid && item.origin === originalMessage);

                if(isMatch) {
                    foundMatch = true;

                    console.log("   ✅ 在数据模型索引 " + i + " 处找到对应条目 [ID: " + msgId + "]");

                    if (item.direction === 1) {
                        var langName = SettingsManager.getLanguageName(targetLang);
                        var langTag = "[" + langName + "] ";

                        console.log("   ├─ 模式: 发送 (多语种追加)");

                        var newText = item.translated === "" ? langTag + translatedMessage
                                                             : item.translated + "\n" + langTag + translatedMessage;

                        chatLogModel.setProperty(i, "translated", newText);
                    }
                    else {
                        console.log("   ├─ 模式: 接收 (单语种替换)");
                        chatLogModel.setProperty(i, "translated", translatedMessage);
                    }

                    chatLogModel.setProperty(i, "isDone", true);
                    if (item.msgId === undefined || item.msgId === 0) {
                        chatLogModel.setProperty(i, "msgId", msgId);
                    }
                    break;
                }
            }

            if (!foundMatch) {
                console.warn("   ❌ [警告] 在 chatLogModel 中未找到匹配项！");
                console.warn("      期待的 ID:", msgId, " PID:", pid, " 期待的原文: \"" + originalMessage + "\"");
            }
        }
    }

    // ===== 主界面 =====
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
            radius: 12.0; samples: 25
            color: ThemeManager.shadowColor
            verticalOffset: 4
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ----- 标题栏 -----
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

                    // 主题切换
                    CustomIconButton {
                        text: ThemeManager.currentTheme === "dark" ? "🌙" : "☀"
                        onClicked: ThemeManager.toggleTheme()
                    }
                    // 界面语言
                    ComboBox {
                        id: langCombo
                        model: languageModel
                        textRole: "text"
                        implicitWidth: 150
                        implicitHeight: 30
                        Layout.preferredWidth: 150
                        currentIndex: {
                            for(var i = 0; i < languageModel.count; i++) {
                                if(languageModel.get(i).value === SettingsManager.languageCode) return i;
                            }
                            return 0;
                        }
                        onActivated: {
                            var selectedCode = languageModel.get(index).value
                            SettingsManager.setLanguageCode(selectedCode)
                        }
                        background: Rectangle { color: ThemeManager.tertiaryColor; radius: 4 }
                        contentItem: Text {
                            text: langCombo.displayText
                            color: ThemeManager.textColor
                            font.pixelSize: 11
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        popup: Popup {
                            y: langCombo.height + 5; width: 140; padding: 5
                            background: Rectangle { color: ThemeManager.tertiaryColor; radius: 8; border.color: ThemeManager.borderColor }
                            contentItem: ListView {
                                implicitHeight: contentHeight
                                model: langCombo.delegateModel
                                clip: true
                            }
                        }
                        delegate: ItemDelegate {
                            width: 130; height: 35
                            background: Rectangle {
                                color: highlighted ? ThemeManager.alpha(ThemeManager.accentColor, 0.2) : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: model.text
                                color: highlighted ? ThemeManager.accentColor : ThemeManager.textColor
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    CustomIconButton { text: "−"; onClicked: mainWindow.showMinimized() }
                    CustomIconButton { text: "✕"; onClicked: mainWindow.hide(); isClose: true }
                }
            }

            // ----- Tab 切换 -----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: ThemeManager.secondaryColor
                clip: true

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    spacing: 15

                    // 聊天 Tab
                    Rectangle {
                        id: chatTab
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        radius: 8
                        color: currentTab === "chat" ? ThemeManager.accentColor : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "💬 " + qsTr("聊天")
                            color: currentTab === "chat" ? "white" : ThemeManager.textColor
                            font.pixelSize: 14
                            font.bold: currentTab === "chat"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: currentTab = "chat"
                        }
                    }

                    // 设置 Tab
                    Rectangle {
                        id: settingsTab
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        radius: 8
                        color: currentTab === "settings" ? ThemeManager.accentColor : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "⚙ " + qsTr("设置")
                            color: currentTab === "settings" ? "white" : ThemeManager.textColor
                            font.pixelSize: 14
                            font.bold: currentTab === "settings"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: currentTab = "settings"
                        }
                    }
                }
            }

            // ----- 内容区域 -----
            StackLayout {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: currentTab === "chat" ? 0 : 1

                // ---------- 聊天页面 ----------
                ColumnLayout {
                    spacing: 0

                    ListView {
                        id: logListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: chatLogModel
                        clip: true
                        spacing: 20
                        topMargin: 20; bottomMargin: 20; leftMargin: 15; rightMargin: 15

                        delegate: Item {
                            width: logListView.width - 30
                            height: mainCol.height + 10

                            readonly property bool isMe: model.direction === 1

                            ColumnLayout {
                                id: mainCol
                                width: parent.width
                                spacing: 4

                                // 1. 发送者名字：根据身份靠左或靠右
                                Text {
                                    id: senderNameText
                                    text: isMe ? qsTr("我") : model.sender
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: isMe ? ThemeManager.accentColor : ThemeManager.successColor
                                    Layout.alignment: isMe ? Qt.AlignRight : Qt.AlignLeft
                                }

                                // 2. 消息气泡容器
                                Rectangle {
                                    id: bubbleContainer
                                    Layout.alignment: isMe ? Qt.AlignRight : Qt.AlignLeft
                                    Layout.preferredWidth: Math.min(innerContent.implicitWidth + 24, mainCol.width * 0.85)
                                    Layout.preferredHeight: innerContent.height + 20
                                    color: ThemeManager.cardBackgroundColor
                                    radius: 10
                                    border.color: ThemeManager.borderColor
                                    border.width: 1

                                    layer.enabled: true
                                    layer.effect: DropShadow {
                                        transparentBorder: true
                                        radius: 4; samples: 8
                                        color: "#20000000"
                                    }

                                    ColumnLayout {
                                        id: innerContent
                                        anchors.centerIn: parent
                                        width: parent.width - 24
                                        spacing: 8
                                        Layout.alignment: Qt.AlignLeft

                                        // 3. 原文内容
                                        Text {
                                            Layout.fillWidth: true
                                            text: "<font color='" + (isMe ? "#BBBBBB" : "#888888") + "'>[" + model.time + "]</font> " + model.origin
                                            font.pixelSize: 11
                                            color: ThemeManager.textMutedColor
                                            textFormat: Text.StyledText
                                            wrapMode: Text.Wrap
                                            horizontalAlignment: Text.AlignLeft
                                        }

                                        // 分隔线
                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 1
                                            color: ThemeManager.borderColor
                                            opacity: 0.3
                                            visible: model.translated !== ""
                                        }

                                        // 4. 译文内容
                                        Text {
                                            Layout.fillWidth: true
                                            text: {
                                                if (model.translated !== "") return model.translated
                                                if (!model.isDone) return qsTr("正在翻译...")
                                                return qsTr("未获取到译文")
                                            }
                                            font.pixelSize: 13
                                            font.bold: true
                                            color: ThemeManager.textColor
                                            wrapMode: Text.Wrap
                                            horizontalAlignment: Text.AlignLeft
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ---------- 设置页面 ----------
                ScrollView {
                    id: settingsScroll
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentHeight: settingsColumn.implicitHeight + 40

                    Column {
                        id: settingsColumn
                        width: settingsScroll.width - 30
                        spacing: 25
                        leftPadding: 15
                        rightPadding: 15
                        topPadding: 30
                        bottomPadding: 20

                        // 我的语言
                        Column {
                            width: parent.width
                            spacing: 8

                            RowLayout {
                                spacing: 10

                                Text {
                                    text: qsTr("我的语言：")
                                    color: ThemeManager.textColor
                                    font.pixelSize: 13
                                    Layout.preferredWidth: implicitWidth
                                }

                                ComboBox {
                                    id: settingsLangCombo
                                    model: languageModel
                                    textRole: "text"
                                    implicitWidth: 150
                                    Layout.preferredWidth: 150
                                    currentIndex: {
                                        for(var i=0; i<languageModel.count; i++) {
                                            if(languageModel.get(i).value === SettingsManager.translateLanguage) return i;
                                        }
                                        return 0;
                                    }
                                    onActivated: {
                                        var selectedCode = languageModel.get(index).value
                                        SettingsManager.setTranslateLanguage(selectedCode)
                                    }
                                    background: Rectangle {
                                        color: ThemeManager.tertiaryColor
                                        radius: 4
                                        border.color: ThemeManager.borderColor
                                    }
                                    contentItem: Text {
                                        text: settingsLangCombo.displayText
                                        color: ThemeManager.textColor
                                        font.pixelSize: 12
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    popup: Popup {
                                        y: settingsLangCombo.height + 5
                                        width: 160
                                        padding: 5
                                        height: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, 540)
                                        background: Rectangle {
                                            color: ThemeManager.tertiaryColor
                                            radius: 8
                                            border.color: ThemeManager.borderColor
                                        }
                                        contentItem: ListView {
                                            implicitHeight: contentHeight
                                            model: settingsLangCombo.delegateModel
                                            clip: true
                                            boundsBehavior: Flickable.StopAtBounds
                                        }
                                    }
                                    delegate: ItemDelegate {
                                        width: 150
                                        height: 35
                                        background: Rectangle {
                                            color: highlighted ? ThemeManager.alpha(ThemeManager.accentColor, 0.2) : "transparent"
                                            radius: 4
                                        }
                                        contentItem: Text {
                                            text: model.text
                                            color: highlighted ? ThemeManager.accentColor : ThemeManager.textColor
                                            font.pixelSize: 12
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }
                                }
                                Item {
                                    Layout.fillWidth: true
                                }
                            }

                            Text {
                                text: qsTr("备注：设置后玩家发送过来的语言将会翻译为此语言")
                                color: ThemeManager.textMutedColor
                                font.pixelSize: 11
                                font.italic: true
                                wrapMode: Text.Wrap
                                width: parent.width
                                leftPadding: 0
                            }
                        }

                        // 分隔线
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: ThemeManager.borderColor
                            opacity: 0.3
                        }

                        // 发送语言多选
                        Column {
                            width: parent.width
                            spacing: 8

                            RowLayout {
                                width: parent.width

                                Text {
                                    text: qsTr("发送语言：")
                                    color: ThemeManager.textColor
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.fillWidth: true
                                }

                                // 全选按钮
                                CheckBox {
                                    id: selectAllCheckBox
                                    text: qsTr("全选")
                                    checked: SettingsManager.translateLanguages.length === languageModel.count

                                    indicator: Rectangle {
                                        implicitWidth: 16
                                        implicitHeight: 16
                                        radius: 3
                                        color: selectAllCheckBox.checked ? ThemeManager.accentColor : "transparent"
                                        border.color: selectAllCheckBox.checked ? ThemeManager.accentColor : ThemeManager.borderColor
                                        border.width: 2
                                        Text {
                                            anchors.centerIn: parent
                                            text: "✓"
                                            color: "white"
                                            font.pixelSize: 10
                                            visible: selectAllCheckBox.checked
                                        }
                                    }

                                    contentItem: Text {
                                        text: selectAllCheckBox.text
                                        color: ThemeManager.textColor
                                        font.pixelSize: 12
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: 22
                                    }

                                    onClicked: {
                                        if (checked) {
                                            var allLangs = []
                                            for (var i = 0; i < languageModel.count; i++) {
                                                allLangs.push(languageModel.get(i).value)
                                            }
                                            SettingsManager.setTranslateLanguages(allLangs)
                                        } else {
                                            SettingsManager.setTranslateLanguages([])
                                        }
                                    }
                                }
                            }

                            Text {
                                text: qsTr("备注：如果多选将会依次发送所有已翻译语言给玩家")
                                color: ThemeManager.textMutedColor
                                font.pixelSize: 11
                                font.italic: true
                                wrapMode: Text.Wrap
                                width: parent.width
                            }

                            Rectangle {
                                width: parent.width
                                height: 1
                                color: ThemeManager.borderColor
                                opacity: 0.2
                            }

                            // 添加间距
                            Item { height: 5 }

                            // 25种语言多选列表
                            GridView {
                                id: languageGridView
                                width: parent.width
                                height: contentHeight
                                interactive: false
                                model: languageModel
                                cellWidth: parent.width / 2
                                cellHeight: 40
                                clip: false

                                delegate: CheckBox {
                                    id: langCheckBox
                                    width: languageGridView.cellWidth - 10
                                    height: 35
                                    text: model.text

                                    checked: SettingsManager.translateLanguages.indexOf(model.value) !== -1

                                    onClicked: {
                                        var currentList = SettingsManager.translateLanguages
                                        var langValue = model.value
                                        var indexInList = currentList.indexOf(langValue)

                                        if (checked) {
                                            if (indexInList === -1) {
                                                currentList.push(langValue)
                                            }
                                        } else {
                                            if (indexInList !== -1) {
                                                currentList.splice(indexInList, 1)
                                            }
                                        }

                                        SettingsManager.setTranslateLanguages(currentList)
                                    }

                                    indicator: Rectangle {
                                        implicitWidth: 18
                                        implicitHeight: 18
                                        x: 2
                                        y: parent.height / 2 - height / 2
                                        radius: 3
                                        color: langCheckBox.checked ? ThemeManager.accentColor : "transparent"
                                        border.color: langCheckBox.checked ? ThemeManager.accentColor : ThemeManager.borderColor
                                        border.width: 2

                                        Text {
                                            anchors.centerIn: parent
                                            text: "✓"
                                            color: "white"
                                            font.pixelSize: 12
                                            font.bold: true
                                            visible: langCheckBox.checked
                                        }
                                    }

                                    contentItem: Text {
                                        text: langCheckBox.text
                                        color: ThemeManager.textColor
                                        font.pixelSize: 12
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: 28
                                    }

                                    background: Rectangle {
                                        color: langCheckBox.hovered ? ThemeManager.alpha(ThemeManager.textColor, 0.05) : "transparent"
                                        radius: 4
                                    }
                                }
                            }
                        }

                        // 分隔线
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: ThemeManager.borderColor
                            opacity: 0.3
                        }

                        Column {
                            width: parent.width
                            spacing: 12

                            RowLayout {
                                width: parent.width
                                spacing: 15

                                Text {
                                    text: qsTr("发送间隔：")
                                    color: ThemeManager.textColor
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.preferredWidth: 80
                                }

                                Slider {
                                    id: intervalSlider
                                    Layout.fillWidth: true
                                    from: 500
                                    to: 5000
                                    stepSize: 100
                                    value: SettingsManager.translateSendInterval

                                    onMoved: {
                                        SettingsManager.setTranslateSendInterval(value)
                                    }

                                    background: Rectangle {
                                        x: intervalSlider.leftPadding
                                        y: intervalSlider.topPadding + intervalSlider.availableHeight / 2 - height / 2
                                        implicitHeight: 4
                                        width: intervalSlider.availableWidth
                                        height: implicitHeight
                                        radius: 2
                                        color: ThemeManager.alpha(ThemeManager.textColor, 0.1)

                                        Rectangle {
                                            width: intervalSlider.visualPosition * parent.width
                                            height: parent.height
                                            color: ThemeManager.accentColor
                                            radius: 2
                                        }
                                    }

                                    handle: Rectangle {
                                        x: intervalSlider.leftPadding + intervalSlider.visualPosition * (intervalSlider.availableWidth - width)
                                        y: intervalSlider.topPadding + intervalSlider.availableHeight / 2 - height / 2
                                        implicitWidth: 16
                                        implicitHeight: 16
                                        radius: 8
                                        color: intervalSlider.pressed ? ThemeManager.alpha(ThemeManager.accentColor, 0.8) : ThemeManager.accentColor
                                        border.color: "white"
                                        border.width: 1
                                    }
                                }

                                Text {
                                    text: (SettingsManager.translateSendInterval / 1000).toFixed(1) + " s"
                                    color: ThemeManager.accentColor
                                    font.pixelSize: 13
                                    font.bold: true
                                    Layout.preferredWidth: 40
                                }
                            }

                            Text {
                                text: qsTr("备注：设置翻译后依次发送消息的时间间隔，防止由于发送过快导致被游戏屏蔽")
                                color: ThemeManager.textMutedColor
                                font.pixelSize: 11
                                font.italic: true
                                wrapMode: Text.Wrap
                                width: parent.width
                            }
                        }

                        // 分隔线
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: ThemeManager.borderColor
                            opacity: 0.3
                        }

                        // 添加间距
                        Item { height: 5 }

                        // 服务器选择
                        RowLayout {
                            width: parent.width
                            spacing: 15

                            Text {
                                text: qsTr("服务器：")
                                color: ThemeManager.textColor
                                font.pixelSize: 13
                                Layout.preferredWidth: 80
                            }

                            ComboBox {
                                id: serverCombo
                                model: ListModel {
                                    id: serverModel
                                    ListElement { name: "本地服务器"; latency: 0 }
                                    ListElement { name: "香港服务器"; latency: 32 }
                                    ListElement { name: "中国服务器"; latency: 78 }
                                }
                                textRole: "name"
                                implicitWidth: 180
                                currentIndex: 0

                                background: Rectangle {
                                    color: ThemeManager.tertiaryColor
                                    radius: 4
                                    border.color: ThemeManager.borderColor
                                }

                                contentItem: Text {
                                    text: {
                                        var item = serverModel.get(serverCombo.currentIndex)
                                        if (item.latency === 0) return serverCombo.displayText + " (-- ms)"
                                        return serverCombo.displayText + " (" + item.latency + " ms)"
                                    }
                                    color: ThemeManager.textColor
                                    font.pixelSize: 12
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                popup: Popup {
                                    y: serverCombo.height + 5
                                    width: 200
                                    padding: 5
                                    background: Rectangle {
                                        color: ThemeManager.tertiaryColor
                                        radius: 8
                                        border.color: ThemeManager.borderColor
                                    }
                                    contentItem: ListView {
                                        implicitHeight: contentHeight
                                        model: serverCombo.delegateModel
                                        clip: true
                                        boundsBehavior: Flickable.StopAtBounds
                                    }
                                }

                                delegate: ItemDelegate {
                                    width: 190
                                    height: 35
                                    background: Rectangle {
                                        color: highlighted ? ThemeManager.alpha(ThemeManager.accentColor, 0.2) : "transparent"
                                        radius: 4
                                    }

                                    contentItem: RowLayout {
                                        spacing: 10
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        anchors.rightMargin: 10

                                        Text {
                                            text: model.name
                                            color: highlighted ? ThemeManager.accentColor : ThemeManager.textColor
                                            font.pixelSize: 12
                                            Layout.fillWidth: true
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        Row {
                                            spacing: 5
                                            Layout.alignment: Qt.AlignVCenter

                                            Rectangle {
                                                width: 8
                                                height: 8
                                                radius: 4
                                                visible: model.latency > 0
                                                color: {
                                                    if (model.latency < 50) return ThemeManager.successColor
                                                    else if (model.latency < 150) return ThemeManager.warningColor
                                                    else return ThemeManager.errorColor
                                                }
                                                anchors.verticalCenter: parent.verticalCenter
                                            }

                                            Text {
                                                text: model.latency > 0 ? model.latency + " ms" : "-- ms"
                                                color: {
                                                    if (model.latency === 0) return ThemeManager.textDisabledColor
                                                    else if (model.latency < 50) return ThemeManager.successColor
                                                    else if (model.latency < 150) return ThemeManager.warningColor
                                                    else return ThemeManager.errorColor
                                                }
                                                font.pixelSize: 11
                                                font.family: "monospace"
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                        }
                                    }
                                }
                            }

                            // 刷新延迟按钮
                            CustomIconButton {
                                text: "🔄"
                                implicitWidth: 28
                                implicitHeight: 28
                                onClicked: {
                                    // 更新所有服务器的延迟
                                    for (var i = 0; i < serverModel.count; i++) {
                                        var item = serverModel.get(i)
                                        if (item.name === "本地服务器") {
                                            serverModel.setProperty(i, "latency", 0)
                                        } else {
                                            var randomLat = Math.floor(Math.random() * 200) + 10
                                            serverModel.setProperty(i, "latency", randomLat)
                                        }
                                    }
                                }
                            }
                        }

                        // 底部留白
                        Item { height: 20 }
                    }
                }
            }
        }
    }

    // ===== 属性与组件 =====
    property string currentTab: "chat"

    component CustomIconButton : AbstractButton {
        id: iconBtn
        property bool isClose: false
        implicitWidth: 32
        implicitHeight: 32
        contentItem: Text {
            text: iconBtn.text
            font.pixelSize: 16
            color: iconBtn.hovered ? (isClose ? ThemeManager.errorColor : ThemeManager.accentColor) : ThemeManager.textColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 8
            color: iconBtn.hovered ? ThemeManager.alpha(ThemeManager.textColor, 0.1) : "transparent"
        }
    }
}