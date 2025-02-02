import Blockstream.Green 0.1
import Blockstream.Green.Core 0.1
import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Controls.Material 2.3
import QtQuick.Layouts 1.13
import QtQuick.Window 2.12

ApplicationWindow {
    id: window

    readonly property WalletView currentWalletView: stack_layout.currentWalletView
    readonly property Wallet currentWallet: currentWalletView ? currentWalletView.wallet : null
    readonly property Account currentAccount: currentWalletView ? currentWalletView.currentAccount : null

    property Navigation navigation: Navigation {
        location: '/home'
    }
    function link(url, text) {
        return `<style>a:link { color: "#00B45A"; text-decoration: none; }</style><a href="${url}">${text || url}</a>`
    }

    property var icons: ({
        'liquid': 'qrc:/svg/liquid.svg',
        'testnet-liquid': 'qrc:/svg/testnet-liquid.svg',
        'bitcoin': 'qrc:/svg/btc.svg',
        'testnet': 'qrc:/svg/btc_testnet.svg'
    })

    property Constants constants: Constants {}

    function formatTransactionTimestamp(tx) {
        return new Date(tx.created_at_ts / 1000).toLocaleString(locale.dateTimeFormat(Locale.LongFormat))
    }

    function accountName(account) {
        if (!account) return ''
        if (account.name !== '') return account.name
        if (account.mainAccount) return qsTrId('id_main_account')
        return qsTrId('Account %1').arg(account.pointer)
    }

    x: Settings.windowX
    y: Settings.windowY
    width: Settings.windowWidth
    height: Settings.windowHeight

    onXChanged: Settings.windowX = x
    onYChanged: Settings.windowY = y
    onWidthChanged: Settings.windowWidth = width
    onHeightChanged: Settings.windowHeight = height
    onCurrentWalletChanged: {
        if (currentWallet && currentWallet.persisted) {
            Settings.updateRecentWallet(currentWallet.id)
        }
    }

    minimumWidth: 900
    minimumHeight: 600
    visible: true
    color: constants.c900
    title: {
        const parts = Qt.application.arguments.indexOf('--debugnavigation') > 0 ? [navigation.location] : []
        if (currentWallet) {
            parts.push(font_metrics.elidedText(currentWallet.name, Qt.ElideRight, window.width / 3));
            if (currentAccount) parts.push(font_metrics.elidedText(accountName(currentAccount), Qt.ElideRight, window.width / 3));
        }
        parts.push('Blockstream Green');
        if (build_type !== 'release') parts.push(`[${build_type}]`)
        return parts.join(' - ');
    }
    FontMetrics {
        id: font_metrics
    }

    RowLayout {
        id: main_layout
        anchors.fill: parent
        spacing: 0
        SideBar {
            Layout.fillHeight: true
        }
        ColumnLayout {
            StackLayout {
                id: stack_layout
                Layout.fillWidth: true
                Layout.fillHeight: true
                readonly property WalletView currentWalletView: currentIndex < 0 ? null : (stack_layout.children[currentIndex].currentWalletView || null)
                Binding on currentIndex {
                    delayed: true
                    value: {
                        let index = stack_layout.currentIndex
                        for (let i = 0; i < stack_layout.children.length; ++i) {
                            const child = stack_layout.children[i]
                            if (!(child instanceof Item)) continue
                            if (child.active && child.enabled) index = i
                        }
                        return index
                    }
                }
                HomeView {
                    readonly property bool active: navigation.path === '/home'
                }
                PreferencesView {
                    readonly property bool active: navigation.path === '/preferences'
                }
                JadeView {
                    id: jade_view
                    readonly property bool active: navigation.path.startsWith('/jade')
                }
                LedgerDevicesView {
                    id: ledger_view
                    readonly property bool active: navigation.path.startsWith('/ledger')
                }
                NetworkView {
                    network: 'bitcoin'
                    title: qsTrId('id_bitcoin_wallets')
                }
                NetworkView {
                    network: 'liquid'
                    title: qsTrId('id_liquid_wallets')
                }
                NetworkView {
                    enabled: Settings.enableTestnet
                    network: 'testnet-liquid'
                    title: qsTrId('id_liquid_testnet_wallets')
                }
                NetworkView {
                    enabled: Settings.enableTestnet
                    network: 'testnet'
                    title: qsTrId('id_testnet_wallets')
                }
            }
        }
    }

    DialogLoader {
        active: navigation.path.match(/\/signup$/)
        dialog: SignupDialog {
            onRejected: navigation.pop()
        }
    }

    DialogLoader {
        active: navigation.path.match(/\/restore$/)
        dialog: RestoreDialog {
            onRejected: navigation.pop()
        }
    }

    DialogLoader {
        properties: {
            const [,, wallet_id] = navigation.path.split('/')
            const wallet = WalletManager.wallet(wallet_id)
            return { wallet }
        }
        active: properties.wallet && !properties.wallet.ready
        dialog: LoginDialog {
            onRejected: navigation.pop()
        }
    }

    DebugActiveFocus {
    }

    Component {
        id: create_account_dialog
        CreateAccountDialog {}
    }

    Component {
        id: remove_wallet_dialog
        AbstractDialog {
            title: qsTrId('id_remove_wallet')
            property Wallet wallet
            anchors.centerIn: parent
            modal: true
            onAccepted: WalletManager.removeWallet(wallet)
            contentItem: ColumnLayout {
                spacing: 16
                SectionLabel {
                    text: qsTrId('id_name')
                }
                Label {
                    text: wallet.name
                }
                SectionLabel {
                    text: qsTrId('id_network')
                }
                RowLayout {
                    Layout.fillHeight: false
                    spacing: 8
                    Image {
                        sourceSize.width: 16
                        sourceSize.height: 16
                        source: icons[wallet.network.key]
                    }
                    Label {
                        Layout.fillWidth: true
                        text: wallet.network.displayName
                    }
                }
                SectionLabel {
                    text: qsTrId('id_confirm_action')
                }
                GTextField {
                    Layout.minimumWidth: 300
                    Layout.fillWidth: true
                    id: confirm_field
                    placeholderText: qsTrId('id_confirm_by_typing_the_wallet')
                }
                Label {
                    text: qsTrId('id_backup_your_mnemonic_before')
                }
            }
            footer: GPane {
                rightPadding: 16
                bottomPadding: 8
                contentItem: RowLayout {
                    HSpacer {
                    }
                    GButton {
                        enabled: confirm_field.text === wallet.name
                        destructive: true
                        large: true
                        text: qsTrId('id_remove')
                        onClicked: accept()
                    }
                }
            }
        }
    }

    Component {
        id: export_transactions_popup
        Popup {
            required property Account account
            id: dialog
            anchors.centerIn: Overlay.overlay
            closePolicy: Popup.NoAutoClose
            modal: true
            Overlay.modal: Rectangle {
                color: "#70000000"
            }
            onClosed: destroy()
            onOpened: controller.save()
            ExportTransactionsController {
                id: controller
                account: dialog.account
                onSaved: dialog.close()
            }
            BusyIndicator {}
        }
    }

    Component {
        id: export_addresses_popup
        Popup {
            required property Account account
            id: dialog
            anchors.centerIn: Overlay.overlay
            closePolicy: Popup.NoAutoClose
            modal: true
            Overlay.modal: Rectangle {
                color: "#70000000"
            }
            onClosed: destroy()
            onOpened: controller.save()
            ExportAddressesController {
                id: controller
                account: dialog.account
                onSaved: dialog.close()
            }
            BusyIndicator {}
        }

    }

    Component {
        id: session_tor_cirtcuit_view
        SessionTorCircuitToolButton {
        }
    }
    Component {
        id: session_connect_view
        SessionConnectToolButton {
        }
    }
}
