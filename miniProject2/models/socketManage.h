#ifndef SOCKETMANAGE_H
#define SOCKETMANAGE_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument> // QJsonDocument, QJsonObject 사용을 위해 추가
#include <QJsonObject>
#include <QJsonParseError> // QJsonParseError 사용을 위해 추가
#include <QMessageBox>

class SocketManage : public QObject {
    Q_OBJECT
public:
    static SocketManage& instance();
    QTcpSocket* socket() const { return m_socket; }

signals:
    // 서버로부터 로그인 응답을 받았을 때 방출할 시그널
    void loginResponseReceived(const QJsonObject& response);
    // 서버로부터 채팅 메시지를 받았을 때 방출할 시그널
    void chatMessageReceived(const QJsonObject& message);
    // 서버로부터 파일을 받았을 때 방출할 시그널
    void fileReceived(const QJsonObject& fileData);
    // 서버로부터 채팅 기록 받아왔을 때 방출할 시그널
    void chattingLogReceived(const QJsonObject& chatLog);
    // 서버로부터 거래완료 신호 받아왔을 때 시그널
    void tradeResponseReceived(const QJsonObject &obj);
    // 서버로부터 이메일 인증 성공 신호 받아왔을 때 시그널
    void emailCheckResponseReceived();
    // 서버로부터 파일 다운로드 응답을 받았을 때 시그널
    void fileDownloadReceived(const QJsonObject& response);

    // 서버로부터 게시판 정보들 받음
    void allPostsReceived(const QJsonObject& obj);
    void searchPostReceived(const QJsonObject& obj);
private slots:
    // QTcpSocket의 readyRead 시그널을 받아서 모든 데이터를 처리할 슬롯
    void onSocketReadyRead();

private:
    explicit SocketManage(QObject *parent = nullptr);
    // 복사 생성자 및 대입 연산자 비활성화 (싱글턴 패턴)
    SocketManage(const SocketManage&) = delete;
    SocketManage& operator=(const SocketManage&) = delete;

    QTcpSocket* m_socket;
    QByteArray m_buffer; // 수신 데이터를 임시로 저장할 버퍼
};

#endif // SOCKETMANAGE_H
