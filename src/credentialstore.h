#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include <functional>

struct CredentialMeta {
    QString origin;
    QString username;
    QString label;
};

class CredentialStore final : public QObject {
public:
    using ResultCallback = std::function<void(bool ok, const QString &message)>;
    using ReadCallback = std::function<void(bool ok, QString password, const QString &message)>;

    explicit CredentialStore(QObject *parent = nullptr);

    QVector<CredentialMeta> entries() const;
    QVector<CredentialMeta> forUrl(const QUrl &url) const;
    bool secureBackendAvailable() const;

    void saveCredential(const QString &origin,
                        const QString &username,
                        const QString &password,
                        const QString &label = QString(),
                        ResultCallback callback = {});
    void readPassword(const CredentialMeta &credential, ReadCallback callback);
    void removeCredential(const CredentialMeta &credential, ResultCallback callback = {});
    int importCsv(const QString &path, QString *error = nullptr);

    static QString originForUrl(const QUrl &url);

private:
    static QString keyFor(const CredentialMeta &credential);
    static QStringList parseCsvLine(const QString &line);
    QVector<CredentialMeta> loadIndex() const;
    void saveIndex(const QVector<CredentialMeta> &items) const;
};
