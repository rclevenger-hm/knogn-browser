#include "credentialstore.h"

#include <QCryptographicHash>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTextStream>

#include <qtkeychain/keychain.h>

#include <initializer_list>

namespace {
constexpr auto kService = "Knogn Browser";
}

CredentialStore::CredentialStore(QObject *parent)
    : QObject(parent) {}

QString CredentialStore::originForUrl(const QUrl &url) {
    if (!url.isValid() || (url.scheme() != QStringLiteral("http") &&
                           url.scheme() != QStringLiteral("https"))) {
        return {};
    }
    QString origin = url.scheme() + QStringLiteral("://") + url.host();
    if (url.port() > 0) origin += QStringLiteral(":") + QString::number(url.port());
    return origin;
}

QString CredentialStore::keyFor(const CredentialMeta &credential) {
    const QByteArray source = (credential.origin + QChar('|') + credential.username).toUtf8();
    return QStringLiteral("credential/") +
           QString::fromLatin1(QCryptographicHash::hash(source, QCryptographicHash::Sha256).toHex());
}

QVector<CredentialMeta> CredentialStore::loadIndex() const {
    QVector<CredentialMeta> result;
    QSettings settings;
    const QByteArray raw = settings.value(QStringLiteral("passwords/index")).toByteArray();
    if (raw.isEmpty()) return result;
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isArray()) return result;
    for (const QJsonValue &value : doc.array()) {
        const QJsonObject obj = value.toObject();
        CredentialMeta item{
            obj.value(QStringLiteral("origin")).toString(),
            obj.value(QStringLiteral("username")).toString(),
            obj.value(QStringLiteral("label")).toString()
        };
        if (!item.origin.isEmpty() && !item.username.isEmpty()) result.append(item);
    }
    return result;
}

void CredentialStore::saveIndex(const QVector<CredentialMeta> &items) const {
    QJsonArray array;
    for (const auto &item : items) {
        QJsonObject obj;
        obj.insert(QStringLiteral("origin"), item.origin);
        obj.insert(QStringLiteral("username"), item.username);
        obj.insert(QStringLiteral("label"), item.label);
        array.append(obj);
    }
    QSettings settings;
    settings.setValue(QStringLiteral("passwords/index"),
                      QJsonDocument(array).toJson(QJsonDocument::Compact));
    settings.sync();
}

QVector<CredentialMeta> CredentialStore::entries() const {
    return loadIndex();
}

QVector<CredentialMeta> CredentialStore::forUrl(const QUrl &url) const {
    const QString origin = originForUrl(url);
    QVector<CredentialMeta> matches;
    for (const auto &item : loadIndex()) {
        if (item.origin.compare(origin, Qt::CaseInsensitive) == 0) matches.append(item);
    }
    return matches;
}

bool CredentialStore::secureBackendAvailable() const {
    return QKeychain::isAvailable();
}

void CredentialStore::saveCredential(const QString &origin,
                                     const QString &username,
                                     const QString &password,
                                     const QString &label,
                                     ResultCallback callback) {
    CredentialMeta meta{origin.trimmed(), username.trimmed(), label.trimmed()};
    if (meta.origin.isEmpty() || meta.username.isEmpty() || password.isEmpty()) {
        if (callback) callback(false, QStringLiteral("Origin, username and password are required."));
        return;
    }

    QVector<CredentialMeta> index = loadIndex();
    bool inserted = true;
    for (auto &item : index) {
        if (item.origin == meta.origin && item.username == meta.username) {
            item.label = meta.label;
            inserted = false;
            break;
        }
    }
    if (inserted) index.append(meta);
    saveIndex(index);

    auto *job = new QKeychain::WritePasswordJob(QString::fromLatin1(kService), this);
    job->setKey(keyFor(meta));
    job->setTextData(password);
    job->setInsecureFallback(false);
    connect(job, &QKeychain::Job::finished, this,
            [this, meta, inserted, callback](QKeychain::Job *finished) {
        if (finished->error() != QKeychain::NoError) {
            if (inserted) {
                auto items = loadIndex();
                for (int i = items.size() - 1; i >= 0; --i) {
                    if (items[i].origin == meta.origin && items[i].username == meta.username) {
                        items.removeAt(i);
                    }
                }
                saveIndex(items);
            }
            if (callback) callback(false, finished->errorString());
            return;
        }
        if (callback) callback(true, QStringLiteral("Saved securely."));
    });
    job->start();
}

