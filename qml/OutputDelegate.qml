import Blockstream.Green 0.1
import Blockstream.Green.Core 0.1
import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.5
import QtQuick.Controls.Material 2.3
import QtQuick.Layouts 1.12

Button {
    required property var output

    function formatAmount(amount, include_ticker = true) {
        const unit = wallet.settings.unit;
        return output.account.wallet.network.liquid ? output.asset.formatAmount(amount, true) : wallet.formatAmount(amount || 0, include_ticker, unit)
    }

    id: self
    hoverEnabled: true
    padding: constants.p3
    background: Rectangle {
        color: constants.c500
        radius: 4
    }
    checkable: true
    checked: output.selected
    onCheckedChanged: output.selected = checked
    spacing: constants.p2
    contentItem: RowLayout {
        spacing: constants.p2
        ColumnLayout {
            Layout.fillWidth: false
            Layout.preferredHeight: 80
            Image {
                visible: !output.account.wallet.network.liquid
                sourceSize.height: 36
                sourceSize.width: 36
                source: icons[wallet.network.id]
            }
            Loader {
                active: output.asset
                visible: active
                sourceComponent: AssetIcon {
                    asset: output.asset
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                }
            }
            VSpacer {
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: constants.p1
            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: formatAmount(output.data['satoshi'], true)
                font.pixelSize: 16
                font.styleName: 'Medium'
            }
            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: output.data['txhash'] + ':' + output.data['pt_idx']
                font.pixelSize: 14
                font.styleName: 'Regular'
            }
            RowLayout {
                Tag {
                    visible: output.locked
                    text: 'LOCKED'
                    ToolTip.text: qsTrId('id_tag_locked')
                }
                Tag {
                    text: 'DUST'
                    visible: output.dust
                    ToolTip.text: qsTrId('id_tag_dust')
                }
                Tag {
                    text: 'NOT CONFIDENTIAL'
                    visible: output.account.wallet.network.liquid && !output.confidential
                    ToolTip.text: qsTrId('id_tag_not_confidential')
                }
                Tag {
                    text: output.addressType
                    font.capitalization: Font.AllUppercase
                    ToolTip.text: qsTrId('id_tag_address_type')
                }
                Tag {
                    visible: output.unconfirmed
                    text: 'UNCONFIRMED'
                    color: '#d2934a'
                    font.capitalization: Font.AllUppercase
                    ToolTip.text: qsTrId('id_tag_unconfirmed')
                }
            }
            VSpacer {
            }
        }
        ColumnLayout {
            Layout.fillWidth: false
            Layout.preferredHeight: 80
            CheckBox {
                checked: self.checked
                onCheckedChanged: output.selected = checked
            }
        }
    }
}
