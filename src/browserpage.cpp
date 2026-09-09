#include "browserpage.h"

#include <utility>

BrowserPage::BrowserPage(QWebEngineProfile *profile,
                         NewPageFactory newPageFactory,
                         QObject *parent)
    : QWebEnginePage(profile, parent),
      newPageFactory_(std::move(newPageFactory)) {}

QWebEnginePage *BrowserPage::createWindow(WebWindowType) {
    return newPageFactory_ ? newPageFactory_() : nullptr;
}
