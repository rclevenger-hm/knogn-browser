#include "privacyprofile.h"

#include "privacyinterceptor.h"

#include <QTimer>
#include <QWebEngineCookieStore>
#include <QWebEngineExtensionInfo>
#include <QWebEngineExtensionManager>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

PrivacyProfile::PrivacyProfile(bool privateMode, QObject *parent)
    : QObject(parent), privateMode_(privateMode) {
    if (privateMode_) {
        // A profile without a storage name is off-the-record. Qt keeps cookies,
        // HTTP cache and normally persistent web data in memory for this profile.
        profile_ = new QWebEngineProfile(this);
    } else {
        profile_ = new QWebEngineProfile(QStringLiteral("knogn-default"), this);
        profile_->setPersistentCookiesPolicy(QWebEngineProfile::OnlyPersistentCookies);
        profile_->setPersistentPermissionsPolicy(
            QWebEngineProfile::PersistentPermissionsPolicy::StoreInMemory);
    }

    configureProfile();
    configureExtensions();
}

void PrivacyProfile::configureProfile() {
    profile_->setPushServiceEnabled(false);
    profile_->setSpellCheckEnabled(true);

    interceptor_ = new PrivacyInterceptor(this);
    profile_->setUrlRequestInterceptor(interceptor_);

    // Qt documents that this filter gates cookies and other stateful tracking
    // surfaces including IndexedDB, DOM storage, filesystem APIs and service workers.
    profile_->cookieStore()->setCookieFilter(
        [](const QWebEngineCookieStore::FilterRequest &request) {
            return !request.thirdParty;
        });

    auto *settings = profile_->settings();
    settings->setAttribute(QWebEngineSettings::BackForwardCacheEnabled, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, false);
    settings->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, false);
    settings->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, false);
    settings->setAttribute(QWebEngineSettings::JavascriptCanPaste, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    settings->setUnknownUrlSchemePolicy(
        QWebEngineSettings::AllowUnknownUrlSchemesFromUserInteraction);
}

void PrivacyProfile::configureExtensions() {
    auto *manager = profile_->extensionManager();
    if (!manager) return;

    connect(manager, &QWebEngineExtensionManager::loadFinished,
            this, [this](const QWebEngineExtensionInfo &extension) {
        disableUnwantedBuiltIn(extension);
    });

    connect(manager, &QWebEngineExtensionManager::installFinished,
            this, [this, manager](const QWebEngineExtensionInfo &extension) {
        if (!extension.error().isEmpty() || !extension.isInstalled()) {
            emit extensionInstallFailed(
                extension.error().isEmpty()
                    ? QStringLiteral("The extension could not be installed.")
                    : extension.error());
            return;
        }

        manager->setExtensionEnabled(extension, true);
        emit extensionInstalled(extension.name());
    });

    // The built-in Hangouts extension is not required for Knogn and represents
    // functionality we do not want silently active in a privacy-first browser.
    QTimer::singleShot(0, this, [this, manager] {
        for (const auto &extension : manager->extensions()) {
            disableUnwantedBuiltIn(extension);
        }
    });
}

void PrivacyProfile::disableUnwantedBuiltIn(const QWebEngineExtensionInfo &extension) {
    if (!profile_ || !profile_->extensionManager()) return;
    if (extension.name().contains(QStringLiteral("Hangouts"), Qt::CaseInsensitive)) {
        profile_->extensionManager()->setExtensionEnabled(extension, false);
    }
}

QWebEngineProfile *PrivacyProfile::profile() const {
    return profile_;
}

QWebEngineExtensionManager *PrivacyProfile::extensionManager() const {
    return profile_ ? profile_->extensionManager() : nullptr;
}

bool PrivacyProfile::isPrivate() const {
    return privateMode_;
}

bool PrivacyProfile::extensionsSupported() const {
    // Qt WebEngine does not load user extensions into off-the-record profiles.
    return !privateMode_ && extensionManager();
}

void PrivacyProfile::installExtension(const QString &path) {
    if (!extensionsSupported()) {
        emit extensionInstallFailed(
            QStringLiteral("Extensions are disabled in Knogn private windows."));
        return;
    }
    extensionManager()->installExtension(path);
}
