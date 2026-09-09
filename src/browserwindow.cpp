#include "browserwindow.h"

#include "browserpage.h"
#include "privacyprofile.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QUrlQuery>
#include <QWebEngineDownloadRequest>
#include <QWebEnginePage>
#include <QWebEnginePermission>
#include <QWebEngineProfile>
#include <QWebEngineView>

namespace {
constexpr int kLifecycleScanMs = 30'000;
}

BrowserWindow::BrowserWindow(bool privateMode, QWidget *parent)
    : QMainWindow(parent),
      privacyProfile_(new PrivacyProfile(privateMode, this)) {
    setupUi();
    setupDownloads();
    setupExtensions();
    setupLifecycleController();
    addTab(QUrl(QStringLiteral("about:blank")));
    resize(1280, 820);
    updateWindowTitle();
    statusBar()->showMessage(privateMode
        ? QStringLiteral("Private window: browsing state remains memory-only")
        : QStringLiteral("Knogn privacy defaults active: third-party state blocked"));
}

void BrowserWindow::setupUi() {
    tabs_ = new QTabWidget(this);
    tabs_->setDocumentMode(true);
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    tabs_->setElideMode(Qt::ElideRight);
    setCentralWidget(tabs_);

    auto *toolbar = addToolBar(QStringLiteral("Navigation"));
    toolbar->setMovable(false);

    auto *back = toolbar->addAction(QStringLiteral("←"));
    auto *forward = toolbar->addAction(QStringLiteral("→"));
    auto *reload = toolbar->addAction(QStringLiteral("↻"));
    auto *home = toolbar->addAction(QStringLiteral("⌂"));
    toolbar->addSeparator();

    address_ = new QLineEdit(this);
    address_->setClearButtonEnabled(true);
    address_->setPlaceholderText(QStringLiteral("Search or enter address"));
    toolbar->addWidget(address_);

    auto *newTab = toolbar->addAction(QStringLiteral("+"));

    progress_ = new QProgressBar(this);
    progress_->setMaximumWidth(110);
    progress_->setRange(0, 100);
    progress_->hide();
    toolbar->addWidget(progress_);

    auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    auto *newWindow = fileMenu->addAction(QStringLiteral("New Window"));
    auto *newPrivate = fileMenu->addAction(QStringLiteral("New Private Window"));
    fileMenu->addSeparator();
    auto *quit = fileMenu->addAction(QStringLiteral("Quit"));

    auto *toolsMenu = menuBar()->addMenu(QStringLiteral("&Tools"));
    installExtensionAction_ = toolsMenu->addAction(QStringLiteral("Install Chrome Extension…"));
    auto *optimizeTabs = toolsMenu->addAction(QStringLiteral("Optimize Background Tabs"));

    auto *privacyMenu = menuBar()->addMenu(QStringLiteral("&Privacy"));
    auto *privacyInfo = privacyMenu->addAction(QStringLiteral("Privacy Status"));

    connect(back, &QAction::triggered, this, [this] {
        if (auto *view = currentView()) view->back();
    });
    connect(forward, &QAction::triggered, this, [this] {
        if (auto *view = currentView()) view->forward();
    });
    connect(reload, &QAction::triggered, this, [this] {
        if (auto *view = currentView()) view->reload();
    });
    connect(home, &QAction::triggered, this, [this] {
        if (auto *view = currentView()) view->setUrl(QUrl(QStringLiteral("about:blank")));
    });
    connect(newTab, &QAction::triggered, this, [this] { addTab(); });
    connect(address_, &QLineEdit::returnPressed, this, &BrowserWindow::openLocation);
    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        auto *widget = tabs_->widget(index);
        tabs_->removeTab(index);
        widget->deleteLater();
        if (tabs_->count() == 0) addTab();
    });
    connect(tabs_, &QTabWidget::currentChanged, this, &BrowserWindow::syncCurrentTab);

    connect(newWindow, &QAction::triggered, this, [] {
        auto *window = new BrowserWindow(false);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
    });
    connect(newPrivate, &QAction::triggered, this, [] {
        auto *window = new BrowserWindow(true);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
    });
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);
    connect(installExtensionAction_, &QAction::triggered,
            this, &BrowserWindow::installExtension);
    connect(optimizeTabs, &QAction::triggered,
            this, &BrowserWindow::optimizeBackgroundTabs);
    connect(privacyInfo, &QAction::triggered, this, [this] {
        const QString profileType = privacyProfile_->isPrivate()
            ? QStringLiteral("memory-only private profile")
            : QStringLiteral("persistent local profile");
        QMessageBox::information(
            this,
            QStringLiteral("Knogn Privacy Status"),
            QStringLiteral(
                "Profile: %1\n"
                "Third-party cookies/state: blocked\n"
                "Push service: disabled\n"
                "DNS prefetch: disabled\n"
                "WebRTC: public interfaces only\n"
                "Telemetry endpoint: none\n"
                "Extensions in this window: %2")
                .arg(profileType,
                     privacyProfile_->extensionsSupported()
                         ? QStringLiteral("available (Manifest V3)")
                         : QStringLiteral("disabled")));
    });

    auto *focusLocation = new QShortcut(QKeySequence(QStringLiteral("Ctrl+L")), this);
    connect(focusLocation, &QShortcut::activated, address_, [this] {
        address_->setFocus();
        address_->selectAll();
    });
    auto *shortcutNewTab = new QShortcut(QKeySequence(QStringLiteral("Ctrl+T")), this);
    connect(shortcutNewTab, &QShortcut::activated, this, [this] { addTab(); });
    auto *shortcutCloseTab = new QShortcut(QKeySequence(QStringLiteral("Ctrl+W")), this);
    connect(shortcutCloseTab, &QShortcut::activated, this, &BrowserWindow::closeCurrentTab);
}

