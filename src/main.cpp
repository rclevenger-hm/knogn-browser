#include "branding.h"
#include "browserflags.h"
#include "browserwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>

int main(int argc, char *argv[]) {
    // QSettings-backed startup configuration needs the application identity set
    // before WebEngine reads Chromium flags during QApplication construction.
    QCoreApplication::setOrganizationName(QStringLiteral("Knogn"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("knogn.local"));
    QCoreApplication::setApplicationName(QStringLiteral("Knogn"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KNOGN_VERSION));

    applyKnognChromiumFlags();

    QApplication app(argc, argv);
    app.setApplicationDisplayName(QStringLiteral("Knogn"));
    app.setWindowIcon(Branding::applicationIcon(256));
    app.setStyleSheet(Branding::applicationStyleSheet());

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Knogn — fast by omission, private by design."));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption privateOption(
        QStringList{QStringLiteral("p"), QStringLiteral("private")},
        QStringLiteral("Start with an off-the-record memory-only profile."));
    parser.addOption(privateOption);
    parser.process(app);

    BrowserWindow window(parser.isSet(privateOption));
    window.show();
    return app.exec();
}
