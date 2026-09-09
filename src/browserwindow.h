#pragma once

#include "bookmarks.h"
#include "credentialstore.h"

#include <QDir>
#include <QMainWindow>
#include <QUrl>

class PrivacyProfile;
class QAction;
class QLineEdit;
class QMenu;
class QProgressBar;
class QTabWidget;
class QTimer;
class QToolBar;
class QWebEnginePage;
class QWebEngineView;

class BrowserWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit BrowserWindow(bool privateMode = false, QWidget *parent = nullptr);

    QWebEnginePage *createPageForPopup();

private slots:
    void openLocation();
    void closeCurrentTab();
    void syncCurrentTab(int index = -1);
    void installExtension();
    void optimizeBackgroundTabs();
    void bookmarkCurrentPage();
    void importBookmarks();
    void manageBookmarks();
    void showSettings();
    void saveCurrentLogin();
    void fillSavedLogin(bool silent = false);
    void importPasswords();
    void manageCredentials();

private:
    QWebEngineView *addTab(const QUrl &url = QUrl(QStringLiteral("about:blank")),
                           bool makeCurrent = true);
    QWebEngineView *currentView() const;
    QUrl resolveInput(const QString &input) const;
    void wireView(QWebEngineView *view);
    void setupUi();
    void setupDownloads();
    void setupExtensions();
    void setupLifecycleController();
    void rebuildBookmarksUi();
    void updateWindowTitle();

    PrivacyProfile *privacyProfile_ = nullptr;
    BookmarksStore bookmarks_;
    CredentialStore credentials_;
    QTabWidget *tabs_ = nullptr;
    QLineEdit *address_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QAction *installExtensionAction_ = nullptr;
    QAction *bookmarkAction_ = nullptr;
    QMenu *bookmarksMenu_ = nullptr;
    QToolBar *bookmarksToolbar_ = nullptr;
    QTimer *lifecycleTimer_ = nullptr;
};
