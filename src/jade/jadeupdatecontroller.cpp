#include "activitymanager.h"
#include "httpmanager.h"
#include "jadeupdatecontroller.h"
#include "jadedevice.h"
#include "network.h"
#include "networkmanager.h"
#include "semver.h"
#include "session.h"
#include "json.h"
#include "jadeapi.h"

#include <QCryptographicHash>
#include <QFile>

#include <gdk.h>

namespace {

const QString JADE_FW_VERSIONS_FILE = "LATEST";

const QString JADE_FW_SERVER_HTTPS = "https://jadefw.blockstream.com";
static const QString JADE_FW_SERVER_ONION = "http://vgza7wu4h7osixmrx6e4op5r72okqpagr3w6oupgsvmim4cz3wzdgrad.onion";

static const QString JADE_FW_JADE_PATH = "/bin/jade/";
static const QString JADE_FW_JADE1_1_PATH = "/bin/jade1.1/";
static const QString JADE_FW_JADEDEV_PATH = "/bin/jadedev/";
static const QString JADE_FW_JADE1_1DEV_PATH = "/bin/jade1.1dev/";
static const QString JADE_FW_SUFFIX = "fw.bin";

static const QString JADE_BOARD_TYPE_JADE = "JADE";
static const QString JADE_BOARD_TYPE_JADE_V1_1 = "JADE_V1.1";
static const QString JADE_FEATURE_SECURE_BOOT = "SB";

} // namespace

JadeHttpRequestActivity::JadeHttpRequestActivity(const QString& path, QObject* parent)
    : HttpRequestActivity(parent)
{
    setMethod("GET");
    addUrl(JADE_FW_SERVER_HTTPS + path);
    addUrl(JADE_FW_SERVER_ONION + path);
}

JadeChannelRequestActivity::JadeChannelRequestActivity(const QString& base, const QString& channel, QObject* parent)
    : JadeHttpRequestActivity(base + channel, parent)
    , m_base(base)
{
    setAccept("text");
}

QVariantList JadeChannelRequestActivity::firmwares() const
{
    QVariantList firmwares;
    for (const auto& line : body().split('\n')) {
        const auto parts = line.split('_');
        if (parts.size() == 4 && parts.last() == JADE_FW_SUFFIX) {
            const auto version = parts[0];
            QVariantMap firmware;
            firmware.insert("path", m_base + line);
            firmware.insert("version", version);
            firmware.insert("config", parts[1]);
            firmware.insert("size", parts[2].toLongLong());
            firmwares.append(firmware);
        }
    }
    return firmwares;
}

JadeBinaryRequestActivity::JadeBinaryRequestActivity(const QString& path, QObject* parent)
    : JadeHttpRequestActivity(path, parent)
{
    setAccept("base64");
}

JadeUnlockActivity::JadeUnlockActivity(JadeDevice* device, QObject* parent)
    : SessionActivity(parent)
    , m_device(device)
{
}

void JadeUnlockActivity::exec()
{
    const auto nets = m_device->versionInfo().value("JADE_NETWORKS").toString();
    m_device->api()->authUser(nets == "TEST" ? "testnet" : "mainnet", [this](const QVariantMap& msg) {
        Q_ASSERT(msg.contains("result"));
        if (msg["result"] == true) {
            finish();
        } else {
            fail();
        }
    }, [=](JadeAPI& jade, int id, const QJsonObject& req) {
        const auto params = Json::fromObject(req.value("params").toObject());
        GA_json* output;
        GA_http_request(session()->m_session, params.get(), &output);
        auto res = Json::toObject(output);
        GA_destroy_json(output);
        jade.handleHttpResponse(id, req, res.value("body").toObject());
    });
}

JadeUpdateActivity::JadeUpdateActivity(const QVariantMap& firmware, const QByteArray& data, JadeDevice* device)
    : Activity(device)
    , m_device(device)
    , m_firmware(firmware)
    , m_data(data)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(m_data);
    m_firmware.insert("hash", hash.result().toHex());
}

void JadeUpdateActivity::exec()
{
    const auto size = m_firmware.value("size").toLongLong();
    auto chunk_size = m_device->versionInfo().value("JADE_OTA_MAX_CHUNK").toInt();
#ifdef Q_OS_MACOS
    if (m_device->systemLocation().contains("cu.usbmodem")) {
        chunk_size = 256;
    }
#endif

    m_device->api()->otaUpdate(m_data, size, chunk_size, [this](const QVariantMap& result) {
        Q_ASSERT(result.contains("uploaded"));
        const auto uploaded = result.value("uploaded").toLongLong();
        progress()->setIndeterminate(uploaded <= 12288);
        progress()->setValue(double(uploaded) / double(m_data.size()));
    }, [this](const QVariantMap& result) {
        if (result["result"] == true) {
            finish();
        } else {
            const auto error = result.value("error").toMap();
            const auto code = error.value("code").toLongLong();
            const auto message = error.value("message").toString();

#define CBOR_RPC_PROTOCOL_ERROR -32001
#define CBOR_RPC_HW_LOCKED -32002
            // message `OTA is only allowed on new or logged-in device.` code is as follow:
            // in 0.1.21 code is CBOR_RPC_PROTOCOL_ERROR
            // in 0.1.23 and later is CBOR_RPC_HW_LOCKED
            if (code == CBOR_RPC_HW_LOCKED || message == "OTA is only allowed on new or logged-in device.") {
                emit locked();
            } else {
                qDebug() << Q_FUNC_INFO << this << "Unexpected error" << code << message;
                fail();
            }
        }
    });
}

