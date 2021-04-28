import Blockstream.Green 0.1
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

ColumnLayout {
    required property Account account

    id: self
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: constants.p2

    RowLayout {
        id: tags_layout
        Layout.fillWidth: true
        implicitHeight: 50
        spacing: constants.p1

        ButtonGroup {
            id: button_group
        }

        Repeater {
            model: account.wallet.network.liquid ? ['all', 'csv', 'p2wsh', 'not confidential'] : ['all', 'csv', 'p2wsh', 'dust', 'locked']
            delegate: Button {
                id: self
                ButtonGroup.group: button_group
                checked: index === 0
                checkable: true
                background: Rectangle {
                    id: rectangle
                    radius: 4
                    color: self.checked ? constants.c200 : constants.c300
                }
                text: modelData
                ToolTip.delay: 300
                ToolTip.visible: hovered
                ToolTip.text: qsTrId(`id_tag_${modelData}`);
            }
        }

        HSpacer {
        }
    }

    ListView {
        id: list_view
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 8
        model: OutputListModelFilter {
            id: output_model_filter
            filter: button_group.checkedButton.text
            model: OutputListModel {
                id: output_model
                account: self.account
            }
        }
        delegate: OutputDelegate {
            hoverEnabled: false
            width: list_view.width-constants.p1
        }

        ScrollIndicator.vertical: ScrollIndicator { }

        BusyIndicator {
            width: 32
            height: 32
            running: output_model.fetching
            anchors.margins: 8
            Layout.alignment: Qt.AlignHCenter
            opacity: output_model.fetching ? 1 : 0
            Behavior on opacity { OpacityAnimator {} }
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Label {
            id: label
            visible: list_view.count === 0
            anchors.centerIn: parent
            color: 'white'
            text: {
                if (output_model_filter.filter === '') {
                    return qsTrId(`You'll see your coins here when you receive funds`)
                } else {
                    return qsTrId('There are no results for the applied filter')
                }
            }
        }
    }
}
