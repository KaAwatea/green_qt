#ifndef GREEN_RECEIVEADDRESSCONTROLLER_H
#define GREEN_RECEIVEADDRESSCONTROLLER_H

#include <QtQml>
#include <QObject>
#include <QJsonObject>

QT_FORWARD_DECLARE_CLASS(Account)

class ReceiveAddressController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Account* account READ account WRITE setAccount NOTIFY accountChanged)
    Q_PROPERTY(QString amount READ amount WRITE setAmount NOTIFY changed)
    Q_PROPERTY(QString address READ address NOTIFY changed)
    Q_PROPERTY(QJsonObject result READ result NOTIFY changed)
    Q_PROPERTY(QString uri READ uri NOTIFY changed)
    Q_PROPERTY(bool generating READ generating NOTIFY generatingChanged)
    Q_PROPERTY(AddressVerification addressVerification READ addressVerification NOTIFY addressVerificationChanged)
    QML_ELEMENT
public:
    enum AddressVerification {
        VerificationNone,
        VerificationPending,
        VerificationAccepted,
        VerificationRejected,
    };
    Q_ENUM(AddressVerification)

    explicit ReceiveAddressController(QObject* parent = nullptr);
    virtual ~ReceiveAddressController();
    Account* account() const;
    void setAccount(Account* account);
    QString amount() const;
    void setAmount(const QString& amount);
    QString address() const;
    QJsonObject result() const { return m_result; }
    QString uri() const;
    bool generating() const;
    void setGenerating(bool generating);
    AddressVerification addressVerification() const { return m_address_verification; }
    void setAddressVerification(AddressVerification address_verification);
public slots:
    void generate();
    void verify();
signals:
    void accountChanged(Account* account);
    void changed();
    void generatingChanged(bool generating);
    void addressVerificationChanged(AddressVerification address_verification);
private:
    void verifyMultisig();
    void verifySinglesig();
public:
    Account* m_account{nullptr};
    QString m_amount;
    QString m_address;
    QJsonObject m_result;
    bool m_generating{false};
    AddressVerification m_address_verification{VerificationNone};
};

#endif // GREEN_RECEIVEADDRESSCONTROLLER_H
