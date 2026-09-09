#include "browserflags.h"
#include "browserwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[]) {
    // Qt WebEngine reads Chromium flags during initialization, so privacy flags
    // must be applied before QApplication constructs any WebEngine objects.
    applyKnognChromiumFlags();

    QCoreApplication::setOrganizationName(QStringLiteral("Knogn"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("knogn.local"));
    QCoreApplication::setApplicationName(QStringLiteral("Knogn"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KNOGN_VERSION));

    QApplication app(argc, argv);
    app.setApplicationDisplayName(QStringLiteral("Knogn"));

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
