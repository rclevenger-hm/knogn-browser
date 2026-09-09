#include "browserwindow.h"

#include "appsettings.h"
#include "browserpage.h"
#include "privacyprofile.h"
#include "settingsdialog.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QWebEngineDownloadRequest>
#include <QWebEnginePage>
#include <QWebEnginePermission>
#include <QWebEngineProfile>
#include <QWebEngineView>

namespace {
constexpr int kLifecycleScanMs = 30'000;

QString javascriptString(const QString &value) {
    QJsonArray array;
    array.append(value);
    QByteArray json = QJsonDocument(array).toJson(QJsonDocument::Compact);
    if (json.size() >= 2) json = json.mid(1, json.size() - 2);
    return QString::fromUtf8(json);
}

QString credentialFillScript(const QString &username, const QString &password) {
    return QStringLiteral(R"JS(
(() => {
  const username = %1;
  const password = %2;
  const passwordField = document.querySelector('input[type="password"]');
  if (!passwordField) return false;
  const form = passwordField.form || document;
  const candidates = [...form.querySelectorAll('input')].filter((el) => {
    const type = (el.type || '').toLowerCase();
    const autocomplete = (el.autocomplete || '').toLowerCase();
    return el !== passwordField &&
      (type === 'email' || type === 'text' || type === '' ||
       autocomplete.includes('username') || autocomplete.includes('email'));
  });
  const userField = candidates.find(el => el.offsetParent !== null) || candidates[0];
  const setValue = (el, value) => {
    const descriptor = Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, 'value');
    if (descriptor && descriptor.set) descriptor.set.call(el, value);
    else el.value = value;
    el.dispatchEvent(new Event('input', {bubbles: true}));
    el.dispatchEvent(new Event('change', {bubbles: true}));
  };
  if (userField) setValue(userField, username);
  setValue(passwordField, password);
  return true;
})()
)JS").arg(javascriptString(username), javascriptString(password));
}
}

