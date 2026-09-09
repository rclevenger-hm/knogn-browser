#pragma once

#include <QWebEnginePage>
#include <functional>

class BrowserPage final : public QWebEnginePage {
    Q_OBJECT
public:
    using NewPageFactory = std::function<QWebEnginePage *()>;

    BrowserPage(QWebEngineProfile *profile,
                NewPageFactory newPageFactory,
                QObject *parent = nullptr);

protected:
    QWebEnginePage *createWindow(WebWindowType type) override;
    bool acceptNavigationRequest(const QUrl &url,
                                 NavigationType type,
                                 bool isMainFrame) override;

private:
    void setFullScreenMode(bool enabled);

    NewPageFactory newPageFactory_;
    Qt::WindowStates previousWindowState_ = Qt::WindowNoState;
    bool fullScreenActive_ = false;
};
