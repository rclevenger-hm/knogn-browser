#include "browserpage.h"

#include "branding.h"
#include "identitydetails.h"
#include "mediadetails.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QTabBar>
#include <QTimer>
#include <QToolBar>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QWidget>

#include <utility>

namespace {
void setBrowserChromeVisible(QWidget *window, bool visible) {
    if (!window) return;

    for (auto *menu : window->findChildren<QMenuBar *>()) menu->setVisible(visible);
    for (auto *toolbar : window->findChildren<QToolBar *>()) toolbar->setVisible(visible);
    for (auto *status : window->findChildren<QStatusBar *>()) status->setVisible(visible);
    for (auto *tabs : window->findChildren<QTabBar *>()) tabs->setVisible(visible);
}
}

BrowserPage::BrowserPage(QWebEngineProfile *profile,
                         NewPageFactory newPageFactory,
                         QObject *parent)
    : QWebEnginePage(profile, parent),
      newPageFactory_(std::move(newPageFactory)) {
    settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);

    connect(this, &QWebEnginePage::fullScreenRequested,
            this, [this](QWebEngineFullScreenRequest request) {
        const bool enable = request.toggleOn();
        auto *view = qobject_cast<QWebEngineView *>(this->parent());
        if (!view || !view->window()) {
            request.reject();
            return;
        }

        request.accept();
        setFullScreenMode(enable);
    });
}

QWebEnginePage *BrowserPage::createWindow(WebWindowType) {
    return newPageFactory_ ? newPageFactory_() : nullptr;
}

bool BrowserPage::acceptNavigationRequest(const QUrl &url,
                                          NavigationType type,
                                          bool isMainFrame) {
    if (isMainFrame && url.scheme() == QStringLiteral("knogn") &&
        url.host() == QStringLiteral("media")) {
        QTimer::singleShot(0, this, [this] {
            setHtml(mediaDiagnosticsHtml(), QUrl(QStringLiteral("knogn://media")));
        });
        return false;
    }

    if (isMainFrame && url.scheme() == QStringLiteral("knogn") &&
        url.host() == QStringLiteral("identity")) {
        QTimer::singleShot(0, this, [this] {
            setHtml(identityDiagnosticsHtml(), QUrl(QStringLiteral("knogn://identity")));
        });
        return false;
    }

    if (isMainFrame && url == QUrl(QStringLiteral("about:blank")) &&
        !newTabBrandingShown_) {
        newTabBrandingShown_ = true;
        QTimer::singleShot(0, this, [this] {
            setHtml(Branding::newTabHtml(), QUrl(QStringLiteral("about:blank")));
        });
        return false;
    }

    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

void BrowserPage::setFullScreenMode(bool enabled) {
    auto *view = qobject_cast<QWebEngineView *>(parent());
    QWidget *window = view ? view->window() : nullptr;
    if (!window) return;

    if (enabled) {
        if (fullScreenActive_) return;
        fullScreenActive_ = true;
        previousWindowState_ = window->windowState();
        setBrowserChromeVisible(window, false);
        window->showFullScreen();
        return;
    }

    if (!fullScreenActive_) return;
    fullScreenActive_ = false;
    window->showNormal();
    window->setWindowState(previousWindowState_);
    setBrowserChromeVisible(window, true);
}
