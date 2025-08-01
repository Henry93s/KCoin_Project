#ifndef POSTMANAGER_H
#define POSTMANAGER_H

#include "post.h"
#include <QVector>
#include <QString>

class PostManager {
public:
    static PostManager* instance();

    QVector<Post> getAllPosts();
    bool addPost(const QString& title, const QString& content, const QString& author);
    bool deletePost(int postId, const QString& currentUserId);

private:
    static PostManager* m_instance;
    PostManager();
};

#endif // POSTMANAGER_H