void BrowserWindow::setupDownloads() {
    connect(privacyProfile_->profile(), &QWebEngineProfile::downloadRequested,
            this, [this](QWebEngineDownloadRequest *download) {
        const QString suggested = download->suggestedFileName();
        const QString destination = QFileDialog::getSaveFileName(
            this, QStringLiteral("Save download"), suggested);
        if (destination.isEmpty()) {
            download->cancel();
            return;
        }

        const QFileInfo file(destination);
        download->setDownloadDirectory(file.absolutePath());
        download->setDownloadFileName(file.fileName());
        download->accept();
    });
}

void BrowserWindow::setupExtensions() {
    installExtensionAction_->setEnabled(privacyProfile_->extensionsSupported());
    if (!privacyProfile_->extensionsSupported()) {
        installExtensionAction_->setToolTip(
            QStringLiteral("Qt WebEngine does not load user extensions into off-the-record profiles."));
    }

    connect(privacyProfile_, &PrivacyProfile::extensionInstalled,
            this, [this](const QString &name) {
        statusBar()->showMessage(
            QStringLiteral("Extension installed and enabled: %1").arg(name), 6000);
    });
    connect(privacyProfile_, &PrivacyProfile::extensionInstallFailed,
            this, [this](const QString &error) {
        QMessageBox::warning(this, QStringLiteral("Extension install failed"), error);
    });
}

void BrowserWindow::setupLifecycleController() {
    lifecycleTimer_ = new QTimer(this);
    lifecycleTimer_->setInterval(kLifecycleScanMs);
    connect(lifecycleTimer_, &QTimer::timeout,
            this, &BrowserWindow::optimizeBackgroundTabs);
    lifecycleTimer_->start();
}

QWebEngineView *BrowserWindow::addTab(const QUrl &url, bool makeCurrent) {
    auto *view = new QWebEngineView(tabs_);
    auto *page = new BrowserPage(
        privacyProfile_->profile(),
        [this] { return createPageForPopup(); },
        view);
    view->setPage(page);
    wireView(view);

    const int index = tabs_->addTab(view, QStringLiteral("New Tab"));
    if (makeCurrent) tabs_->setCurrentIndex(index);
    view->setUrl(url);
    return view;
}

QWebEnginePage *BrowserWindow::createPageForPopup() {
    return addTab(QUrl(QStringLiteral("about:blank")), true)->page();
}

void BrowserWindow::wireView(QWebEngineView *view) {
    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &title) {
        const int index = tabs_->indexOf(view);
        if (index >= 0) {
            tabs_->setTabText(index, title.isEmpty() ? QStringLiteral("New Tab") : title.left(48));
        }
        if (view == currentView()) updateWindowTitle();
    });
    connect(view, &QWebEngineView::urlChanged, this, [this, view](const QUrl &url) {
        if (view == currentView()) address_->setText(url.toDisplayString());
    });
    connect(view, &QWebEngineView::loadProgress, this, [this, view](int value) {
        if (view != currentView()) return;
        progress_->setValue(value);
        progress_->setVisible(value > 0 && value < 100);
    });
    connect(view->page(), &QWebEnginePage::permissionRequested,
            this, [this](QWebEnginePermission permission) {
        const QString host = permission.origin().host();
        const auto answer = QMessageBox::question(
            this,
            QStringLiteral("Site permission"),
            QStringLiteral("Allow %1 to use the requested capability for this session?").arg(host));
        if (answer == QMessageBox::Yes) permission.grant();
        else permission.deny();
    });
}

