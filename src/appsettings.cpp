#include "appsettings.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

namespace {
QSettings settings() {
    return QSettings();
}
}

namespace AppSettings {

QString searchTemplate() {
    return settings().value(
        QStringLiteral("general/searchTemplate"),
        QStringLiteral("https://duckduckgo.com/?q=%1")).toString();
}

QUrl searchUrl(const QString &query) {
    QString value = searchTemplate();
    if (!value.contains(QStringLiteral("%1"))) {
        value = QStringLiteral("https://duckduckgo.com/?q=%1");
    }
    value.replace(QStringLiteral("%1"),
                  QString::fromLatin1(QUrl::toPercentEncoding(query)));
    return QUrl(value);
}

QString homePage() {
    return settings().value(QStringLiteral("general/homePage"),
                            QStringLiteral("about:blank")).toString();
}

bool askWhereToSaveDownloads() {
    return settings().value(QStringLiteral("downloads/askEveryTime"), true).toBool();
}

QString downloadDirectory() {
    const QString fallback = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return settings().value(QStringLiteral("downloads/directory"), fallback).toString();
}

bool blockThirdPartyState() {
    return settings().value(QStringLiteral("privacy/blockThirdPartyState"), true).toBool();
}

bool allowFederatedIdentityState() {
    return settings().value(QStringLiteral("privacy/allowFederatedIdentityState"), true).toBool();
}

bool sendDoNotTrack() {
    return settings().value(QStringLiteral("privacy/sendDnt"), true).toBool();
}

bool sendGlobalPrivacyControl() {
    return settings().value(QStringLiteral("privacy/sendGpc"), true).toBool();
}

bool spellCheckEnabled() {
    return settings().value(QStringLiteral("privacy/spellCheck"), true).toBool();
}

bool passwordManagerEnabled() {
    return settings().value(QStringLiteral("passwords/enabled"), true).toBool();
}

bool autoFillPasswords() {
    return settings().value(QStringLiteral("passwords/autoFill"), true).toBool();
}

QString proxyMode() {
    return settings().value(QStringLiteral("network/proxyMode"),
                            QStringLiteral("system")).toString();
}

QString proxyEndpoint() {
    return settings().value(QStringLiteral("network/proxyEndpoint")).toString().trimmed();
}

QString proxyBypassList() {
    return settings().value(QStringLiteral("network/proxyBypass")).toString().trimmed();
}

}  // namespace AppSettings
