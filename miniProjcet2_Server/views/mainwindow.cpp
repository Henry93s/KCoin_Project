#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "usermanage.h"

#include <QMessageBox>
#include <QDebug>
#include <QNetworkInterface>
#include <QHostAddress>


#define PORT_NUM  51234


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /*  ================== 서버 구현함에 따라 주석처리
    // 클라이언트 소켓 리스트 - devwooms
    clientSocketList = new QList<QTcpSocket*>();


    // Tcp 서버 객체 생성 후 시그널 연결 - devwooms
    tcpServer = new QTcpServer(this);
    // 새로운 연결이 될 때마다 clientConnect 실행 - devwooms
    connect(tcpServer, SIGNAL(newConnection()), SLOT(clientConnect()));
    
    // TCP 서버를 열 수 없는 경우, 사용자에게 오류 메시지를 보여주고 프로그램을 종료 - devwooms
    if(!tcpServer->listen(QHostAddress::Any, PORT_NUM)){
        QMessageBox::critical(this, tr("Echo Server"), tr("Unable to start the server.%1.").arg(tcpServer -> errorString()));
        close();
        return;
    }

    // IP 주소 가져오기 - devwooms
    QString myIP = getMyIP();
    QString serverInfo = QString("%1:%2").arg(myIP).arg(tcpServer->serverPort());
    
    // Ui에 보여주기 ( 테스트 코드 나중에는 트레이로만 ) - devwooms
    ui->serverState->setText(tr("The server is running on %1").arg(serverInfo));
    qDebug() << "Server started on:" << serverInfo;
    */
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 현재 컴퓨터의 외부에서 접근 가능한 IP 가져오기 - devwooms
QString MainWindow::getMyIP()
{

    // 현재 컴퓨터에 있는 모든 네트워크 인터페이스 목록 가져오기 - devwooms
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    // 가져온 네트워크 인터페이스 목록 interfaces를 for문으로 확인 - devwooms
    for (const QNetworkInterface &interface : interfaces) {
        // IsLoopBack   - 127.0.0.1 처럼 자신과 통신하는 네트워크 인서페이스 - devwooms
        // IsUp         - Off된 상태 네트워크 인터페이스 - devwooms
        // IsRunning    - 켜지있지만(On) 활성화되지 않은 네으퉈크 인터페이스 - devwooms
        // 위에 해당되는 네트워크 인터페이스는 무시 - devwooms
        if (interface.flags() & QNetworkInterface::IsLoopBack || 
            !(interface.flags() & QNetworkInterface::IsUp) ||
            !(interface.flags() & QNetworkInterface::IsRunning)) {
            continue;
        }
        
        // interface에 할당된 IP 주소 정보 리스트 가져옴 - devwooms
        // interface는 여러 IP 주소를 가질 수도 있어서 List로 받음 - devwooms
        const QList<QNetworkAddressEntry> addressEntries = interface.addressEntries();
        // entry만큼 for문 - devwooms
        for (const QNetworkAddressEntry &entry : addressEntries) {
            // ip 주소 - devwooms
            QHostAddress ipAddress = entry.ip();
            // IPv4Protocol - IPv4인지 확인 - devwooms
            // LocalHost    - 로컬인지 확인 - devwooms
            // ip주소가 로컬이면 안되고 IPv4로 통신할거니깐 조건에 맞다면 Return - devwooms
            if (ipAddress.protocol() == QAbstractSocket::IPv4Protocol &&
                ipAddress != QHostAddress::LocalHost) {
                return ipAddress.toString();
            }
        }
    }

    // IP를 못 찾을 경우 로컬로 반환 - devwooms
    return "127.0.0.1";
}

// 클라이언트 연결 시 실행 함수 - devwooms
void MainWindow::clientConnect()
{
    /*  ================== 서버 구현함에 따라 주석처리
    // 서버와 연결된 클라이언트의 소켓을 반환해줌 ( nextPendingConnection() ) - devwooms
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();

    // 클라이언트 소켓 리스트에 추가 - devwooms
    clientSocketList->append(clientSocket);
    
    // 클라이언트 소켓이 disconnected(연결 해제)시 클라이언트 소켓에서 해당 소켓을 지우고 메모리에서 삭제 - devwooms
    connect(clientSocket, &QTcpSocket::disconnected, this, [this, clientSocket](){
        clientSocketList->removeOne(clientSocket);
        clientSocket->deleteLater();
    });
    

    qDebug() << "새로운 클라이언트 연결됨 / 총 클라이언트 수 : " << clientSocketList->size();
   */
}

