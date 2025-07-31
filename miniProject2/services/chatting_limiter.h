#ifndef CHATTING_LIMITER_H
#define CHATTING_LIMITER_H

#include <QQueue>
#include <QTimer>
#include <QDateTime>
#include <QDebug>

// geonwoo
class ChattingLimiter : public QObject {
    Q_OBJECT

public:
    ChattingLimiter(QObject *parent = nullptr) : QObject(parent), blocked(false) {};

    // 30초 차단 중일 때 메시지 전송을 요청했을 경우, 전송을 거부 처리.
    bool allowSendMsgCheck();

private:
    QQueue<qint64> timestamps;
    // client 채팅 차단 여부
    bool blocked;
    // client 채팅 차단 시작 시간
    qint64 blockStartTime;
};



#endif // CHATTING_LIMITER_H