BrowserWindow::BrowserWindow(bool privateMode, QWidget *parent)
    : QMainWindow(parent),
      privacyProfile_(new PrivacyProfile(privateMode, this)),
      credentials_(this) {
    setupUi();
    setupDownloads();
    setupExtensions();
    setupLifecycleController();
    rebuildBookmarksUi();
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

    bookmarkAction_ = toolbar->addAction(QStringLiteral("☆"));
    bookmarkAction_->setToolTip(QStringLiteral("Bookmark this page (Ctrl+D)"));
    auto *newTab = toolbar->addAction(QStringLiteral("+"));

    progress_ = new QProgressBar(this);
    progress_->setMaximumWidth(110);
    progress_->setRange(0, 100);
    progress_->hide();
    toolbar->addWidget(progress_);

    bookmarksToolbar_ = addToolBar(QStringLiteral("Bookmarks"));
    bookmarksToolbar_->setMovable(false);
    bookmarksToolbar_->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    auto *newWindow = fileMenu->addAction(QStringLiteral("New Window"));
    auto *newPrivate = fileMenu->addAction(QStringLiteral("New Private Window"));
    fileMenu->addSeparator();
    auto *settingsAction = fileMenu->addAction(QStringLiteral("Settings…"));
    fileMenu->addSeparator();
    auto *quit = fileMenu->addAction(QStringLiteral("Quit"));

    bookmarksMenu_ = menuBar()->addMenu(QStringLiteral("&Bookmarks"));

    auto *passwordMenu = menuBar()->addMenu(QStringLiteral("&Passwords"));
    auto *saveLogin = passwordMenu->addAction(QStringLiteral("Save login for this site…"));
    auto *fillLogin = passwordMenu->addAction(QStringLiteral("Fill saved login"));
    passwordMenu->addSeparator();
    auto *manageLogins = passwordMenu->addAction(QStringLiteral("Manage saved logins…"));
    auto *importLogins = passwordMenu->addAction(QStringLiteral("Import passwords from CSV…"));

    auto *toolsMenu = menuBar()->addMenu(QStringLiteral("&Tools"));
    installExtensionAction_ = toolsMenu->addAction(QStringLiteral("Install Chrome Extension…"));
    auto *optimizeTabs = toolsMenu->addAction(QStringLiteral("Optimize Background Tabs"));
    auto *mediaInfo = toolsMenu->addAction(QStringLiteral("Media Compatibility"));
    toolsMenu->addSeparator();
    auto *toolsSettings = toolsMenu->addAction(QStringLiteral("Settings…"));

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
        if (auto *view = currentView()) {
            const QString configured = AppSettings::homePage();
            view->setUrl(configured == QStringLiteral("about:blank")
                ? QUrl(QStringLiteral("about:blank"))
                : QUrl::fromUserInput(configured));
        }
    });
    connect(newTab, &QAction::triggered, this, [this] { addTab(); });
    connect(bookmarkAction_, &QAction::triggered, this, &BrowserWindow::bookmarkCurrentPage);
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
    connect(settingsAction, &QAction::triggered, this, &BrowserWindow::showSettings);
    connect(toolsSettings, &QAction::triggered, this, &BrowserWindow::showSettings);
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);
    connect(installExtensionAction_, &QAction::triggered,
            this, &BrowserWindow::installExtension);
    connect(optimizeTabs, &QAction::triggered,
            this, &BrowserWindow::optimizeBackgroundTabs);
    connect(mediaInfo, &QAction::triggered, this, [this] {
        if (auto *view = currentView()) view->setUrl(QUrl(QStringLiteral("knogn://media")));
    });
    connect(saveLogin, &QAction::triggered, this, &BrowserWindow::saveCurrentLogin);
    connect(fillLogin, &QAction::triggered, this, [this] { fillSavedLogin(false); });
    connect(manageLogins, &QAction::triggered, this, &BrowserWindow::manageCredentials);
    connect(importLogins, &QAction::triggered, this, &BrowserWindow::importPasswords);

    connect(privacyInfo, &QAction::triggered, this, [this] {
        const QString profileType = privacyProfile_->isPrivate()
            ? QStringLiteral("memory-only private profile")
            : QStringLiteral("persistent local profile");
        QMessageBox::information(
            this,
            QStringLiteral("Knogn Privacy Status"),
            QStringLiteral(
                "Profile: %1\n"
                "Third-party cookies/state: %2\n"
                "Push service: disabled\n"
                "DNS prefetch: disabled\n"
                "WebRTC: public interfaces only\n"
                "Telemetry endpoint: none\n"
                "Secure credential backend: %3\n"
                "Extensions in this window: %4")
                .arg(profileType,
                     AppSettings::blockThirdPartyState() ? QStringLiteral("blocked")
                                                         : QStringLiteral("allowed"),
                     credentials_.secureBackendAvailable() ? QStringLiteral("available")
                                                           : QStringLiteral("unavailable"),
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
    auto *shortcutBookmark = new QShortcut(QKeySequence(QStringLiteral("Ctrl+D")), this);
    connect(shortcutBookmark, &QShortcut::activated, this, &BrowserWindow::bookmarkCurrentPage);
}

void BrowserWindow::setupDownloads() {
    connect(privacyProfile_->profile(), &QWebEngineProfile::downloadRequested,
            this, [this](QWebEngineDownloadRequest *download) {
        QString destination;
        if (AppSettings::askWhereToSaveDownloads()) {
            destination = QFileDialog::getSaveFileName(
                this, QStringLiteral("Save download"),
                QDir(AppSettings::downloadDirectory()).filePath(download->suggestedFileName()));
            if (destination.isEmpty()) {
                download->cancel();
                return;
            }
        } else {
            QDir directory(AppSettings::downloadDirectory());
            if (!directory.exists()) QDir().mkpath(directory.absolutePath());
            destination = directory.filePath(download->suggestedFileName());
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
    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        if (!ok || !AppSettings::passwordManagerEnabled() ||
            !AppSettings::autoFillPasswords()) return;
        if (!credentials_.forUrl(view->url()).isEmpty()) fillSavedLogin(true);
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
        if (i == tabs_->currentIndex() &&
            view->page()->lifecycleState() != QWebEnginePage::LifecycleState::Active) {
            view->page()->setLifecycleState(QWebEnginePage::LifecycleState::Active);
        }
    }

    if (auto *view = currentView()) address_->setText(view->url().toDisplayString());
    updateWindowTitle();
}

void BrowserWindow::installExtension() {
    if (!privacyProfile_->extensionsSupported()) return;

    QMessageBox chooser(this);
    chooser.setWindowTitle(QStringLiteral("Install Chrome Extension"));
    chooser.setText(QStringLiteral(
        "Knogn currently supports Chrome/Chromium Manifest V3 extensions through Qt WebEngine.\n\n"
        "Choose a ZIP package or an unpacked extension directory."));
    auto *zipButton = chooser.addButton(QStringLiteral("Choose ZIP"), QMessageBox::AcceptRole);
    auto *directoryButton = chooser.addButton(QStringLiteral("Choose Directory"), QMessageBox::ActionRole);
    chooser.addButton(QMessageBox::Cancel);
    chooser.exec();

    QString path;
    if (chooser.clickedButton() == zipButton) {
        path = QFileDialog::getOpenFileName(
            this, QStringLiteral("Install extension"), QString(),
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

void BrowserWindow::bookmarkCurrentPage() {
    auto *view = currentView();
    if (!view || !view->url().isValid() || view->url().scheme() == QStringLiteral("about")) return;
    const QString suggested = view->title().isEmpty() ? view->url().host() : view->title();
    bool ok = false;
    const QString title = QInputDialog::getText(
        this, QStringLiteral("Add bookmark"), QStringLiteral("Name"),
        QLineEdit::Normal, suggested, &ok);
    if (!ok) return;
    if (bookmarks_.add(title, view->url())) {
        rebuildBookmarksUi();
        statusBar()->showMessage(QStringLiteral("Bookmark saved"), 3000);
    }
}

void BrowserWindow::rebuildBookmarksUi() {
    if (!bookmarksMenu_ || !bookmarksToolbar_) return;
    bookmarksMenu_->clear();
    bookmarksToolbar_->clear();

    auto *add = bookmarksMenu_->addAction(QStringLiteral("Bookmark this page"));
    add->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    connect(add, &QAction::triggered, this, &BrowserWindow::bookmarkCurrentPage);
    auto *manage = bookmarksMenu_->addAction(QStringLiteral("Manage bookmarks…"));
    connect(manage, &QAction::triggered, this, &BrowserWindow::manageBookmarks);
    auto *import = bookmarksMenu_->addAction(QStringLiteral("Import bookmarks…"));
    connect(import, &QAction::triggered, this, &BrowserWindow::importBookmarks);

    if (!bookmarks_.entries().isEmpty()) bookmarksMenu_->addSeparator();
    for (const auto &entry : bookmarks_.entries()) {
        auto *menuAction = bookmarksMenu_->addAction(entry.title);
        menuAction->setToolTip(entry.url.toDisplayString());
        connect(menuAction, &QAction::triggered, this, [this, url = entry.url] {
            if (auto *view = currentView()) view->setUrl(url);
        });

        auto *barAction = bookmarksToolbar_->addAction(entry.title.left(28));
        barAction->setToolTip(entry.url.toDisplayString());
        connect(barAction, &QAction::triggered, this, [this, url = entry.url] {
            if (auto *view = currentView()) view->setUrl(url);
        });
    }
    bookmarksToolbar_->setVisible(!bookmarks_.entries().isEmpty());
}

void BrowserWindow::importBookmarks() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Import bookmarks"), QString(),
        QStringLiteral("Browser bookmarks (*.html *.htm *.json);;All files (*)"));
    if (path.isEmpty()) return;
    int imported = 0;
    QString error;
    if (!bookmarks_.importFile(path, &imported, &error)) {
        QMessageBox::warning(this, QStringLiteral("Bookmark import failed"), error);
        return;
    }
    rebuildBookmarksUi();
    QMessageBox::information(this, QStringLiteral("Bookmarks imported"),
                             QStringLiteral("Imported %1 new bookmark(s).").arg(imported));
}

void BrowserWindow::manageBookmarks() {
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Knogn Bookmarks"));
    dialog.resize(760, 480);
    auto *layout = new QVBoxLayout(&dialog);
    auto *table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Address")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table, 1);

    auto reload = [this, table] {
        table->setRowCount(bookmarks_.entries().size());
        for (int i = 0; i < bookmarks_.entries().size(); ++i) {
            const auto &entry = bookmarks_.entries().at(i);
            table->setItem(i, 0, new QTableWidgetItem(entry.title));
            table->setItem(i, 1, new QTableWidgetItem(entry.url.toDisplayString()));
        }
    };
    reload();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *open = buttons->addButton(QStringLiteral("Open"), QDialogButtonBox::ActionRole);
    auto *remove = buttons->addButton(QStringLiteral("Remove"), QDialogButtonBox::ActionRole);
    auto *import = buttons->addButton(QStringLiteral("Import…"), QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(open, &QPushButton::clicked, this, [this, table, &dialog] {
        const int row = table->currentRow();
        if (row < 0 || row >= bookmarks_.entries().size()) return;
        if (auto *view = currentView()) view->setUrl(bookmarks_.entries().at(row).url);
        dialog.accept();
    });
    connect(remove, &QPushButton::clicked, this, [this, table, reload] {
        if (bookmarks_.removeAt(table->currentRow())) {
            reload();
            rebuildBookmarksUi();
        }
    });
    connect(import, &QPushButton::clicked, this, [this, reload] {
        importBookmarks();
        reload();
    });
    dialog.exec();
}

void BrowserWindow::showSettings() {
    if (!showSettingsDialog(this)) return;
    QMessageBox::information(
        this, QStringLiteral("Settings saved"),
        QStringLiteral("Search, home page and download settings apply immediately. "
                       "Privacy policy changes apply to new windows. VPN/tunnel routing changes apply after restarting Knogn."));
}

void BrowserWindow::saveCurrentLogin() {
    if (privacyProfile_->isPrivate()) {
        QMessageBox::information(this, QStringLiteral("Private window"),
            QStringLiteral("Knogn does not save new credentials from private windows. Open the site in a normal window to save a login."));
        return;
    }
    if (!AppSettings::passwordManagerEnabled()) {
        QMessageBox::information(this, QStringLiteral("Password manager disabled"),
                                 QStringLiteral("Enable the password manager in Settings first."));
        return;
    }
    if (!credentials_.secureBackendAvailable()) {
        QMessageBox::warning(this, QStringLiteral("Secure credential store unavailable"),
            QStringLiteral("Knogn will not fall back to plaintext password storage. The operating-system credential store is unavailable."));
        return;
    }
    auto *view = currentView();
    const QString origin = view ? CredentialStore::originForUrl(view->url()) : QString();
    if (origin.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("No web login"),
                                 QStringLiteral("Open an HTTP or HTTPS site before saving a login."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Save login"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    auto *site = new QLineEdit(origin, &dialog);
    site->setReadOnly(true);
    auto *username = new QLineEdit(&dialog);
    auto *password = new QLineEdit(&dialog);
    password->setEchoMode(QLineEdit::Password);
    auto *label = new QLineEdit(&dialog);
    form->addRow(QStringLiteral("Site"), site);
    form->addRow(QStringLiteral("Username"), username);
    form->addRow(QStringLiteral("Password"), password);
    form->addRow(QStringLiteral("Label"), label);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;

    credentials_.saveCredential(origin, username->text(), password->text(), label->text(),
        [window = QPointer<BrowserWindow>(this)](bool ok, const QString &message) {
            if (!window) return;
            if (ok) window->statusBar()->showMessage(QStringLiteral("Login saved securely"), 4000);
            else QMessageBox::warning(window, QStringLiteral("Could not save login"), message);
        });
}

void BrowserWindow::fillSavedLogin(bool silent) {
    auto *view = currentView();
    if (!view || !AppSettings::passwordManagerEnabled()) return;
    QVector<CredentialMeta> matches = credentials_.forUrl(view->url());
    if (matches.isEmpty()) {
        if (!silent) QMessageBox::information(this, QStringLiteral("Saved logins"),
                                              QStringLiteral("No saved login matches this site."));
        return;
    }

    CredentialMeta selected = matches.first();
    if (!silent && matches.size() > 1) {
        QStringList usernames;
        for (const auto &item : matches) usernames.append(item.username);
        bool ok = false;
        const QString username = QInputDialog::getItem(
            this, QStringLiteral("Choose login"), QStringLiteral("Username"),
            usernames, 0, false, &ok);
        if (!ok) return;
        for (const auto &item : matches) {
            if (item.username == username) {
                selected = item;
                break;
            }
        }
    }

    QPointer<QWebEngineView> safeView(view);
    credentials_.readPassword(selected,
        [this, safeView, selected, silent](bool ok, QString password, const QString &message) {
            if (!safeView) return;
            if (!ok) {
                if (!silent) QMessageBox::warning(this, QStringLiteral("Could not read login"), message);
                return;
            }
            safeView->page()->runJavaScript(
                credentialFillScript(selected.username, password),
                [this, silent](const QVariant &result) {
                    if (!silent && !result.toBool()) {
                        QMessageBox::information(this, QStringLiteral("Login fields not found"),
                            QStringLiteral("This page does not expose a conventional username/password form that Knogn can fill."));
                    }
                });
        });
}

void BrowserWindow::importPasswords() {
    if (!credentials_.secureBackendAvailable()) {
        QMessageBox::warning(this, QStringLiteral("Secure credential store unavailable"),
            QStringLiteral("Knogn will not import passwords without an operating-system credential store."));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Import browser passwords"), QString(),
        QStringLiteral("Password exports (*.csv);;All files (*)"));
    if (path.isEmpty()) return;
    QString error;
    const int queued = credentials_.importCsv(path, &error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Password import failed"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Password import"),
        QStringLiteral("Queued %1 login(s) for secure storage in the operating-system credential store.").arg(queued));
}

void BrowserWindow::manageCredentials() {
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Saved Knogn Logins"));
    dialog.resize(780, 480);
    auto *layout = new QVBoxLayout(&dialog);
    auto *table = new QTableWidget(&dialog);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({QStringLiteral("Site"), QStringLiteral("Username"), QStringLiteral("Label")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table, 1);

    auto reload = [this, table] {
        const auto items = credentials_.entries();
        table->setRowCount(items.size());
        for (int i = 0; i < items.size(); ++i) {
            table->setItem(i, 0, new QTableWidgetItem(items[i].origin));
            table->setItem(i, 1, new QTableWidgetItem(items[i].username));
            table->setItem(i, 2, new QTableWidgetItem(items[i].label));
        }
    };
    reload();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *remove = buttons->addButton(QStringLiteral("Remove"), QDialogButtonBox::ActionRole);
    auto *import = buttons->addButton(QStringLiteral("Import CSV…"), QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(remove, &QPushButton::clicked, this, [this, table, reload] {
        const int row = table->currentRow();
        const auto items = credentials_.entries();
        if (row < 0 || row >= items.size()) return;
        credentials_.removeCredential(items[row], [reload](bool ok, const QString &) {
            if (ok) reload();
        });
    });
    connect(import, &QPushButton::clicked, this, [this, reload] {
        importPasswords();
        reload();
    });
    dialog.exec();
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
    if (looksLikeSearch) return AppSettings::searchUrl(input);
    return QUrl::fromUserInput(input);
}
