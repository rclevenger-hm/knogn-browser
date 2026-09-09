#include "browserflags.h"

#include <QList>
#include <QtGlobal>

namespace {
QList<QByteArray> requiredPrivacyFlags() {
    return {
        QByteArrayLiteral("--disable-background-networking"),
        QByteArrayLiteral("--disable-component-update"),
        QByteArrayLiteral("--disable-domain-reliability"),
        QByteArrayLiteral("--disable-sync"),
        QByteArrayLiteral("--disable-breakpad"),
        QByteArrayLiteral("--no-pings")
    };
}
}

QByteArray knognChromiumFlags() {
    QList<QByteArray> flags;
    const QByteArray existing = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS").trimmed();
    if (!existing.isEmpty()) {
        flags.append(existing);
    }

    for (const QByteArray &flag : requiredPrivacyFlags()) {
        bool alreadyPresent = false;
        for (const QByteArray &entry : flags) {
            if (entry.contains(flag)) {
                alreadyPresent = true;
                break;
            }
        }
        if (!alreadyPresent) flags.append(flag);
    }

    return flags.join(' ');
}

void applyKnognChromiumFlags() {
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", knognChromiumFlags());
}
