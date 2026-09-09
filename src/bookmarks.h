#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QVector>

struct BookmarkEntry {
    QString title;
    QUrl url;
};

class BookmarksStore {
public:
    BookmarksStore();

    const QVector<BookmarkEntry> &entries() const { return entries_; }
    bool add(const QString &title, const QUrl &url);
    bool removeAt(int index);
    bool importFile(const QString &path, int *importedCount = nullptr, QString *error = nullptr);

private:
    void load();
    bool save() const;
    int importHtml(const QByteArray &data);
    int importChromiumJson(const QByteArray &data, QString *error);

    QString path_;
    QVector<BookmarkEntry> entries_;
};
