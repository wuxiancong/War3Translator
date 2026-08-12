pragma Singleton

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import War3Translator.ThemeManager 1.0

QtObject {
    id: root

    // 保存主窗口引用
    property var rootWindow: null
    // 保存弹窗实例
    property var _popupInstance: null

    // --- 核心修复：初始化函数 ---
    function init(windowObj) {
        rootWindow = windowObj;
        console.log("✅ MessagePopup 已成功关联主窗口");
    }

    function show(msg, type) {
        var inst = _ensureInstance();
        if (inst) inst.show(msg, type);
    }

    function showConfirm(msg, confirmCb) {
        var inst = _ensureInstance();
        if (inst) inst.showConfirm(msg, confirmCb);
    }

    // --- 内部：确保弹窗实例已创建 ---
    function _ensureInstance() {
        if (_popupInstance !== null) return _popupInstance;

        // 检查 rootWindow 是否已经通过 init 注入
        if (!rootWindow) {
            console.error("MessagePopup Error: 未调用 init() 关联主窗口。");
            return null;
        }

        try {
            // 在主窗口的 contentItem 上创建弹窗
            _popupInstance = popupComponent.createObject(rootWindow.contentItem);
            return _popupInstance;
        } catch (e) {
            console.error("MessagePopup Error: 动态创建失败 " + e);
            return null;
        }
    }

    // --- 弹窗 UI 组件 (保持不变) ---
    property Component popupComponent: Component {
        Popup {
            id: messagePopup
            property string message: ""
            property int msgType: 0
            property bool isConfirmation: false
            property var onConfirmCallback: null
            property real progressValue: 1.0
            property int duration: 3000

            x: (parent.width - width) / 2
            y: 30
            width: Math.min(340, parent.width - 40)
            height: contentCol.implicitHeight + 24
            opacity: 0
            padding: 0
            modal: isConfirmation
            focus: true
            closePolicy: isConfirmation ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)

            function show(msg, type) {
                var actualType = (type !== undefined) ? type : 0;
                isConfirmation = false;
                message = msg;
                msgType = actualType;
                progressValue = 1.0;
                autoCloseTimer.stop();
                messagePopup.open();
                showAnim.restart();
                autoCloseTimer.restart();
            }

            function showConfirm(msg, confirmCb) {
                autoCloseTimer.stop();
                isConfirmation = true;
                message = msg;
                msgType = 2;
                onConfirmCallback = confirmCb;
                messagePopup.open();
                showAnim.restart();
            }

            function hide() { hideAnim.restart(); }

            background: Rectangle {
                color: ThemeManager.cardBackgroundColor
                radius: ThemeManager.borderRadiusMd
                border.color: ThemeManager.borderColor
                border.width: 1
                Rectangle {
                    width: parent.width - 20; height: 2; radius: 1
                    anchors.top: parent.top; anchors.topMargin: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: getStatusColor()
                }
            }

            ColumnLayout {
                id: contentCol
                width: parent.width - 32
                anchors.centerIn: parent
                spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: getStatusIcon(); font.pixelSize: 16 }
                    Text {
                        text: getStatusTitle()
                        font.bold: true; font.pixelSize: 13
                        color: getStatusColor(); Layout.fillWidth: true
                    }
                    AbstractButton {
                        implicitWidth: 20; implicitHeight: 20
                        onClicked: messagePopup.hide()
                        contentItem: Text { text: "✕"; font.pixelSize: 14; color: ThemeManager.textMutedColor; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                }
                Text { text: message; font.pixelSize: 12; color: ThemeManager.textColor; wrapMode: Text.Wrap; Layout.fillWidth: true; lineHeight: 1.2 }
                RowLayout {
                    visible: isConfirmation
                    Layout.alignment: Qt.AlignRight
                    spacing: 8
                    Button { text: qsTr("取消"); onClicked: messagePopup.hide() }
                    Button { text: qsTr("确定"); onClicked: { if (onConfirmCallback) onConfirmCallback(); messagePopup.hide(); } }
                }
                Rectangle {
                    visible: !isConfirmation
                    Layout.fillWidth: true; height: 1
                    color: ThemeManager.alpha(getStatusColor(), 0.2)
                    Rectangle { width: parent.width * progressValue; height: parent.height; color: getStatusColor() }
                }
            }

            NumberAnimation { id: showAnim; target: messagePopup; property: "opacity"; from: 0; to: 1; duration: 200 }
            SequentialAnimation {
                id: hideAnim
                NumberAnimation { target: messagePopup; property: "opacity"; from: 1; to: 0; duration: 150 }
                ScriptAction { script: messagePopup.close() }
            }
            Timer {
                id: autoCloseTimer
                interval: 30; repeat: true
                onTriggered: {
                    progressValue -= 30 / duration
                    if (progressValue <= 0) { stop(); messagePopup.hide(); }
                }
            }
            function getStatusColor() {
                if (msgType === 1) return ThemeManager.successColor;
                if (msgType === 2) return ThemeManager.warningColor;
                if (msgType === 3) return ThemeManager.errorColor;
                return ThemeManager.accentColor;
            }
            function getStatusIcon() { return ["ℹ️", "✅", "⚠️", "❌"][msgType] || "ℹ️" }
            function getStatusTitle() { return [qsTr("提示"), qsTr("成功"), qsTr("警告"), qsTr("错误")][msgType] || "" }
        }
    }
}