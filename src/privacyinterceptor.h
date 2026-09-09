#pragma once

#include <QWebEngineUrlRequestInterceptor>

class PrivacyInterceptor final : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT
public:
    explicit PrivacyInterceptor(QObject *parent = nullptr);

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

private:
    bool sendDnt_ = true;
    bool sendGpc_ = true;
};
