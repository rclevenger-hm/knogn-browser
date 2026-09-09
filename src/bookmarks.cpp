#include "bookmarks.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextDocumentFragment>

#include <functional>

BookmarksStore::BookmarksStore() {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    path_ = QDir(dataDir).filePath(QStringLiteral("bookmarks.json"));
    load();
}

void BookmarksStore::load() {
    entries_.clear();
    QFile file(path_);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) return;
    for (const QJsonValue &value : doc.array()) {
        const QJsonObject obj = value.toObject();
        const QUrl url(obj.value(QStringLiteral("url")).toString());
        if (!url.isValid() || url.isEmpty()) continue;
        entries_.append({obj.value(QStringLiteral("title")).toString(), url});
    }
}

bool BookmarksStore::save() const {
    QJsonArray array;
    for (const auto &entry : entries_) {
        QJsonObject obj;
        obj.insert(QStringLiteral("title"), entry.title);
        obj.insert(QStringLiteral("url"), entry.url.toString());
        array.append(obj);
    }
    QFile file(path_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(QJsonDocument(array).toJson(QJsonDocument::Indented)) >= 0;
}

bool BookmarksStore::add(const QString &title, const QUrl &url) {
    if (!url.isValid() || url.isEmpty()) return false;
    for (const auto &entry : entries_) {
        if (entry.url == url) return true;
    }
    entries_.append({title.trimmed().isEmpty() ? url.host() : title.trimmed(), url});
    return save();
}

bool BookmarksStore::removeAt(int index) {
    if (index < 0 || index >= entries_.size()) return false;
    entries_.removeAt(index);
    return save();
}

int BookmarksStore::importHtml(const QByteArray &data) {
    const QString html = QString::fromUtf8(data);
    const QRegularExpression linkPattern(
        QStringLiteral("<A\\s+[^>]*HREF\\s*=\\s*[\"']([^\"']+)[\"'][^>]*>(.*?)</A>"),
        QRegularExpression::CaseInsensitiveOption |
        QRegularExpression::DotMatchesEverythingOption);

    int imported = 0;
    auto match = linkPattern.globalMatch(html);
    while (match.hasNext()) {
        const auto item = match.next();
        const QUrl url = QUrl::fromUserInput(item.captured(1).trimmed());
        if (!url.isValid() || url.isEmpty()) continue;
        QString title = QTextDocumentFragment::fromHtml(item.captured(2)).toPlainText().trimmed();
        const int before = entries_.size();
        add(title, url);
        if (entries_.size() > before) ++imported;
    }
    return imported;
}

int BookmarksStore::importChromiumJson(const QByteArray &data, QString *error) {
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (!doc.isObject()) {
        if (error) *error = parseError.errorString();
        return 0;
    }

    int imported = 0;
    std::function<void(const QJsonObject &)> walk = [&](const QJsonObject &node) {
        const QString type = node.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("url")) {
            const QUrl url(node.value(QStringLiteral("url")).toString());
            if (url.isValid() && !url.isEmpty()) {
                const int before = entries_.size();
                add(node.value(QStringLiteral("name")).toString(), url);
                if (entries_.size() > before) ++imported;
            }
        }
        const QJsonArray children = node.value(QStringLiteral("children")).toArray();
        for (const QJsonValue &child : children) walk(child.toObject());
    };

    const QJsonObject root = doc.object();
    if (root.contains(QStringLiteral("roots"))) {
        const QJsonObject roots = root.value(QStringLiteral("roots")).toObject();
        for (auto it = roots.begin(); it != roots.end(); ++it) {
            if (it.value().isObject()) walk(it.value().toObject());
        }
    } else {
        walk(root);
    }
    return imported;
}

bool BookmarksStore::importFile(const QString &path, int *importedCount, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    const QByteArray data = file.readAll();
    const QString suffix = QFileInfo(path).suffix().toLower();
    int imported = 0;
    if (suffix == QStringLiteral("json") || QFileInfo(path).fileName() == QStringLiteral("Bookmarks")) {
        imported = importChromiumJson(data, error);
    } else {
        imported = importHtml(data);
    }
    if (importedCount) *importedCount = imported;
    if (imported > 0) save();
    return error == nullptr || error->isEmpty();
}
