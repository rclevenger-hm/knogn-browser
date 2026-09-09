#pragma once

#include <QWebEngineUrlRequestInterceptor>

class PrivacyInterceptor final : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT
public:
    explicit PrivacyInterceptor(QObject *parent = nullptr);

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;
};
