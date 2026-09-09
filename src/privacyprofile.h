#pragma once

#include <QObject>

class PrivacyInterceptor;
class QWebEngineExtensionInfo;
class QWebEngineExtensionManager;
class QWebEngineProfile;

class PrivacyProfile final : public QObject {
    Q_OBJECT
public:
    explicit PrivacyProfile(bool privateMode, QObject *parent = nullptr);

    QWebEngineProfile *profile() const;
    QWebEngineExtensionManager *extensionManager() const;
    bool isPrivate() const;
    bool extensionsSupported() const;

    void installExtension(const QString &path);

signals:
    void extensionInstalled(const QString &name);
    void extensionInstallFailed(const QString &error);

private:
    void configureProfile();
    void configureExtensions();
    void disableUnwantedBuiltIn(const QWebEngineExtensionInfo &extension);

    QWebEngineProfile *profile_ = nullptr;
    PrivacyInterceptor *interceptor_ = nullptr;
    bool privateMode_ = false;
};
