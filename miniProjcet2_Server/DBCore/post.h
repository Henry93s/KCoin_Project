#ifndef POST_H
#define POST_H

#include <QString>
#include <QDateTime>

struct Post {
    int id;
    QString title;
    QString content;
    QString author;
    QDateTime createdAt;
};

#endif // POST_H
