#include "chattingRoomView.h"
#include "models/socketManage.h"    // socketManage 사용을 위해 추가
#include "models/sendingManage.h"   // sendingManage 사용을 위해 추가
#include <QDebug>            // 디버그 출력을 위해 추가

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QIcon>
#include <QTimer> // QTimer 추가
#include "../services/chatting_limiter.h"

ChattingLimiter limiter;

//==================================
//        채팅방 기능 구현
//==================================
ChattingRoomView::ChattingRoomView(const QString& name, QWidget *parent)
    : DropWidget(parent, name), chatViewName(name)          // name 변수를 chatViewName에....
{
    setupUI();
    this->setWindowTitle(chatViewName);
    //==========================
    //  메세지 기록이 있다면 받음
    //==========================
    // 채팅방 이름의 로그를 불러와달라고 요청 보냄
    sendingManage::instance()->giveMeLog(chatViewName);
    // type == messagelog일 때 처리
    connect(&SocketManage::instance(), &SocketManage::chattingLogReceived, this, [this](const QJsonObject& chatLog){
        QJsonArray logArray = chatLog["log"].toArray();

        if (chatLog["exist"]=="yes"){
            for (const QJsonValue& v : logArray) {
                QString line = v.toString();
                
                // HTML 링크인지 일반 텍스트인지 구분하여 처리
                if (line.contains("<a href=")) {
                    // 파일 링크는 HTML로 그대로 표시
                    textBrowser->append(line);
                    qDebug() << "로그 파일 링크 표시:" << line;
                } else {
                    // 일반 텍스트는 HTML 이스케이프 처리
                    QString escapedLine = line.toHtmlEscaped();
                    textBrowser->append(escapedLine);
                    qDebug() << "로그 일반 텍스트 표시:" << escapedLine;
                }
            }
            qDebug() << "로그 불러옴";
        }
        else{
            qDebug() << "불러올 로그 없음";
        }
    });

    //==========================
    //   서버로부터 메세지 받음
    //==========================
    // SocketManage에서 방출하는 chatMessageReceived 시그널을 받도록 변경
    connect(&SocketManage::instance(), &SocketManage::chatMessageReceived, this, [this](const QJsonObject& message) {
        qDebug() << "ChattingWindow: chatMessageReceived 시그널 받음";
        // 이 message는 이미 SocketManage에서 파싱된 JSON 객체임
        // textBrowser에 message 띄우기
        if(message["type"].toString() == "messagesend"){ // 타입이 'messagesend'인지 다시 확인
            QString chatName = message["chatViewName"].toString();
            QString text = message["textMessage"].toString();
            if(chatName == this->chatViewName){ // 현재 채팅방 이름과 일치하는 메시지만 표시
                
                // HTML 링크인지 일반 텍스트인지 구분하여 처리
                if (text.contains("<a href=")) {
                    // 파일 링크는 HTML로 그대로 표시
                    textBrowser->append(text);
                    qDebug() << "파일 링크 메시지 표시:" << text;
                } else {
                    // 일반 텍스트는 HTML 이스케이프 처리
                    QString escapedText = text.toHtmlEscaped();
                    textBrowser->append(escapedText);
                    qDebug() << "일반 텍스트 메시지 표시:" << escapedText;
                }
                
                qDebug() << chatName<<"으로 메세지 띄움";
            }
        }
    });

    //==========================
    //   서버로부터 파일 받음
    //==========================
    connect(&SocketManage::instance(), &SocketManage::fileReceived, this, [this](const QJsonObject& fileData) {
        qDebug() << "ChattingWindow: fileReceived 시그널 받음";
        
        if(fileData["type"].toString() == "filesend") {
            QString chatName = fileData["chatViewName"].toString();
            if(chatName == this->chatViewName) { // 현재 채팅방과 일치하는 파일만 처리
                QString fileName = fileData["fileName"].toString();
                int fileSize = fileData["fileSize"].toInt();
                
                // 채팅창에 파일 수신 메시지 표시
                textBrowser->append(QString("[파일 수신] %1 (%2 bytes)").arg(fileName).arg(fileSize));
                qDebug() << "파일 수신:" << fileName;
            }
        }
    });

    //==========================
    //    서버로 메세지 전송
    //==========================
    connect(pushButton_2, &QPushButton::pressed,this,[this](){
        QString message = lineEdit->text();
        
        if (!message.isEmpty()) { // 빈 메시지는 전송하지 않음
            lineEdit->clear();
            
            // 메시지 복사본 생성 (참조 문제 방지)
            QString messageCopy = QString(message);

            // geonwoo
            // true : 1초에 3회 초과 채팅 메시지 연결 시도로 채팅이 30초 동안 제한됨
            if(limiter.allowSendMsgCheck()){
                sendingManage::instance()->sendMessage(chatViewName, messageCopy);
                qDebug() << "메시지 전송 시도: " << messageCopy;
            } else {
                // false : 채팅 제한 없음
                qDebug() << "채팅이 차단되어 있음";
            }
        }
    });

    //==========================
    //    Enter 키로 메세지 전송
    //==========================
    connect(lineEdit, &QLineEdit::returnPressed, this, [this](){
        QString message = lineEdit->text();
        
        if (!message.isEmpty()) {
            lineEdit->clear();
            QString messageCopy = QString(message);

            // geonwoo
            // true : 1초에 3회 초과 채팅 메시지 연결 시도로 채팅이 30초 동안 제한됨
            if(limiter.allowSendMsgCheck()){
                sendingManage::instance()->sendMessage(chatViewName, messageCopy);
                qDebug() << "[Enter] 메시지 전송 시도: " << messageCopy;
            } else {
                // false : 채팅 제한 없음
                qDebug() << "채팅이 차단되어 있음";
            }
        }
    });

    //==========================
    //    지연 전송 (한글 조합 완료 대기)
    //==========================
    auto sendMessageDelayed = [this]() {
        QTimer::singleShot(50, this, [this]() {  // 50ms 지연 후 전송
            QString message = lineEdit->text();
            
            if (!message.isEmpty()) {
                lineEdit->clear();
                QString messageCopy = QString(message);

                // geonwoo
                // true : 1초에 3회 초과 채팅 메시지 연결 시도로 채팅이 30초 동안 제한됨
                if(limiter.allowSendMsgCheck()){
                    sendingManage::instance()->sendMessage(chatViewName, messageCopy);
                    qDebug() << "[지연] 메시지 전송 시도: " << messageCopy;
                } else {
                    // false : 채팅 제한 없음
                    qDebug() << "채팅이 차단되어 있음";
                }
            }
        });
    };

    //==========================
    //   버튼 클릭시 파일 - devwooms
    //==========================
    connect(pushButton, &QPushButton::pressed, this, [this](){
        QString filePath = QFileDialog::getOpenFileName(this, "파일 선택", "", "모든 파일 (*)");
        if (!filePath.isEmpty()) {
            qDebug() << "선택된 파일:" << filePath;
            
            // 파일 전송 확인
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "파일 전송",
                QString("선택된 파일을 전송하시겠습니까?\n%1").arg(QFileInfo(filePath).fileName()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
                );
            
            if (reply == QMessageBox::Yes) {
                QStringList filePaths;
                filePaths << filePath;
                sendingManage::instance()->sendFile(filePaths, chatViewName);
                qDebug() << "파일 전송 요청 완료";
            } else {
                qDebug() << "파일 전송 취소";
            }
        }
    });
    //==========================
    //   드래그 앤 드랍시 파일 - devwooms
    //==========================
    connect(this, &DropWidget::fileDropped, this, [this](const QStringList& filePaths) {
        qDebug() << "파일 드롭됨:" << filePaths;
        // sendingManage를 통해 파일 전송
        sendingManage::instance()->sendFile(filePaths, chatViewName);
        qDebug() << "파일 전송 요청 완료";
    });

    //==========================
    //   파일 다운로드 링크 클릭 처리
    //==========================
    connect(textBrowser, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
        qDebug() << "링크 클릭됨:" << url.toString();
        
        if (url.scheme() == "download") {
            QString fileId = url.host(); // download://fileId 에서 fileId 추출
            qDebug() << "파일 다운로드 요청:" << fileId;
            
            // 파일 저장 경로 선택
            QString fileName = QString("downloaded_file_%1").arg(fileId); // 기본 파일명
            QString savePath = QFileDialog::getSaveFileName(this, "파일 저장", fileName, "모든 파일 (*)");
            
            if (!savePath.isEmpty()) {
                qDebug() << "저장 경로 선택됨:" << savePath;
                // 서버에 파일 다운로드 요청
                requestFileDownload(fileId, savePath);
            } else {
                qDebug() << "파일 저장 취소됨";
            }
        }
    });

    //==========================
    //   파일 다운로드 응답 처리
    //==========================
    connect(&SocketManage::instance(), &SocketManage::fileDownloadReceived, this, [this](const QJsonObject& response) {
        qDebug() << "파일 다운로드 응답 받음";
        
        QString fileId = response["fileId"].toString();
        QString fileName = response["fileName"].toString();
        QString base64Data = response["fileData"].toString();
        bool success = response["success"].toBool();
        
        if (success && !base64Data.isEmpty()) {
            // Base64 데이터를 바이너리로 디코딩
            QByteArray fileData = QByteArray::fromBase64(base64Data.toUtf8());
            
            // 저장 경로 가져오기 (미리 저장해둔 경로 사용)
            QString savePath = pendingDownloads.value(fileId);
            
            if (!savePath.isEmpty() && !fileData.isEmpty()) {
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly)) {
                    qint64 writtenBytes = file.write(fileData);
                    file.close();
                    
                    if (writtenBytes == fileData.size()) {
                        qDebug() << "파일 다운로드 완료:" << savePath;
                        QMessageBox::information(this, "다운로드 완료", 
                                               QString("파일이 성공적으로 다운로드되었습니다.\n%1").arg(savePath));
                    } else {
                        qWarning() << "파일 쓰기 실패";
                        QMessageBox::warning(this, "다운로드 실패", "파일 저장 중 오류가 발생했습니다.");
                    }
                } else {
                    qWarning() << "파일 열기 실패:" << savePath;
                    QMessageBox::warning(this, "다운로드 실패", "파일을 저장할 수 없습니다.");
                }
                
                // 대기 중인 다운로드에서 제거
                pendingDownloads.remove(fileId);
            }
        } else {
            qWarning() << "파일 다운로드 실패:" << fileId;
            QMessageBox::warning(this, "다운로드 실패", "파일을 다운로드할 수 없습니다.");
            pendingDownloads.remove(fileId);
        }
    });
}