QWebEngineView *BrowserWindow::currentView() const {
    return qobject_cast<QWebEngineView *>(tabs_->currentWidget());
}

void BrowserWindow::openLocation() {
    if (auto *view = currentView()) view->setUrl(resolveInput(address_->text()));
}

void BrowserWindow::closeCurrentTab() {
    const int index = tabs_->currentIndex();
    if (index < 0) return;
    auto *widget = tabs_->widget(index);
    tabs_->removeTab(index);
    widget->deleteLater();
    if (tabs_->count() == 0) addTab();
}

void BrowserWindow::syncCurrentTab(int) {
    for (int i = 0; i < tabs_->count(); ++i) {
        auto *view = qobject_cast<QWebEngineView *>(tabs_->widget(i));
        if (!view) continue;
        if (i == tabs_->currentIndex()) {
            if (view->page()->lifecycleState() != QWebEnginePage::LifecycleState::Active) {
                view->page()->setLifecycleState(QWebEnginePage::LifecycleState::Active);
            }
        }
    }

    if (auto *view = currentView()) {
        address_->setText(view->url().toDisplayString());
    }
    updateWindowTitle();
}

void BrowserWindow::installExtension() {
    if (!privacyProfile_->extensionsSupported()) return;

    QMessageBox chooser(this);
    chooser.setWindowTitle(QStringLiteral("Install Chrome Extension"));
    chooser.setText(QStringLiteral(
        "Knogn currently supports Chrome/Chromium Manifest V3 extensions through Qt WebEngine.\n\n"
        "Choose a ZIP package, or cancel this dialog to choose an unpacked extension directory."));
    auto *zipButton = chooser.addButton(QStringLiteral("Choose ZIP"), QMessageBox::AcceptRole);
    auto *directoryButton = chooser.addButton(QStringLiteral("Choose Directory"), QMessageBox::ActionRole);
    chooser.addButton(QMessageBox::Cancel);
    chooser.exec();

    QString path;
    if (chooser.clickedButton() == zipButton) {
        path = QFileDialog::getOpenFileName(
            this,
            QStringLiteral("Install extension"),
            QString(),
            QStringLiteral("Chrome extension packages (*.zip);;All files (*)"));
    } else if (chooser.clickedButton() == directoryButton) {
        path = QFileDialog::getExistingDirectory(
            this, QStringLiteral("Install unpacked extension"));
    }

    if (!path.isEmpty()) privacyProfile_->installExtension(path);
}

void BrowserWindow::optimizeBackgroundTabs() {
    int optimized = 0;
    for (int i = 0; i < tabs_->count(); ++i) {
        if (i == tabs_->currentIndex()) continue;
        auto *view = qobject_cast<QWebEngineView *>(tabs_->widget(i));
        if (!view) continue;

        const auto recommended = view->page()->recommendedState();
        const auto current = view->page()->lifecycleState();
        if (recommended != QWebEnginePage::LifecycleState::Active && current != recommended) {
            view->page()->setLifecycleState(recommended);
            ++optimized;
        }
    }

    if (optimized > 0) {
        statusBar()->showMessage(
            QStringLiteral("Optimized %1 background tab(s)").arg(optimized), 3000);
    }
}

void BrowserWindow::updateWindowTitle() {
    QString title = QStringLiteral("Knogn");
    if (auto *view = currentView(); view && !view->title().isEmpty()) {
        title = view->title() + QStringLiteral(" — Knogn");
    }
    if (privacyProfile_->isPrivate()) title += QStringLiteral(" — Private");
    setWindowTitle(title);
}

QUrl BrowserWindow::resolveInput(const QString &rawInput) const {
    const QString input = rawInput.trimmed();
    if (input.isEmpty()) return QUrl(QStringLiteral("about:blank"));

    const bool looksLikeSearch = input.contains(' ') ||
        (!input.contains('.') && !input.contains(QLatin1String("://")) &&
         input != QLatin1String("localhost"));

    if (looksLikeSearch) {
        QUrl search(QStringLiteral("https://duckduckgo.com/"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("q"), input);
        search.setQuery(query);
        return search;
    }

    return QUrl::fromUserInput(input);
}
