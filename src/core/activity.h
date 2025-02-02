#ifndef GREEN_ACTIVITY_H
#define GREEN_ACTIVITY_H

#include <QtQml>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(ActivityManager)

class Progress : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal from READ from NOTIFY fromChanged)
    Q_PROPERTY(qreal to READ to NOTIFY toChanged)
    Q_PROPERTY(qreal value READ value NOTIFY valueChanged)
    Q_PROPERTY(bool indeterminate READ indeterminate NOTIFY indeterminateChanged)
    QML_ANONYMOUS
public:
    Progress(QObject* parent = nullptr);
    qreal from() const { return m_from; }
    void setFrom(qreal from);
    qreal to() const { return m_to; }
    void setTo(qreal to);
    qreal value() const { return m_value; }
    void setValue(qreal value);
    void incrementValue(int inc = 1);
    bool indeterminate() const { return m_indeterminate; }
    void setIndeterminate(bool indeterminate);
signals:
    void fromChanged(qreal from);
    void toChanged(qreal to);
    void valueChanged(qreal value);
    void indeterminateChanged(bool indeterminate);
private:
    qreal m_from{0};
    qreal m_to{1};
    qreal m_value{0};
    bool m_indeterminate{true};
};

class Activity : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Progress* progress READ progress CONSTANT)
    Q_PROPERTY(QJsonObject message READ message NOTIFY messageChanged)
    QML_ELEMENT
public:
    // TODO: maybe split Pending to Ready and Active
    enum class Status {
        Pending,
        Finished,
        Failed,
    };
    Q_ENUM(Status)
    Activity(QObject* parent = nullptr);
    QString type() const;
    Status status() const;
    Progress* progress() { return &m_progress; }
    QJsonObject message() const { return m_message; }
    void setMessage(const QJsonObject& message);
    void finish();
    void fail();
private:
    virtual void exec() = 0;
signals:
    void statusChanged(Status status);
    void finished();
    void failed();
    void messageChanged(const QJsonObject& message);
private:
    Status m_status{Status::Pending};
    Progress m_progress;
    QJsonObject m_message;

    friend class ActivityManager;
};

#endif // GREEN_ACTIVITY_H
