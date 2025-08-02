#include "post.h"

// Getter 구현
int Post::getPostID() {
    return postID;
}

QString Post::getUserID() {
    return userID;
}

QString Post::getTitle() {
    return title;
}

QString Post::getContents() {
    return contents;
}

// Setter 구현
// 주의: ID는 보통 자동증가 필드라 set은 실제론 안 쓰일 수 있음
void Post::setPostID(int pid) {
    postID = pid;
}

void Post::setUserID(const QString& id) {
    userID = id;
}

void Post::setTitle(const QString& t) {
    title = t;
}

void Post::setContents(const QString& c) {
    contents = c;
}
