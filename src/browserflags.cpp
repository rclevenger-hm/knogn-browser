#include "browserflags.h"

#include "appsettings.h"

#include <QList>
#include <QtGlobal>

namespace {
QList<QByteArray> requiredPrivacyFlags() {
    return {
        QByteArrayLiteral("--disable-component-update"),
        QByteArrayLiteral("--disable-domain-reliability"),
        QByteArrayLiteral("--disable-sync"),
        QByteArrayLiteral("--disable-breakpad"),
        QByteArrayLiteral("--no-pings")
    };
}

void appendIfMissing(QList<QByteArray> &flags, const QByteArray &flag) {
    for (const QByteArray &entry : flags) {
        if (entry.contains(flag.left(flag.indexOf('=') > 0 ? flag.indexOf('=') : flag.size()))) {
            return;
        }
    }
    flags.append(flag);
}
}

QByteArray knognChromiumFlags() {
    QList<QByteArray> flags;
    const QByteArray existing = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS").trimmed();
    if (!existing.isEmpty()) flags.append(existing);

    for (const QByteArray &flag : requiredPrivacyFlags()) appendIfMissing(flags, flag);

    const QString proxyMode = AppSettings::proxyMode();
    if (proxyMode == QStringLiteral("direct")) {
        appendIfMissing(flags, QByteArrayLiteral("--no-proxy-server"));
    } else if (proxyMode == QStringLiteral("http") ||
               proxyMode == QStringLiteral("socks5")) {
        QString endpoint = AppSettings::proxyEndpoint().trimmed();
        endpoint.remove(QChar(' '));
        if (!endpoint.isEmpty() && !endpoint.contains(QChar('@'))) {
            if (!endpoint.contains(QStringLiteral("://"))) {
                endpoint.prepend(proxyMode + QStringLiteral("://"));
            }
            appendIfMissing(flags,
                QByteArrayLiteral("--proxy-server=") + endpoint.toUtf8());
        }
    }

    if (proxyMode != QStringLiteral("direct")) {
        QString bypass = AppSettings::proxyBypassList();
        bypass.remove(QChar(' '));
        if (!bypass.isEmpty()) {
            appendIfMissing(flags,
                QByteArrayLiteral("--proxy-bypass-list=") + bypass.toUtf8());
        }
    }

    return flags.join(' ');
}

void applyKnognChromiumFlags() {
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", knognChromiumFlags());
}
