#ifndef GREEN_SESSION_H
#define GREEN_SESSION_H

#include "activity.h"

#include <QtQml>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(Network);

QT_FORWARD_DECLARE_STRUCT(GA_session)

class Session : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Network* network READ network WRITE setNetwork NOTIFY networkChanged)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool connecting READ isConnecting NOTIFY isConnectingChanged)
    QML_ELEMENT
public:
    Session(QObject* parent = nullptr);
    virtual ~Session();
    Network* network() const { return m_network; }
    void setNetwork(Network* network);
    bool isActive() const { return m_active; }
    void setActive(bool active);
    bool isConnected() const { return m_connected; }
    bool isConnecting() const { return m_connecting; }
signals:
    void networkChanged(Network* network);
    void notificationHandled(const QJsonObject& notification);
    void networkEvent(const QJsonObject& event);
    void sessionEvent(const QJsonObject& event);
    void activeChanged(bool active);
    void connectedChanged(bool connected);
    void isConnectingChanged(bool connecting);
    void torEvent(const QJsonObject& event);
    void activityCreated(Activity* activity);
private:
    void update();
    void handleNotification(const QJsonObject& notification);
    void setConnected(bool connected);
public:
    Network* m_network{nullptr};
    bool m_active{false};
    QThread* m_thread{nullptr};
    QObject* m_context{nullptr};
    // TODO: make m_session private
    GA_session* m_session{nullptr};
    bool m_connected{false};
    bool m_connecting{false};
};

class SessionActivity : public Activity
{
    Q_OBJECT
    Q_PROPERTY(Session* session READ session CONSTANT)
    QML_ELEMENT
public:
    SessionActivity(Session* session);
    Session* session() const { return m_session; }
private:
    Session* const m_session;
};

class SessionTorCircuitActivity : public SessionActivity
{
    Q_OBJECT
    Q_PROPERTY(QStringList logs READ logs NOTIFY logsChanged)
    QML_ELEMENT
public:
    SessionTorCircuitActivity(Session* session);
    QStringList logs() const { return m_logs; }
    void exec() {}
signals:
    void logsChanged(const QStringList& logs);
private:
    QMetaObject::Connection m_tor_event_connection;
    QStringList m_logs;
};

class SessionConnectActivity : public SessionActivity
{
    Q_OBJECT
    QML_ELEMENT
public:
    SessionConnectActivity(Session* session);
    void exec() {}
private:
    QMetaObject::Connection m_network_event_connection;
    QMetaObject::Connection m_session_event_connection;
};

#endif // GREEN_SESSION_H
