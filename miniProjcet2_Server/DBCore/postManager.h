#ifndef POSTMANAGER_H
#define POSTMANAGER_H

#include "post.h"
#include <QVector>
#include <QObject>
#include <QString>
#include <QSqlQuery>

class PostManager : public QObject {
    Q_OBJECT
public:
    static PostManager* instance();

    void getAllPosts();
    bool addPost(const QString& title, const QString& content, const QString& author);
    bool deletePost(int postId, const QString& currentUserId);

private:
    QVector<Post> posts;
    static PostManager* m_instance;
    PostManager();

signals:
    QSqlQuery requestQuery(const QString& strQuery, bool& isSuccess);
};

#endif // POSTMANAGER_H
