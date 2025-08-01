#include "postmanager.h"
// #include <QSqlDatabase>
// #include <QSqlQuery>
// #include <QSqlError>
// #include <QDebug>

PostManager* PostManager::m_instance = nullptr;

// 싱글톤 instance() 구현
PostManager* PostManager::instance() {
    if (!m_instance)
        m_instance = new PostManager();
    return m_instance;
}

// PostManager 생성자 구현
PostManager::PostManager() {
    // 초기화 작업 등
}

// 게시글 전체 가져오기
QVector<Post> PostManager::getAllPosts() {
    QVector<Post> posts;

    // 아래는 DB 기능 주석 처리된 부분
    /*
    QSqlQuery query("SELECT id, title, content, author, created_at FROM posts ORDER BY created_at DESC");
    while (query.next()) {
        Post post;
        post.id = query.value(0).toInt();
        post.title = query.value(1).toString();
        post.content = query.value(2).toString();
        post.author = query.value(3).toString();
        post.createdAt = query.value(4).toDateTime();
        posts.append(post);
    }
    */

    return posts;
}

// 게시글 추가
bool PostManager::addPost(const QString& title, const QString& content, const QString& author) {
    // DB 기능 비활성화
    /*
    QSqlQuery query;
    query.prepare("INSERT INTO posts (title, content, author) VALUES (?, ?, ?)");
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(author);
    return query.exec();
    */

    return true; // 임시로 성공 반환
}

// 게시글 삭제 (작성자 확인 포함)
bool PostManager::deletePost(int postId, const QString& currentUserId) {
    // DB 기능 비활성화
    /*
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT author FROM posts WHERE id = ?");
    checkQuery.addBindValue(postId);
    if (!checkQuery.exec() || !checkQuery.next())
        return false;

    if (checkQuery.value(0).toString() != currentUserId)
        return false; // 본인 확인 실패

    QSqlQuery query;
    query.prepare("DELETE FROM posts WHERE id = ?");
    query.addBindValue(postId);
    return query.exec();
    */

    return true; // 임시로 성공 반환
}

