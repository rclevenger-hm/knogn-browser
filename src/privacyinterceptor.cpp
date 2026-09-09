#include "privacyinterceptor.h"

#include <QWebEngineUrlRequestInfo>

PrivacyInterceptor::PrivacyInterceptor(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent) {}

void PrivacyInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info) {
    // These are preference signals, not anonymity guarantees. They are added to
    // ordinary HTTP(S) requests without redirecting or proxying user traffic.
    const QByteArray scheme = info.requestUrl().scheme().toLatin1().toLower();
    if (scheme == "http" || scheme == "https") {
        info.setHttpHeader(QByteArrayLiteral("DNT"), QByteArrayLiteral("1"));
        info.setHttpHeader(QByteArrayLiteral("Sec-GPC"), QByteArrayLiteral("1"));
    }
}
