#include "account.h"
#include "asset.h"
#include "json.h"
#include "network.h"
#include "session.h"
#include "output.h"
#include "util.h"
#include "wallet.h"
#include <gdk.h>

Output::Output(const QJsonObject& data, Account* account)
    : QObject(account)
    , m_account(account)
{
    updateFromData(data);
    connect(account->wallet(), &Wallet::blockHeightChanged, this, &Output::updateExpired);
}

void Output::updateFromData(const QJsonObject& data)
{
    if (m_data == data) return;
    m_data = data;
    emit dataChanged(m_data);
    update();
}

void Output::update()
{
    if (!m_asset && m_account->wallet()->network()->isLiquid()) {
        auto asset_id = m_data["asset_id"].toString();
        m_asset = m_account->wallet()->getOrCreateAsset(asset_id);
        emit assetChanged(m_asset);
    }

    setDust(m_data["satoshi"].toDouble() < 1092 && !m_account->wallet()->network()->isLiquid());
    setCanBeLocked(m_data["satoshi"].toDouble() < 2184);
    setLocked(m_data["user_status"].toInt() == 1);
    setConfidential(m_data["confidential"].toBool());
    setUnconfirmed(m_data["block_height"].toDouble() == 0);
    setAddressType(m_data["address_type"].toString());
    updateExpired();
}

void Output::updateExpired()
{
    const auto expiry_height = m_data.value("expiry_height");
    if (expiry_height.isDouble()) {
        const auto block_height = account()->wallet()->blockHeight();
        setExpired(expiry_height.toInt() <= block_height);
    } else {
        setExpired(false);
    }
}

void Output::setExpired(bool expired)
{
    if (m_expired == expired) return;
    m_expired = expired;
    emit expiredChanged(m_expired);
}

void Output::setDust(bool dust)
{
    if (m_dust == dust) return;
    m_dust = dust;
    emit dustChanged(m_dust);
}

void Output::setLocked(bool locked)
{
    if (m_locked == locked) return;
    m_locked = locked;
    emit lockedChanged(m_locked);
}

void Output::setCanBeLocked(bool canBeLocked)
{
    if (m_can_be_locked == canBeLocked) return;
    m_can_be_locked = canBeLocked;
    emit canBeLockedChanged(m_can_be_locked);
}

void Output::setConfidential(bool confidential)
{
    if (m_confidential == confidential) return;
    m_confidential = confidential;
    emit confidentialChanged(m_confidential);
}

void Output::setUnconfirmed(bool unconfirmed)
{
    if (m_unconfirmed == unconfirmed) return;
    m_unconfirmed = unconfirmed;
    emit unconfirmedChanged(m_unconfirmed);
}

void Output::setAddressType(const QString& address_type)
{
    if (m_address_type == address_type) return;
    m_address_type = address_type;
    emit addressTypeChanged(m_address_type);
}

