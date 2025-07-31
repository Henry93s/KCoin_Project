#include "chatting_limiter.h"

// geonwoo
/*
 * Flood Protection (과도한 요청 방지)
 *  : 일정 시간 내 한 client 가 보낼 수 있는 요청(채팅 MSG) 수를
  제한하여, DOS(하나의 ip)공격(반복 요청으로 서버 마비를 방지.
 => 채팅 메시지 전송을 초당 3회까지로 제한하는 Burst Limit 를 적용함.
*/

// 30초 차단 중일 때 메시지 전송을 요청했을 때, 메시지 전송을 거부함
bool ChattingLimiter::allowSendMsgCheck() {
    qint64 nowTime = QDateTime::currentMSecsSinceEpoch();

    // 30초 차단 중이면 전송을 거부함
    if (blocked) {
        if (nowTime - blockStartTime > 30000) {
            blocked = false;
            timestamps.clear();
        } else {
            qDebug() << "현재 " << QDateTime::fromMSecsSinceEpoch(blockStartTime).toString(Qt::ISODate) << " 시각부터 30초 차단이 적용되어 있습니다.";
            return false;
        }
    }

    // 1. 최근 1초 이내 메시지들만 필터링
    // (1초 이내 메시지의 timestamp cnt 를 측정하기 위함)
    while(!timestamps.isEmpty() && nowTime - timestamps.head() > 1000){
        timestamps.dequeue();
    }
    // 2. 초당 3회 제한
    if(timestamps.size() > 3){
        blockStartTime = nowTime;
        blocked = true;
        qDebug() << "채팅 입력은 초당 3회로 제한되어, 30초간 채팅을 입력할 수 없습니다.";
        return false;
    }

    timestamps.enqueue(nowTime);
    return true;
}
