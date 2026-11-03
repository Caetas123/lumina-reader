import QtQuick

Item {
    id: icon

    property string name: ""
    property color iconColor: Theme.textSecondary
    property real strokeWidth: 1.7

    implicitWidth: 20
    implicitHeight: 20

    onNameChanged: canvas.requestPaint()
    onIconColorChanged: canvas.requestPaint()
    onStrokeWidthChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            var context = getContext("2d")
            context.reset()
            context.save()
            context.scale(width / 24, height / 24)
            context.strokeStyle = icon.iconColor
            context.fillStyle = icon.iconColor
            context.lineWidth = icon.strokeWidth
            context.lineCap = "round"
            context.lineJoin = "round"

            if (icon.name === "book" || icon.name === "library") {
                context.beginPath()
                context.moveTo(3.5, 5.5)
                context.quadraticCurveTo(7.4, 4.2, 11.4, 6.2)
                context.lineTo(11.4, 19.5)
                context.quadraticCurveTo(7.3, 17.5, 3.5, 18.8)
                context.closePath()
                context.stroke()
                context.beginPath()
                context.moveTo(20.5, 5.5)
                context.quadraticCurveTo(16.6, 4.2, 12.6, 6.2)
                context.lineTo(12.6, 19.5)
                context.quadraticCurveTo(16.7, 17.5, 20.5, 18.8)
                context.closePath()
                context.stroke()
                context.beginPath()
                context.moveTo(12, 6.2)
                context.lineTo(12, 19.5)
                context.stroke()
            } else if (icon.name === "star" || icon.name === "favorite") {
                var star = []
                for (var starIndex = 0; starIndex < 10; ++starIndex) {
                    var starAngle = -Math.PI / 2 + starIndex * Math.PI / 5
                    var starRadius = starIndex % 2 === 0 ? 9.2 : 4.1
                    star.push({ x: 12 + Math.cos(starAngle) * starRadius,
                                y: 12 + Math.sin(starAngle) * starRadius })
                }
                context.beginPath()
                context.moveTo(star[0].x, star[0].y)
                for (var starPoint = 1; starPoint < star.length; ++starPoint) {
                    context.lineTo(star[starPoint].x, star[starPoint].y)
                }
                context.closePath()
                context.stroke()
            } else if (icon.name === "clock") {
                context.beginPath()
                context.arc(12, 12, 8.4, 0, Math.PI * 2)
                context.stroke()
                context.beginPath()
                context.moveTo(12, 7.4)
                context.lineTo(12, 12)
                context.lineTo(15.2, 14.2)
                context.stroke()
            } else if (icon.name === "moon") {
                context.beginPath()
                context.arc(11.3, 11.3, 7.5, -0.8, Math.PI * 1.45)
                context.stroke()
                context.beginPath()
                context.arc(14.8, 8.4, 7.1, 1.0, Math.PI * 1.9)
                context.stroke()
            } else if (icon.name === "check") {
                context.beginPath()
                context.moveTo(4.5, 12.3)
                context.lineTo(9.2, 17)
                context.lineTo(19.8, 6.5)
                context.stroke()
            } else if (icon.name === "close") {
                context.beginPath()
                context.moveTo(6, 6)
                context.lineTo(18, 18)
                context.moveTo(18, 6)
                context.lineTo(6, 18)
                context.stroke()
            } else if (icon.name === "sparkle") {
                context.beginPath()
                context.moveTo(12, 2.8)
                context.lineTo(13.8, 10.2)
                context.lineTo(21.2, 12)
                context.lineTo(13.8, 13.8)
                context.lineTo(12, 21.2)
                context.lineTo(10.2, 13.8)
                context.lineTo(2.8, 12)
                context.lineTo(10.2, 10.2)
                context.closePath()
                context.stroke()
            } else if (icon.name === "settings") {
                context.beginPath()
                context.arc(12, 12, 3.2, 0, Math.PI * 2)
                context.stroke()
                for (var gearIndex = 0; gearIndex < 8; ++gearIndex) {
                    var gearAngle = gearIndex * Math.PI / 4
                    var x1 = 12 + Math.cos(gearAngle) * 6.2
                    var y1 = 12 + Math.sin(gearAngle) * 6.2
                    var x2 = 12 + Math.cos(gearAngle) * 9.3
                    var y2 = 12 + Math.sin(gearAngle) * 9.3
                    context.beginPath()
                    context.moveTo(x1, y1)
                    context.lineTo(x2, y2)
                    context.stroke()
                }
                context.beginPath()
                context.arc(12, 12, 7.2, 0, Math.PI * 2)
                context.stroke()
            } else if (icon.name === "plus") {
                context.beginPath()
                context.moveTo(12, 5)
                context.lineTo(12, 19)
                context.moveTo(5, 12)
                context.lineTo(19, 12)
                context.stroke()
            } else if (icon.name === "folder") {
                context.beginPath()
                context.moveTo(3.5, 7.5)
                context.lineTo(9, 7.5)
                context.lineTo(11, 9.5)
                context.lineTo(20.5, 9.5)
                context.lineTo(20.5, 18.5)
                context.lineTo(3.5, 18.5)
                context.closePath()
                context.stroke()
                context.beginPath()
                context.moveTo(3.5, 9.5)
                context.lineTo(20.5, 9.5)
                context.stroke()
            } else if (icon.name === "watch") {
                context.beginPath()
                context.moveTo(3.5, 7.5)
                context.lineTo(9, 7.5)
                context.lineTo(11, 9.5)
                context.lineTo(20.5, 9.5)
                context.lineTo(20.5, 18.5)
                context.lineTo(3.5, 18.5)
                context.closePath()
                context.stroke()
                context.beginPath()
                context.moveTo(13, 12)
                context.lineTo(13, 16.5)
                context.lineTo(16.4, 14.25)
                context.closePath()
                context.stroke()
            } else if (icon.name === "list") {
                for (var listIndex = 0; listIndex < 3; ++listIndex) {
                    var listY = 6.5 + listIndex * 5.5
                    context.beginPath()
                    context.moveTo(3.5, listY)
                    context.lineTo(6, listY)
                    context.moveTo(9.5, listY)
                    context.lineTo(20.5, listY)
                    context.stroke()
                }
            } else if (icon.name === "history") {
                context.beginPath()
                context.arc(12, 12, 8, -0.4, Math.PI * 1.75)
                context.stroke()
                context.beginPath()
                context.moveTo(4.5, 5.2)
                context.lineTo(4.3, 10.2)
                context.lineTo(8.7, 8.3)
                context.stroke()
                context.beginPath()
                context.moveTo(12, 7.5)
                context.lineTo(12, 12)
                context.lineTo(15, 13.8)
                context.stroke()
            } else if (icon.name === "search") {
                context.beginPath()
                context.arc(10.3, 10.3, 5.8, 0, Math.PI * 2)
                context.stroke()
                context.beginPath()
                context.moveTo(14.7, 14.7)
                context.lineTo(20, 20)
                context.stroke()
            } else if (icon.name === "shield") {
                context.beginPath()
                context.moveTo(12, 3.2)
                context.lineTo(19.5, 6.2)
                context.lineTo(18.7, 13.8)
                context.quadraticCurveTo(17.8, 18.4, 12, 20.8)
                context.quadraticCurveTo(6.2, 18.4, 5.3, 13.8)
                context.lineTo(4.5, 6.2)
                context.closePath()
                context.stroke()
            } else if (icon.name === "menu") {
                for (var menuIndex = 0; menuIndex < 3; ++menuIndex) {
                    var menuY = 7 + menuIndex * 5
                    context.beginPath()
                    context.moveTo(4, menuY)
                    context.lineTo(20, menuY)
                    context.stroke()
                }
            } else if (icon.name === "bookmark") {
                context.beginPath()
                context.moveTo(6, 4)
                context.lineTo(18, 4)
                context.lineTo(18, 20)
                context.lineTo(12, 16.3)
                context.lineTo(6, 20)
                context.closePath()
                context.stroke()
            } else if (icon.name === "type") {
                context.beginPath()
                context.moveTo(4, 6)
                context.lineTo(20, 6)
                context.moveTo(12, 6)
                context.lineTo(12, 19)
                context.moveTo(8, 19)
                context.lineTo(16, 19)
                context.stroke()
            } else if (icon.name === "fullscreen") {
                context.beginPath()
                context.moveTo(4, 9)
                context.lineTo(4, 4)
                context.lineTo(9, 4)
                context.moveTo(15, 4)
                context.lineTo(20, 4)
                context.lineTo(20, 9)
                context.moveTo(20, 15)
                context.lineTo(20, 20)
                context.lineTo(15, 20)
                context.moveTo(9, 20)
                context.lineTo(4, 20)
                context.lineTo(4, 15)
                context.stroke()
            } else if (icon.name === "chevron-left") {
                context.beginPath()
                context.moveTo(14.5, 5)
                context.lineTo(7.5, 12)
                context.lineTo(14.5, 19)
                context.stroke()
            } else if (icon.name === "chevron-right") {
                context.beginPath()
                context.moveTo(9.5, 5)
                context.lineTo(16.5, 12)
                context.lineTo(9.5, 19)
                context.stroke()
            } else if (icon.name === "edit") {
                context.beginPath()
                context.moveTo(5, 19)
                context.lineTo(8.8, 18.2)
                context.lineTo(19, 8)
                context.lineTo(16, 5)
                context.lineTo(5.8, 15.2)
                context.closePath()
                context.stroke()
            }

            context.restore()
        }

        Connections {
            target: Theme
            function onModeChanged() { canvas.requestPaint() }
        }
    }
}
