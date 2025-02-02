import Blockstream.Green 0.1
import Blockstream.Green.Core 0.1
import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.0

AbstractDialog {
    id: self
    icon: {
        const { network, type } = navigation.param
        if (type === 'amp') return 'qrc:/svg/amp.svg'
        if (network) return icons[network]
        return ''
    }
    title: qsTrId('id_restore_green_wallet')
    width: 900
    height: 500
    closePolicy: Popup.NoAutoClose

    Navigation {
        id: navigation
        property RestoreController controller
        location: window.navigation.location
    }


    Connections {
        enabled: target
        target: navigation.controller ? navigation.controller.wallet : null
        function onActivityCreated(activity) {
            if (activity instanceof CheckRestoreActivity) {
            } else if (activity instanceof AcceptRestoreActivity) {
                activity.finished.connect(() => {
                    const wallet = navigation.controller.wallet
                    window.navigation.go(`/${wallet.network.key}/${wallet.id}`)
                })
            }
        }
    }



    contentItem: StackLayout {
        property Item currentItem: {
            if (stack_layout.currentIndex < 0) return null
            let item = stack_layout.children[stack_layout.currentIndex]
            if (item instanceof Loader) item = item.item
            if (item) item.focus = true
            return item
        }
        id: stack_layout
        currentIndex: {
            let index = -1
            for (let i = 0; i < stack_layout.children.length; ++i) {
                let child = stack_layout.children[i]
                if (!(child instanceof Item)) continue
                if (child.active) index = i
            }
            return index
        }
        SelectNetworkView {
            readonly property bool active: true
            showAMP: true
            view: 'restore'
        }
        AnimLoader {
            active: (navigation.param.network || '') !== ''
            animated: self.opened
            sourceComponent: MnemonicEditor {
                id: editor
                title: qsTrId('id_enter_your_recovery_phrase')
                footer: RowLayout {
                    spacing: constants.s1
                    GButton {
                        visible: navigation.canPop
                        text: qsTrId('id_back')
                        onClicked: navigation.pop()
                    }
                    GButton {
                        text: qsTrId('id_clear')
                        enabled: !editor.controller.active
                        onClicked: editor.controller.clear();
                    }
                    HSpacer {
                    }
                    GButton {
                        text: qsTrId('id_continue')
                        enabled: editor.valid
                        onClicked: navigation.set({ mnemonic: editor.mnemonic, password: editor.password })
                    }
                }
            }
        }
        AnimLoader {
            active: {
                const { mnemonic, password } = navigation.param
                if (mnemonic === undefined) return false
                const words = mnemonic.split(',')
                if (words.length === 12) return true
                if (words.length === 24) return true
                if (words.length === 27 && password !== undefined) return true
                return false
            }
            animated: self.opened
            sourceComponent: DiscoverServerTypeView {
            }
        }

        AnimLoader {
            active: navigation.controller
            animated: self.opened
            sourceComponent: Page {
                background: null
                footer: RowLayout {
                    spacing: constants.s1
                    GButton {
                        text: qsTrId('id_back')
                        onClicked: navigation.pop()
                    }
                    HSpacer {
                    }
                }
                contentItem: ColumnLayout {
                    spacing: 16
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTrId('id_set_a_new_pin')
                    }
                    PinView {
                        Layout.alignment: Qt.AlignHCenter
                        id: pin_view
                        onPinChanged: {
                            if (!pin.valid) return
                            navigation.set({ pin: pin.value })
                            Qt.callLater(pin_view.clear)
                        }
                    }
                    VSpacer {
                    }
                }
            }
        }

        AnimLoader {
            active: navigation.controller && navigation.controller.pin.length === 6
            animated: self.opened
            sourceComponent: Page {
                background: null
                footer: RowLayout {
                    spacing: constants.s1
                    GButton {
                        text: qsTrId('id_back')
                        onClicked: navigation.pop()
                    }
                    HSpacer {
                    }
                }
                contentItem: ColumnLayout {
                    spacing: 16
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTrId('id_verify_your_pin')
                    }
                    PinView {
                        Layout.alignment: Qt.AlignHCenter
                        onPinChanged: {
                            if (!pin.valid) return
                            if (pin.value !== navigation.controller.pin) {
                                clear();
                                ToolTip.show(qsTrId('id_pins_do_not_match_please_try'), 1000);
                            } else {
                                onTriggered: navigation.controller.accept()
                            }
                        }
                    }
                    VSpacer {
                    }
                }
            }
        }
    }

    Component {
        id: check_view
        RowLayout {
            required property CheckRestoreActivity activity
            id: self
            BusyIndicator {
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignCenter
            }
            Label {
                Layout.alignment: Qt.AlignCenter
                text: 'Checking'
            }
        }
    }

    Component {
        id: accept_view
        RowLayout {
            required property AcceptRestoreActivity activity
            id: self
            BusyIndicator {
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignCenter
            }
            Label {
                Layout.alignment: Qt.AlignCenter
                text: 'Restoring'
            }
        }
    }
}
