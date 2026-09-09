#include "privacyinterceptor.h"

#include "appsettings.h"

#include <QWebEngineUrlRequestInfo>

PrivacyInterceptor::PrivacyInterceptor(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent),
      sendDnt_(AppSettings::sendDoNotTrack()),
      sendGpc_(AppSettings::sendGlobalPrivacyControl()) {}

void PrivacyInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info) {
    const QByteArray scheme = info.requestUrl().scheme().toLatin1().toLower();
    if (scheme != "http" && scheme != "https") return;
    if (sendDnt_) {
        info.setHttpHeader(QByteArrayLiteral("DNT"), QByteArrayLiteral("1"));
    }
    if (sendGpc_) {
        info.setHttpHeader(QByteArrayLiteral("Sec-GPC"), QByteArrayLiteral("1"));
    }
}
