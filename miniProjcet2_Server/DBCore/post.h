#ifndef POST_H
#define POST_H

#include <QString>
#include <QDateTime>

class Post {
public:
    int getPostID();
    QString getUserID();
    QString getTitle();
    QString getContents();

    void setPostID(int postID);
    void setUserID(const QString& userID);
    void setTitle(const QString& title);
    void setContents(const QString& contents);

private:
    int postID;
    QString userID;
    QString title;
    QString contents;
};

#endif // POST_H
