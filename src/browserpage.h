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

private:
    NewPageFactory newPageFactory_;
};
