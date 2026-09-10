#pragma once

#include <QString>
#include <QUrl>

namespace AppSettings {

QString searchTemplate();
QUrl searchUrl(const QString &query);
QString homePage();
bool askWhereToSaveDownloads();
QString downloadDirectory();

bool blockThirdPartyState();
bool allowFederatedIdentityState();
bool sendDoNotTrack();
bool sendGlobalPrivacyControl();
bool spellCheckEnabled();
bool passwordManagerEnabled();
bool autoFillPasswords();

QString proxyMode();
QString proxyEndpoint();
QString proxyBypassList();

}  // namespace AppSettings