void CredentialStore::readPassword(const CredentialMeta &credential, ReadCallback callback) {
    auto *job = new QKeychain::ReadPasswordJob(QString::fromLatin1(kService), this);
    job->setKey(keyFor(credential));
    job->setInsecureFallback(false);
    connect(job, &QKeychain::Job::finished, this,
            [job, callback](QKeychain::Job *finished) {
        if (!callback) return;
        if (finished->error() != QKeychain::NoError) {
            callback(false, {}, finished->errorString());
            return;
        }
        callback(true, job->textData(), {});
    });
    job->start();
}

void CredentialStore::removeCredential(const CredentialMeta &credential, ResultCallback callback) {
    auto *job = new QKeychain::DeletePasswordJob(QString::fromLatin1(kService), this);
    job->setKey(keyFor(credential));
    job->setInsecureFallback(false);
    connect(job, &QKeychain::Job::finished, this,
            [this, credential, callback](QKeychain::Job *finished) {
        if (finished->error() != QKeychain::NoError &&
            finished->error() != QKeychain::EntryNotFound) {
            if (callback) callback(false, finished->errorString());
            return;
        }
        auto items = loadIndex();
        for (int i = items.size() - 1; i >= 0; --i) {
            if (items[i].origin == credential.origin &&
                items[i].username == credential.username) {
                items.removeAt(i);
            }
        }
        saveIndex(items);
        if (callback) callback(true, QStringLiteral("Removed."));
    });
    job->start();
}

QStringList CredentialStore::parseCsvLine(const QString &line) {
    QStringList fields;
    QString current;
    bool quoted = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (ch == QChar('"')) {
            if (quoted && i + 1 < line.size() && line.at(i + 1) == QChar('"')) {
                current += QChar('"');
                ++i;
            } else {
                quoted = !quoted;
            }
        } else if (ch == QChar(',') && !quoted) {
            fields.append(current);
            current.clear();
        } else {
            current += ch;
        }
    }
    fields.append(current);
    return fields;
}

int CredentialStore::importCsv(const QString &path, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = file.errorString();
        return 0;
    }

    QTextStream stream(&file);
    if (stream.atEnd()) return 0;
    const QStringList headers = parseCsvLine(stream.readLine());
    QHash<QString, int> columns;
    for (int i = 0; i < headers.size(); ++i) {
        columns.insert(headers[i].trimmed().toLower(), i);
    }

    auto column = [&columns](std::initializer_list<const char *> names) -> int {
        for (const char *name : names) {
            const auto it = columns.constFind(QString::fromLatin1(name));
            if (it != columns.cend()) return it.value();
        }
        return -1;
    };

    const int urlColumn = column({"url", "origin", "hostname"});
    const int usernameColumn = column({"username", "user", "login"});
    const int passwordColumn = column({"password", "pass"});
    const int nameColumn = column({"name", "title"});
    if (urlColumn < 0 || usernameColumn < 0 || passwordColumn < 0) {
        if (error) *error = QStringLiteral("CSV must include URL, username and password columns.");
        return 0;
    }

    int imported = 0;
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (line.trimmed().isEmpty()) continue;
        const QStringList fields = parseCsvLine(line);
        const int requiredMax = qMax(urlColumn, qMax(usernameColumn, passwordColumn));
        if (fields.size() <= requiredMax) continue;
        const QUrl url = QUrl::fromUserInput(fields[urlColumn].trimmed());
        const QString origin = originForUrl(url);
        const QString username = fields[usernameColumn].trimmed();
        const QString password = fields[passwordColumn];
        const QString label = nameColumn >= 0 && nameColumn < fields.size()
            ? fields[nameColumn].trimmed() : QString();
        if (origin.isEmpty() || username.isEmpty() || password.isEmpty()) continue;
        saveCredential(origin, username, password, label);
        ++imported;
    }
    return imported;
}
