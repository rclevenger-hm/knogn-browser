#pragma once

#include <QMainWindow>
#include <QUrl>

class PrivacyProfile;
class QAction;
class QLineEdit;
class QProgressBar;
class QTabWidget;
class QTimer;
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
    void updateWindowTitle();

    PrivacyProfile *privacyProfile_ = nullptr;
    QTabWidget *tabs_ = nullptr;
    QLineEdit *address_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QAction *installExtensionAction_ = nullptr;
    QTimer *lifecycleTimer_ = nullptr;
};
