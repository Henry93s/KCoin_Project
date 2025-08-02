#include "socketManage.h"
#include <QDebug> // 디버그 출력을 위해 추가

// 싱글턴 인스턴스 초기화
SocketManage& SocketManage::instance() {
    static SocketManage instance;
    return instance;
}

// 생성자: 소켓 초기화 및 readyRead 시그널 연결
SocketManage::SocketManage(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)) {
    // 소켓의 readyRead 시그널을 SocketManage의 onSocketReadyRead 슬롯에 연결
    connect(m_socket, &QTcpSocket::readyRead, this, &SocketManage::onSocketReadyRead);

    // 연결/연결 해제 등 소켓 상태 변화 디버깅을 위한 연결 (선택 사항)
    connect(m_socket, &QTcpSocket::connected, [](){ qDebug() << "SocketManage: 서버 연결 완료"; });
    connect(m_socket, &QTcpSocket::disconnected, [](){ qDebug() << "SocketManage: 서버 연결 해제"; });
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            [](QAbstractSocket::SocketError socketError){
                qDebug() << "SocketManage: 소켓 에러 발생:" << socketError;
            });
}

// 모든 수신 데이터를 처리하는 슬롯
void SocketManage::onSocketReadyRead() {
    m_buffer.append(m_socket->readAll()); // 소켓에서 읽을 수 있는 모든 데이터를 버퍼에 추가

    qDebug() << "SocketManage: readyRead 발생. 현재 버퍼 크기:" << m_buffer.size();

    // 버퍼에서 완전한 메시지를 파싱
    while(true){
        int newlineIndex = m_buffer.indexOf('\n'); // 메시지 끝을 나타내는 개행 문자 찾기
        // 완전한 메시지가 없으면 루프 종료
        if(newlineIndex == -1) {
            qDebug() << "SocketManage: 완전한 메시지 없음. 버퍼에 남은 데이터:" << m_buffer.size();
            break;
        }

        QByteArray oneMessage = m_buffer.left(newlineIndex); // 개행 문자 전까지의 데이터 추출
        m_buffer.remove(0, newlineIndex + 1); // 추출한 메시지와 개행 문자를 버퍼에서 제거

        qDebug() << "SocketManage: 파싱된 메시지:" << QString::fromUtf8(oneMessage);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(oneMessage, &err); // JSON 파싱

        // JSON 파싱 에러 또는 객체가 아니면 건너뛰기
        if(err.error != QJsonParseError::NoError || !doc.isObject()){
            qDebug() << "SocketManage: JSON 파싱 에러 발생:" << err.errorString();
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj.value("type").toString(); // 메시지 타입 추출

        // 메시지 타입에 따라 적절한 시그널 방출
        if (type == "response") { // 로그인 응답 메시지
            qDebug() << "SocketManage: loginResponseReceived 시그널 방출";
            emit loginResponseReceived(obj);
        } else if (type == "messagesend") { // 채팅 메시지
            qDebug() << "SocketManage: chatMessageReceived 시그널 방출";
            emit chatMessageReceived(obj);
        }
        // file - devwooms
        else if (type == "filesend"){
            qDebug() << "SocketManage: fileReceived 시그널 방출";
            emit fileReceived(obj);
        }
        else if (type == "messagelog"){  // 메세지 기록 보내줌
            qDebug()<< "message log 불러옴";
            emit chattingLogReceived(obj);
        }
        else if (type == "traderesponse"){
            emit tradeResponseReceived(obj);
        }
        else if (type == "filedownload"){  // 파일 다운로드 응답
            qDebug() << "SocketManage: fileDownloadReceived 시그널 방출";
            emit fileDownloadReceived(obj);
        }
        else if (type == "yourebanned"){
            qDebug() << "신고로 인한 채팅 정지 상태 안내";
            QMessageBox::warning(nullptr, "채팅 정지", "신고로 인한 채팅 정지 상태입니다.");
        }
        else if (type == "emailtrue"){
            qDebug() << "이메일 인증 성공";
            emit emailCheckResponseReceived();
        }
        else if (type == "emailfalse"){
            qDebug() << "이메일 인증 실패";
            QMessageBox::warning(nullptr, "인증 코드 불일치", "인증 번호를 다시 확인해주세요.");
        }
        else if (type == "postRead"){

        }
        else if (type == "postAllRead"){
            emit allPostsReceived(obj);
        }
        // geonwoo 게시판 글 추가 / 삭제 처리 응답 확인
        else if (type == "postWrite"){
            emit postWriteReceived(obj);
        }
        else if (type == "postDelete"){
            emit postDeleteReceived(obj);
        }
        else {
            qDebug() << "SocketManage: 알 수 없는 메시지 타입:" << type;
        }
    }
}
