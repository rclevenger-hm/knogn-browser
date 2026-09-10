#include "privacyprofile.h"

#include "appsettings.h"
#include "privacyinterceptor.h"

#include <QTimer>
#include <QUrl>
#include <QWebEngineCookieStore>
#include <QWebEngineExtensionInfo>
#include <QWebEngineExtensionManager>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace {

bool isFederatedIdentityOrigin(const QUrl &origin) {
    if (origin.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
        return false;
    }

    const QString host = origin.host().toLower();
    static const QStringList identityHosts = {
        QStringLiteral("accounts.google.com"),
        QStringLiteral("accounts.youtube.com"),
        QStringLiteral("login.microsoftonline.com"),
        QStringLiteral("login.live.com"),
        QStringLiteral("appleid.apple.com"),
        QStringLiteral("idmsa.apple.com"),
        QStringLiteral("github.com"),
        QStringLiteral("www.facebook.com")
    };

    return identityHosts.contains(host);
}

bool isWebFirstParty(const QUrl &url) {
    const QString scheme = url.scheme().toLower();
    return scheme == QStringLiteral("https") || scheme == QStringLiteral("http");
}

}  // namespace

PrivacyProfile::PrivacyProfile(bool privateMode, QObject *parent)
    : QObject(parent), privateMode_(privateMode) {
    if (privateMode_) {
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
    profile_->setSpellCheckEnabled(AppSettings::spellCheckEnabled());

    interceptor_ = new PrivacyInterceptor(this);
    profile_->setUrlRequestInterceptor(interceptor_);

    const bool blockThirdParty = AppSettings::blockThirdPartyState();
    const bool allowFederatedIdentity = AppSettings::allowFederatedIdentityState();
    profile_->cookieStore()->setCookieFilter(
        [blockThirdParty, allowFederatedIdentity](
            const QWebEngineCookieStore::FilterRequest &request) {
            if (!blockThirdParty || !request.thirdParty) return true;

            // Keep third-party tracking state blocked generally, but permit the
            // identity provider's own state when a normal web page explicitly
            // invokes a known federated-login service. Qt documents origin and
            // firstPartyUrl as the intended inputs for narrow third-party
            // cookie/storage allowlists. This exception also covers the related
            // IndexedDB/DOM storage/service-worker gate controlled by the filter.
            if (allowFederatedIdentity &&
                isWebFirstParty(request.firstPartyUrl) &&
                isFederatedIdentityOrigin(request.origin)) {
                return true;
            }

            return false;
        });

    auto *settings = profile_->settings();
    settings->setAttribute(QWebEngineSettings::BackForwardCacheEnabled, true);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
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
