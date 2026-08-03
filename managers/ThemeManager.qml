// components/ThemeManager.qml
pragma Singleton

import QtQuick 2.15

QtObject {
    id: theme
    property string currentTheme: "dark"

    property var themes: ({
                              "dark": {
                                  "primary"/* 主背景 - 深空黑 */ : "#0c0f14",
                                  "secondary"/* 次级背景 - 深灰 */ : "#161b22",
                                  "tertiary"/* 三级背景 - 炭灰 */ : "#222831",
                                  "cardBackground"/* 卡片背景 - 暗灰 */ : "#1a1e24",
                                  "accent"/* 主强调色 - 电竞红 */ : "#ff4655",
                                  "accentSecondary"/* 次强调色 - 深青 */ : "#007a91",
                                  "highlight"/* 高亮色 - 亮粉红 */ : "#ff6b7a",
                                  "text"/* 主文字 - 浅灰白 */ : "#d0d3d8",
                                  "textMuted"/* 次要文字 - 中灰 */ : "#8b8e93",
                                  "textInverted"/* 反色文字 - 深空黑 */ : "#0c0f14",
                                  "textHover"/* 悬停文字 - 纯白 */ : "#FFFFFF",
                                  "textDisabled"/* 禁用文字 - 深灰 */ : "#5a5e66",
                                  "border"/* 边框 - 暗灰蓝 */ : "#2f3640",
                                  "borderMuted"/* 次要边框 - 中灰 */ : "#3a3f47",
                                  "borderDisabled"/* 禁用边框 - 中灰 */ : "#3a3f47",
                                  "backgroundDisabled"/* 禁用背景 - 暗灰 */ : "#1a1e24",
                                  "backgroundEnabled"/* 启用背景 - 炭灰 */ : "#222831",
                                  "backgroundHover"/* 悬停背景 - 深蓝灰 */ : "#2f3a50",
                                  "success"/* 成功 - 翡翠绿 */ : "#27ae60",
                                  "warning"/* 警告 - 金黄色 */ : "#f4c542",
                                  "error"/* 错误 - 警报红 */ : "#e74c3c",
                                  "overlay"/* 遮罩层 - 深色半透明 */ : "#00000000",
                                  "ripple"/* 涟漪效果 - 浅色涟漪 */ : "#33FFFFFF",
                                  "pulse"/* 脉冲效果 - 浅色脉冲 */ : "#60FFFFFF",
                                  "glow"/* 发光效果 - 电竞红 */ : "#ff4655",
                                  "info"/* 信息 - 亮蓝色 */ : "#3498db",
                                  "link"/* 链接 - 青色 */ : "#00b8d9",
                                  "shade"/* 阴影色 - 深灰 */ : "#1e242d",
                                  "deepGray"/* 中深灰色，不再像黑色 */ : "#454d5a",
                                  "lightGray"/* 中浅灰色，不再刺眼 */ : "#aeb5bd",
                                  "extraPrimary"/* 额外主色 - 深蓝灰 */ : "#1e2733",
                                  "extraSecondary"/* 额外次色 - 中蓝灰 */ : "#28313d",
                                  "extraTertiary"/* 额外三级色 - 浅蓝灰 */ : "#34404e",
                                  "neutral"/* 中性色 - 银灰 */ : "#95a5a6",
                                  "selection"/* 选择背景 - 深蓝 */ : "#264d73",
                                  "hoverHighlight"/* 悬停高亮 - 蓝灰 */ : "#3a5268",
                                  "focus"/* 聚焦边框 - 青色 */ : "#00b8d9",
                                  "shadow"/* 阴影 - 深色阴影 */ : "#99000000",
                                  "gradientStart"/* 渐变开始 - 深灰 */ : "#161b22",
                                  "gradientEnd"/* 渐变结束 - 深空黑 */ : "#0c0f14",
                                  "purple"/* 紫色 - 紫罗兰 */ : "#9b59b6",
                                  "orange"/* 橙色 - 暖橙 */ : "#e67e22",
                                  "cyan"/* 青色 - 绿松石 */ : "#1abc9c",
                                  "pink"/* 粉色 - 玫红 */ : "#e84393",
                                  "gold"/* 金色 - 亮金 */ : "#f1c40f",
                                  "silver"/* 银色 - 银灰 */ : "#8e9599",
                                  "bronze"/* 铜色 - 古铜 */ : "#cd7f32"
                              },
                              "light": {
                                  "primary"/* 主背景 - 鸽子灰 */ : "#eef0f2",
                                  "secondary"/* 次级背景 - 浅钢灰 */ : "#e4e6e8",
                                  "tertiary"/* 三级背景 - 中钢灰 */ : "#d8dadc",
                                  "cardBackground"/* 卡片背景 - 几乎白色 */ : "#fcfcfd",
                                  "accent"/* 主强调色 - 灰蓝色 */ : "#5a8aab",
                                  "accentSecondary"/* 次强调色 - 灰绿色 */ : "#6a8b82",
                                  "highlight"/* 高亮色 - 柔和蓝 */ : "#8ab3cf",
                                  "text"/* 主文字 - 炭灰色 */ : "#3c4043",
                                  "textMuted"/* 次要文字 - 石板灰 */ : "#6c7073",
                                  "textInverted"/* 反色文字 - 几乎白色 */ : "#fcfcfd",
                                  "textHover"/* 悬停文字 - 深炭灰 */ : "#1a1e24",
                                  "textDisabled"/* 禁用文字 - 银灰色 */ : "#a8adb0",
                                  "border"/* 边框 - 浅灰色 */ : "#caced0",
                                  "borderMuted"/* 次要边框 - 中灰色 */ : "#b8bcbf",
                                  "borderDisabled"/* 禁用边框 - 更浅的灰色 */ : "#dce0e3",
                                  "backgroundDisabled"/* 禁用背景 */ : "#ebeef0",
                                  "backgroundEnabled"/* 启用背景 */ : "#fcfcfd",
                                  "backgroundHover"/* 悬停背景 */ : "#e1e4e6",
                                  "success"/* 成功 - 翡翠绿 */ : "#2d8a49",
                                  "warning"/* 警告 - 赭石黄 */ : "#d19a66",
                                  "error"/* 错误 - 砖红色 */ : "#c05c5c",
                                  "overlay"/* 遮罩层 */ : "#00FFFFFF",
                                  "ripple"/* 涟漪效果 */ : "#33555555",
                                  "pulse"/* 脉冲效果 */ : "#605a8aab",
                                  "glow"/* 发光效果 */ : "#5a8aab",
                                  "info"/* 信息 - 石蓝色 */ : "#5f85a6",
                                  "link"/* 链接 - 蓝灰色 */ : "#5a8aab",
                                  "shade"/* 阴影色 */ : "#e6e8ea",
                                  "deepGray"/* 深铅色，增加对比 */ : "#5a6268",
                                  "lightGray"/* 中灰色，确保白字清晰 */ : "#9099a2",
                                  "extraPrimary"/* 额外主色 */ : "#fcfcfd",
                                  "extraSecondary"/* 额外次色 */ : "#eef0f2",
                                  "extraTertiary"/* 额外三级色 */ : "#e4e6e8",
                                  "neutral"/* 中性色 */ : "#9ca0a3",
                                  "selection"/* 选择背景 */ : "#d3d6d9",
                                  "hoverHighlight"/* 悬停高亮 */ : "#d8dadc",
                                  "focus"/* 聚焦边框 */ : "#5a8aab",
                                  "shadow"/* 阴影 */ : "#40000000",
                                  "gradientStart"/* 渐变开始 */ : "#fcfcfd",
                                  "gradientEnd"/* 渐变结束 */ : "#eef0f2",
                                  "purple"/* 紫色 - 灰紫 */ : "#8b82a2",
                                  "orange"/* 橙色 - 土橙 */ : "#c48a68",
                                  "cyan"/* 青色 - 灰青 */ : "#609393",
                                  "pink"/* 粉色 - 灰粉 */ : "#b48494",
                                  "gold"/* 金色 - 暗金 */ : "#b59d5b",
                                  "silver"/* 银色 - 铅灰 */ : "#7b7f82",
                                  "bronze"/* 铜色 - 古铜灰 */ : "#a88d7a"
                              }
                          })

    // 颜色属性
    property color primaryColor: themes[currentTheme].primary
    property color secondaryColor: themes[currentTheme].secondary
    property color tertiaryColor: themes[currentTheme].tertiary
    property color cardBackgroundColor: themes[currentTheme].cardBackground
    property color accentColor: themes[currentTheme].accent
    property color accentSecondaryColor: themes[currentTheme].accentSecondary
    property color highlightColor: themes[currentTheme].highlight
    property color textColor: themes[currentTheme].text
    property color textMutedColor: themes[currentTheme].textMuted
    property color textInvertedColor: themes[currentTheme].textInverted
    property color textHoverColor: themes[currentTheme].textHover
    property color textDisabledColor: themes[currentTheme].textDisabled
    property color borderColor: themes[currentTheme].border
    property color borderMutedColor: themes[currentTheme].borderMuted
    property color borderDisabledColor: themes[currentTheme].borderDisabled
    property color backgroundDisabledColor: themes[currentTheme].backgroundDisabled
    property color backgroundEnabledColor: themes[currentTheme].backgroundEnabled
    property color backgroundHoverColor: themes[currentTheme].backgroundHover
    property color successColor: themes[currentTheme].success
    property color warningColor: themes[currentTheme].warning
    property color errorColor: themes[currentTheme].error
    property color overlayColor: themes[currentTheme].overlay
    property color rippleColor: themes[currentTheme].ripple
    property color pulseColor: themes[currentTheme].pulse
    property color glowColor: themes[currentTheme].glow
    property color infoColor: themes[currentTheme].info
    property color linkColor: themes[currentTheme].link
    property color shadeColor: themes[currentTheme].shade
    property color deepGrayColor: themes[currentTheme].deepGray
    property color lightGrayColor: themes[currentTheme].lightGray
    property color extraPrimaryColor: themes[currentTheme].extraPrimary
    property color extraSecondaryColor: themes[currentTheme].extraSecondary
    property color extraTertiaryColor: themes[currentTheme].extraTertiary
    property color neutralColor: themes[currentTheme].neutral
    property color selectionColor: themes[currentTheme].selection
    property color hoverHighlightColor: themes[currentTheme].hoverHighlight
    property color focusColor: themes[currentTheme].focus
    property color shadowColor: themes[currentTheme].shadow
    property color gradientStartColor: themes[currentTheme].gradientStart
    property color gradientEndColor: themes[currentTheme].gradientEnd
    property color purpleColor: themes[currentTheme].purple
    property color orangeColor: themes[currentTheme].orange
    property color cyanColor: themes[currentTheme].cyan
    property color pinkColor: themes[currentTheme].pink
    property color goldColor: themes[currentTheme].gold
    property color silverColor: themes[currentTheme].silver
    property color bronzeColor: themes[currentTheme].bronze

    // 字体系统
    property int fontSizeH1: 28
    property int fontSizeH2: 22
    property int fontSizeH3: 18
    property int fontSizeH4: 16
    property int fontSizeBody: 14
    property int fontSizeSmall: 12
    property int fontSizeTiny: 10
    property int fontSizeGiant: 42
    property int fontSizeHuge: 48
    property int fontSizeTitanic: 96

    property int fontWeightBold: Font.Bold
    property int fontWeightMedium: Font.Medium
    property int fontWeightNormal: Font.Normal

    property string fontFamilyMain: "Arial, sans-serif"
    property string fontFamilyTime: "Arial, Helvetica, sans-serif"

    // 间距系统
    property int spacingXs: 4
    property int spacingSm: 8
    property int spacingMd: 16
    property int spacingLg: 24
    property int spacingXl: 32
    property int spacingXxl: 48

    // 圆角系统
    property int borderRadiusSm: 4
    property int borderRadiusMd: 8
    property int borderRadiusLg: 12
    property int borderRadiusXl: 16

    // 阴影系统
    property string shadowSm: "0 1px 2px 0 rgba(0,0,0,0.1)"
    property string shadowMd: "0 4px 6px -1px rgba(0,0,0,0.1), 0 2px 4px -1px rgba(0,0,0,0.06)"
    property string shadowLg: "0 10px 15px -3px rgba(0,0,0,0.1), 0 4px 6px -2px rgba(0,0,0,0.05)"

    // 函数
    function setTheme(themeName) {
        if (themes.hasOwnProperty(themeName)) {
            currentTheme = themeName
        }
    }

    function getAvailableThemes() {
        return Object.keys(themes)
    }

    function toggleTheme() {
        if (currentTheme === "dark") {
            currentTheme = "light"
        } else {
            currentTheme = "dark"
        }
    }

    // 工具函数
    function alpha(color, opacity) {
        return Qt.rgba(color.r, color.g, color.b, opacity)
    }
}
