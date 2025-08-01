#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <QList>
#include <QString>

#include <QSqlQuery>

#include "usermanage.h"

class QJsonObject;

class ClientHandler : public QObject {
    Q_OBJECT

public:
    explicit ClientHandler(QTcpSocket* socket, QObject* parent = nullptr);

signals:
    void loggedIn(ClientHandler* handler);
    void disconnected(ClientHandler* handler);

public slots:
    void onReadyRead();
    void onDisconnected();
    void sendMessageToClient(QByteArray data);

signals:
    QSqlQuery requestQuery(const QString& strQuery, bool& isSuccess);

public:
    userManage* GetUserMange() { return usermanage;}
private:
    void readyRead_LoginRequest(const QJsonObject& obj);
    void readyRead_SignRequest(const QJsonObject& obj);
    void readyRead_MessageSendRequest(const QJsonObject& obj);
    void readyRead_FileSend(const QJsonObject& obj);
    void readyRead_FileDownload(const QJsonObject& obj);
    void readyRead_GiveLog(const QJsonObject& obj);
    void readyRead_Trade(const QJsonObject& obj);
    void readyRead_Report(const QJsonObject& obj);
    void readyRead_EmailCheck(const QJsonObject& obj);
    void readyRead_Emailcodecheck(const QJsonObject& obj);
private:
    QTcpSocket* socket;
    qintptr socketDescriptor;
    QByteArray readBuffer;
    userManage* usermanage;
    
    // 이메일 인증 관련 클라이언트별 변수들
    QString pendingEmail;        // 현재 인증 진행 중인 이메일
    QString savedCode;    // 해당 클라이언트의 인증 코드

};

#endif // CLIENTHANDLER_H
