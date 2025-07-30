#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QList>
#include "clientHandler.h"
#include "clientsetup.h"
#include <QMutex>

class ServerManager : public QObject {
    Q_OBJECT
public:
    static ServerManager& getInstance();
    QString getMyIP();

public slots:
    void addClient(ClientHandler* handler);
    void removeClient(ClientHandler* handler);
    void broadcastMessage(QByteArray& data);
    void clientConnect();

private:
    QTcpServer* tcpServer;
    QList<ClientHandler*> clientHandlerList;
    QList<ClientSetup*>* setupList;
    // 생성자
    ServerManager(QObject* parent = nullptr);                   // 생성자 private처리
    ServerManager(const ServerManager&) = delete;               // 복사 생성자 삭제
    ServerManager& operator=(const ServerManager&) = delete;    // 대입 연산자 삭제
    QMutex clientListMutex;

    void run();
};

#endif // SERVERMANAGER_H
