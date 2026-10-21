import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    width: 960
    height: 600
    minimumWidth: 640
    minimumHeight: 400
    visible: true
    title: "NeoVLC"

    readonly property bool mediaLoaded: vlcPlayer.available && vlcPlayer.mediaPath.length > 0
    readonly property bool seekable: vlcPlayer.duration > 0

    function formatTime(milliseconds) {
        const totalSeconds = Math.max(0, Math.floor(milliseconds / 1000))
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 6

            Button {
                text: qsTr("Open File")
                onClicked: fileDialog.open()
            }

            Button {
                text: qsTr("Play")
                enabled: mediaLoaded
                onClicked: vlcPlayer.play()
            }

            Button {
                text: qsTr("Pause")
                enabled: mediaLoaded
                onClicked: vlcPlayer.pause()
            }

            Button {
                text: qsTr("Stop")
                enabled: mediaLoaded
                onClicked: vlcPlayer.stop()
            }

            Label {
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                text: mediaLoaded
                    ? vlcPlayer.mediaPath
                    : qsTr("No media loaded")
            }

            Label {
                text: qsTr("Volume")
            }

            Slider {
                id: volumeSlider
                Layout.preferredWidth: 120
                from: 0
                to: 200
                stepSize: 1
                value: vlcPlayer.volume
                onMoved: vlcPlayer.setVolume(Math.round(value))
            }

            Label {
                Layout.preferredWidth: 40
                horizontalAlignment: Text.AlignRight
                text: Math.round(volumeSlider.value) + "%"
            }
        }
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 8

            Label {
                Layout.preferredWidth: 44
                text: formatTime(vlcPlayer.position)
            }

            Slider {
                id: seekSlider
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, vlcPlayer.duration)
                enabled: seekable
                onPressedChanged: {
                    if (!pressed && seekable)
                        vlcPlayer.seek(Math.round(value))
                }
            }

            Label {
                Layout.preferredWidth: 44
                horizontalAlignment: Text.AlignRight
                text: formatTime(vlcPlayer.duration)
            }

            Label {
                Layout.preferredWidth: 60
                horizontalAlignment: Text.AlignRight
                text: vlcPlayer.playbackStateName
            }
        }
    }

    Connections {
        target: vlcPlayer

        function onPositionChanged() {
            if (!seekSlider.pressed)
                seekSlider.value = vlcPlayer.position
        }

        function onMediaOpened() {
            videoItem.clear()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
    }

    VideoItem {
        id: videoItem
        anchors.fill: parent
        output: vlcVideo
    }

    Label {
        anchors.centerIn: parent
        visible: !mediaLoaded
        text: "NeoVLC"
        font.pixelSize: 32
    }

    Label {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        visible: vlcPlayer.errorString.length > 0
        wrapMode: Text.Wrap
        color: "#c62828"
        text: vlcPlayer.errorString
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Open Video File")
        nameFilters: [
            qsTr("Video files (*.mp4 *.mkv *.webm *.avi *.mov *.m4v *.mpg *.mpeg)"),
            qsTr("All files (*)")
        ]
        onAccepted: vlcPlayer.openFile(selectedFile)
    }
}