JadeUpdateController::JadeUpdateController(QObject *parent)
    : QObject(parent)
{
}

void JadeUpdateController::setChannel(const QString& channel)
{
    if (m_channel == channel) return;
    m_firmwares.clear();
    emit firmwaresChanged(m_firmwares);
    m_channel = channel;
    emit channelChanged(m_channel);
    check();
}

void JadeUpdateController::disconnectDevice()
{
    m_device->api()->disconnectDevice();
}

void JadeUpdateController::setDevice(JadeDevice *device)
{
    if (m_device == device) return;
    m_device = device;
    emit deviceChanged(m_device);
}

void JadeUpdateController::check()
{
    if (!m_device) return;

    const auto version_info = m_device->versionInfo();
    const auto board_type = version_info.value("BOARD_TYPE", JADE_BOARD_TYPE_JADE).toString();
    const auto config = version_info.value("JADE_CONFIG").toString();
    const auto features = version_info.value("JADE_FEATURES").toStringList();
    const bool secure_boot = features.contains(JADE_FEATURE_SECURE_BOOT);

    QString path;
    if (board_type == JADE_BOARD_TYPE_JADE) {
        path = secure_boot ? JADE_FW_JADE_PATH : JADE_FW_JADEDEV_PATH;
    } else if (board_type == JADE_BOARD_TYPE_JADE_V1_1) {
        path = secure_boot ? JADE_FW_JADE1_1_PATH : JADE_FW_JADE1_1DEV_PATH;
    } else {
        return;
    }

    const QString channel = m_channel.isEmpty() ? JADE_FW_VERSIONS_FILE : m_channel;
    const bool latest_channel = channel == JADE_FW_VERSIONS_FILE;
    auto activity = new JadeChannelRequestActivity(path, channel, this);
    connect(activity, &Activity::finished, this, [=] {
        activity->deleteLater();
        m_firmwares.clear();
        for (auto data : activity->firmwares()) {
            QVariantMap firmware = data.toMap();
            const bool same_version = SemVer::parse(m_device->version()) == SemVer::parse(firmware.value("version").toString());
            const bool greater_version = SemVer::parse(m_device->version()) < SemVer::parse(firmware.value("version").toString());
            const bool same_config = config.compare(firmware.value("config").toString(), Qt::CaseInsensitive) == 0;
            const bool installed = same_version && same_config;
            firmware.insert("installed", installed);
            m_firmwares.append(firmware);
            if (latest_channel && same_config && greater_version) {
                m_firmware_available = firmware;
            }
        }
        emit firmwaresChanged(m_firmwares);
        emit firmwareAvailableChanged();
    });
    HttpManager::instance()->exec(activity);
    emit activityCreated(activity);
}

void JadeUpdateController::update(const QVariantMap& firmware)
{
    const auto path = firmware.value("path").toString();
    const auto data = m_firmware_data.value(path);

    if (data.isEmpty()) {
        auto activity = new JadeBinaryRequestActivity(path, this);
        connect(activity, &Activity::failed, this, [activity] {
            activity->deleteLater();
        });
        connect(activity, &Activity::finished, this, [this, firmware, path, activity] {
            activity->deleteLater();
            const auto data = QByteArray::fromBase64(activity->body().toLocal8Bit());
            m_firmware_data.insert(path, data);
            update(firmware);
        });
        emit activityCreated(activity);
        HttpManager::instance()->exec(activity);
    } else {
        auto activity = new JadeUpdateActivity(firmware, data, m_device);
        connect(activity, &JadeUpdateActivity::locked, this, [this, activity] {
            auto unlock_activity = unlock();
            connect(unlock_activity, &Activity::finished, this, [activity, unlock_activity] {
                unlock_activity->deleteLater();
                activity->exec();
            });
            connect(unlock_activity, &Activity::failed, this, [activity, unlock_activity] {
                unlock_activity->deleteLater();
                activity->fail();
            });
        });
        connect(activity, &Activity::failed, this, [activity] {
            activity->deleteLater();
        });
        emit activityCreated(activity);
        ActivityManager::instance()->exec(activity);
    }
}

JadeUnlockActivity* JadeUpdateController::unlock()
{
    auto activity = new JadeUnlockActivity(m_device, this);
    HttpManager::instance()->exec(activity);
    emit activityCreated(activity);
    return activity;
}