void ChattingRoomView::setupUI()
{
    // 메인 레이아웃
    QHBoxLayout *horizontalLayout_2 = new QHBoxLayout(this);
    horizontalLayout_2->setContentsMargins(12, 12, 12, 12);
    
    // 수직 레이아웃
    QVBoxLayout *verticalLayout = new QVBoxLayout();
    
    // 텍스트 브라우저
    textBrowser = new QTextBrowser();
    textBrowser->setOpenLinks(false);  // 링크 자동 열기 비활성화 (채팅 내용 유지)
    verticalLayout->addWidget(textBrowser);
    
    // 하단 수평 레이아웃
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    
    // 파일 추가 버튼
    pushButton = new QPushButton();
    pushButton->setMinimumSize(50, 50);
    pushButton->setMaximumSize(50, 50);
    pushButton->setIcon(QIcon(":/assets/assets/AddFile.png"));
    pushButton->setIconSize(QSize(50, 50));
    horizontalLayout->addWidget(pushButton);
    
    // 라인 에디트
    lineEdit = new QLineEdit();
    lineEdit->setMinimumHeight(50);
    horizontalLayout->addWidget(lineEdit);
    
    // 전송 버튼
    pushButton_2 = new QPushButton("PushButton");
    pushButton_2->setMinimumHeight(50);
    horizontalLayout->addWidget(pushButton_2);
    
    verticalLayout->addLayout(horizontalLayout);
    horizontalLayout_2->addLayout(verticalLayout);
}

ChattingRoomView::~ChattingRoomView()
{
}

void ChattingRoomView::requestFileDownload(const QString& fileId, const QString& savePath)
{
    qDebug() << "requestFileDownload 호출:" << fileId << savePath;
    
    // 대기 중인 다운로드 목록에 추가
    pendingDownloads.insert(fileId, savePath);
    
    // sendingManage를 통해 서버에 파일 다운로드 요청
    sendingManage::instance()->requestFileDownload(fileId);
    
    qDebug() << "파일 다운로드 요청 전송 완료:" << fileId;
}
