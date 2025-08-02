#include "postmanager.h"
#include "post.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

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
void PostManager::getAllPosts() {

    bool isSuccess;
    QSqlQuery query = emit requestQuery(QString("SELECT postID, userID, title, contents FROM coin.`Post`"), isSuccess);
    if(!isSuccess){
        qDebug() << query.lastError().text();
    }

    while (query.next()) {
        Post post;
        post.setPostID(query.value(0).toInt());
        post.setUserID(query.value(1).toString());
        post.setTitle(query.value(2).toString());
        post.setContents(query.value(3).toString());
        posts.append(post);
    }

    return;
}

// 게시글 추가
bool PostManager::addPost(const QString& title, const QString& contents, const QString& userID) {
    bool isSuccess;

    QString sql = QString("INSERT INTO coin.Post (userID, title, contents) "
                          "VALUES ('%1', '%2', '%3')")
                      .arg(userID, title, contents);

    QSqlQuery query = emit requestQuery(sql, isSuccess);
    if (!isSuccess) {
        qDebug() << "[addPost] Insert failed:" << query.lastError().text();
        return false;
    }

    // INSERT 후 최신 게시글 목록 다시 불러오기
    getAllPosts();

    return true;
}

// 게시글 삭제 (작성자 확인 포함)
bool PostManager::deletePost(int postID, const QString& currentUserID) {
    bool isSuccess;

    // 1. 작성자 확인
    QString checkSql = QString("SELECT userID FROM coin.Post WHERE postID = %1").arg(postID);
    QSqlQuery checkQuery = emit requestQuery(checkSql, isSuccess);

    if (!isSuccess || !checkQuery.next()) {
        qDebug() << "[deletePost] Post not found or DB error:" << checkQuery.lastError().text();
        return false;
    }

    QString postUserID = checkQuery.value(0).toString();
    if (postUserID != currentUserID) {
        qDebug() << "[deletePost] 작성자 불일치. 삭제 불가.";
        return false;
    }

    // 2. 삭제 쿼리
    QString deleteSql = QString("DELETE FROM coin.Post WHERE postID = %1").arg(postID);
    QSqlQuery deleteQuery = emit requestQuery(deleteSql, isSuccess);

    if (!isSuccess) {
        qDebug() << "[deletePost] Delete failed:" << deleteQuery.lastError().text();
        return false;
    }

    // QVector<Post>에서도 제거
    for (int i = 0; i < posts.size(); ++i) {
        if (posts[i].getPostID() == postID) {
            posts.remove(i);
            break;
        }
    }

    return true;
}

/*
bool PostManager::deletePost(int postID, const QString& currentUserID) {
    // 게시글 작성자 확인
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT userID FROM coin.Post WHERE postID = ?");
    checkQuery.addBindValue(postID);

    if (!checkQuery.exec() || !checkQuery.next()) {
        qDebug() << "[deletePost] Post not found or DB error:" << checkQuery.lastError().text();
        return false;
    }

    QString postUserID = checkQuery.value(0).toString();
    if (postUserID != currentUserID) {
        qDebug() << "[deletePost] 작성자 불일치. 삭제 불가.";
        return false;
    }

    // 삭제 실행
    QSqlQuery deleteQuery;
    deleteQuery.prepare("DELETE FROM coin.Post WHERE postID = ?");
    deleteQuery.addBindValue(postID);

    if (!deleteQuery.exec()) {
        qDebug() << "[deletePost] Delete failed:" << deleteQuery.lastError().text();
        return false;
    }

    // QVector<Post>에서도 제거
    for (int i = 0; i < posts.size(); ++i) {
        if (posts[i].getPostID() == postID) {
            posts.remove(i);
            break;
        }
    }

    return true;
}
*/
