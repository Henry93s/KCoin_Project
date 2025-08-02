#include "clientHandler.h"
#include "servermanager.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include <QSslSocket>
#include <QTimer>
#include <QRandomGenerator>
#include <QFileInfo>
#include <QDir>


ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent), socket(socket) {
    usermanage = new userManage(this);
    postmanager = this->postmanager->instance();
    connect(socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
}

void ClientHandler::onDisconnected() {
    qDebug() << "클라이언트 연결 해제";
    emit disconnected(this);
    socket->deleteLater();
    this->deleteLater();
}

//==========================
//       받은 신호 처리
//==========================
void ClientHandler::onReadyRead() {
    qDebug() << "신호 받음";
    readBuffer.append(socket->readAll());

    // ServerManager::getInstance();

    while (readBuffer.contains('\n')) {
        int newlineIndex = readBuffer.indexOf('\n');
        QByteArray line = readBuffer.left(newlineIndex);
        readBuffer.remove(0, newlineIndex + 1);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError) {
            qDebug() << "클라이언트에서 받은 정보 Json 파싱 에러 발생";
            continue;
        }

        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString type = obj.value("type").toString();    // 신호의 type 읽어오기

            //==========================
            //       로그인 요청 처리
            //==========================
            if (type == "login") {
                readyRead_LoginRequest(obj);
                //==========================
                //       회원가입 처리
                //==========================
            } else if (type == "signup") {
                readyRead_SignRequest(obj);
                //==========================
                //       메세지 전송 처리
                //==========================
            } else if (type == "messagesend") {
                readyRead_MessageSendRequest(obj);
                //==========================
                //       파일 전송 처리
                //==========================
            } else if (type == "filesend") {
                readyRead_FileSend(obj);
                //==========================
                //       파일 다운로드 요청 처리
                //==========================
            } else if (type == "filedownload") {
                readyRead_FileDownload(obj);
                //==========================
                //       이메일 인증 요청 처리
            }
            //==========================
            //      채팅 로그 전송
            //==========================
            else if (type == "givemelog"){
                readyRead_GiveLog(obj);
            }
            //==========================
            //     매수/매도 처리
            //==========================
            else if(type == "trade"){
                readyRead_Trade(obj);
            }
            //==========================
            //       신고 처리
            //==========================
            else if (type == "report") {
                readyRead_Report(obj);
            }
            //==========================
            //      이메일 확인 처리
            //==========================
            else if (type == "emailcheck"){
                readyRead_EmailCheck(obj);
            }
            //==========================
            //      이메일 코드 확인 처리
            //==========================
            else if (type == "emailcodecheck"){
                readyRead_Emailcodecheck(obj);
            }

        }
    }
}

void ClientHandler::sendMessageToClient(QByteArray data) {
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(data);
        socket->flush();
    }
}

void ClientHandler::readyRead_LoginRequest(const QJsonObject& obj)
{
    QString ID = obj.value("ID").toString();
    QString PW = obj.value("PW").toString();
    QString nameOut;

    // usermanage로 로그인 시도(성공 시 nameOut에 이름 세팅)
    bool val = usermanage->signIn(ID, PW, nameOut);

    QJsonObject JsonResponse;
    JsonResponse["type"] = "response";
    JsonResponse["result"] = val ? "success" : "fail";
    JsonResponse["name"] = nameOut;

    if (val) {
        // 1) 로그인 성공 시 유저 정보 전체 내려주기
        QJsonObject userInfo = usermanage->getUserDetailByName(nameOut);
        JsonResponse["user"] = userInfo;
        JsonResponse["history"] = userInfo["tradingHis"];
    }

    // 응답 전송
    QJsonDocument respDoc(JsonResponse);
    QByteArray respData = respDoc.toJson(QJsonDocument::Compact);
    respData.append('\n');
    socket->write(respData);
}

void ClientHandler::readyRead_SignRequest(const QJsonObject &obj)
{
    QString name = obj.value("Name").toString();
    QString id = obj.value("ID").toString();
    QString pw = obj.value("PW").toString();
    QString phone = obj.value("Phone").toString();

    userInfo info;
    info.name = name;
    info.ID = id;
    info.PW = pw;
    info.phoneNum = phone;

    usermanage->signUp(info);
}

void ClientHandler::readyRead_MessageSendRequest(const QJsonObject &obj)
{
    qDebug() << "message 전달받음";
    QString senderName = obj.value("senderName").toString();
    QString chatViewName = obj.value("chatViewName").toString();
    QString text = obj.value("textMessage").toString();
    QString sendString = "[" + senderName + "] : " + text;

    if (usermanage->isBanned(senderName)) {
        // 금지 상태면 "yourebanned" 신호만 전송
        QJsonObject resp;
        resp["type"] = "yourebanned";
        QJsonDocument doc(resp);
        socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
        return;
    }
    QJsonObject sendObj;
    sendObj["type"] = "messagesend";
    sendObj["textMessage"] = sendString;
    sendObj["chatViewName"] = chatViewName;

    QJsonDocument sendDoc(sendObj);
    QByteArray sending = sendDoc.toJson(QJsonDocument::Compact);
    sending.append('\n');

    ServerManager::getInstance().broadcastMessage(sending);
    qDebug() << "클라이언트로 전송 요청 완료";

    // 전송 내용 파일로 저장, DB 폴더에 "채팅방 이름"의 json파일을 생성 및 열람
    QString chatPath = usermanage->getDBPath().replace("userInfo.json", chatViewName + ".json");
    QFile file(chatPath);

    // 파일이 없을 경우 생성
    if (!file.exists()) {
        if (file.open(QIODevice::WriteOnly)) {
            QJsonArray emptyArray;
            QJsonDocument doc(emptyArray);
            file.write(doc.toJson());
            file.close();
            qDebug() << chatViewName <<" Json 파일 생성";
        } else {
            qWarning() << "파일 생성 실패";
            return;
        }
    }

    // 저장할 내용 이어붙이기 위해 파일의 앞내용 읽어옴
    QJsonArray messageArray;                   // Json파일내용물 저장될 JsonArray
    if(file.open(QIODevice::ReadOnly)){
        QByteArray readData = file.readAll();
        QJsonDocument dataDoc = QJsonDocument::fromJson(readData);
        messageArray = dataDoc.array();
        file.close();
    }
    else{
        qWarning()<<"파일 읽기 실패";
        return;
    }

    messageArray.append(sendString);      // json Object를 userArray로
    if(file.open(QIODevice::WriteOnly)){
        QJsonDocument newDoc(messageArray);
        file.write(newDoc.toJson());
        file.close();
        qDebug()<<"메세지 저장 완료";
    }
}

void ClientHandler::readyRead_FileSend(const QJsonObject &obj)
{
    qDebug() << "파일 데이터 전달받음";

    // JSON에서 파일 메타데이터 및 실제 데이터 추출
    QString chatViewName = obj.value("chatViewName").toString();
    QString senderName = obj.value("senderName").toString();  // ← 전송자 이름 추출
    QString fileId = obj.value("fileId").toString();
    QString fileName = obj.value("fileName").toString();
    QString originalPath = obj.value("originalPath").toString();
    qint64 fileSize = obj.value("fileSize").toDouble();
    QString fileExtension = obj.value("fileExtension").toString();
    QString mimeType = obj.value("mimeType").toString();
    QString timestamp = obj.value("timestamp").toString();
    QString checksum = obj.value("checksum").toString();
    QString base64Data = obj.value("fileData").toString();  // ← Base64 데이터 추출

    qDebug() << "파일 정보:";
    qDebug() << "  - 전송자:" << senderName;
    qDebug() << "  - 파일ID:" << fileId;
    qDebug() << "  - 파일명:" << fileName;
    qDebug() << "  - 크기:" << fileSize << "bytes";
    qDebug() << "  - 타입:" << mimeType;
    qDebug() << "  - 채팅방:" << chatViewName;
    qDebug() << "  - Base64 데이터 크기:" << base64Data.size() << "characters";

    // Base64 데이터를 바이너리로 디코딩
    QByteArray fileData = QByteArray::fromBase64(base64Data.toUtf8());
    qDebug() << "Base64 디코딩 완료 - 디코딩된 크기:" << fileData.size() << "bytes";

    // 서버에 파일 저장 디렉토리 생성
    QString dbPath = usermanage->getDBPath();
    QFileInfo dbInfo(dbPath);
    QString filesDir = dbInfo.dir().absolutePath() + "/files";

    QDir dir;
    if (!dir.exists(filesDir)) {
        dir.mkpath(filesDir);
        qDebug() << "파일 저장 디렉토리 생성:" << filesDir;
    }

    // 서버에 저장할 파일 경로 생성 (fileId_원본파일명)
    QString serverFilePath = QString("%1/%2_%3").arg(filesDir).arg(fileId).arg(fileName);

    // 서버에 실제 파일 저장
    QFile serverFile(serverFilePath);
    if (serverFile.open(QIODevice::WriteOnly)) {
        qint64 writtenBytes = serverFile.write(fileData);
        serverFile.close();

        if (writtenBytes == fileData.size()) {
            qDebug() << "서버에 파일 저장 완료:" << serverFilePath;
            qDebug() << "저장된 파일 크기:" << writtenBytes << "bytes";
        } else {
            qWarning() << "파일 저장 불완전 - 예상:" << fileData.size() << "실제:" << writtenBytes;
        }
    } else {
        qWarning() << "서버에 파일 저장 실패:" << serverFilePath;
    }

    // 파일 메타데이터를 chatFiles.json에 저장
    QString chatFilesPath = usermanage->getDBPath().replace("userInfo.json", "chatFiles.json");
    QFile chatFilesFile(chatFilesPath);

    QJsonArray fileArray;

    // 기존 파일 데이터 읽기
    if (chatFilesFile.exists() && chatFilesFile.open(QIODevice::ReadOnly)) {
        QByteArray readData = chatFilesFile.readAll();
        QJsonDocument dataDoc = QJsonDocument::fromJson(readData);
        if (dataDoc.isArray()) {
            fileArray = dataDoc.array();
        }
        chatFilesFile.close();
    }

    // 새 파일 정보 객체 생성
    QJsonObject fileRecord;
    fileRecord["fileId"] = fileId;
    fileRecord["fileName"] = fileName;
    fileRecord["senderName"] = senderName;  // ← 전송자 이름 저장
    fileRecord["originalPath"] = originalPath;
    fileRecord["serverPath"] = serverFilePath;  // ← 서버 저장 경로 추가
    fileRecord["fileSize"] = fileSize;
    fileRecord["fileExtension"] = fileExtension;
    fileRecord["mimeType"] = mimeType;
    fileRecord["chatViewName"] = chatViewName;
    fileRecord["timestamp"] = timestamp;
    fileRecord["checksum"] = checksum;
    fileRecord["uploadTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // 배열에 추가
    fileArray.append(fileRecord);

    // 파일에 저장
    if (chatFilesFile.open(QIODevice::WriteOnly)) {
        QJsonDocument newDoc(fileArray);
        chatFilesFile.write(newDoc.toJson());
        chatFilesFile.close();
        qDebug() << "파일 메타데이터 저장 완료:" << fileName;
    } else {
        qWarning() << "chatFiles.json 저장 실패";
    }

    // 채팅 메시지로 파일 공유 알림 추가 (전송자 이름 포함)
    QString fileMessage = QString("<a href='download://%1'>[%2] [파일] %3 (%4 bytes) - 클릭하여 다운로드</a>")
                              .arg(fileId).arg(senderName).arg(fileName).arg(fileSize);
    QString chatPath = usermanage->getDBPath().replace("userInfo.json", chatViewName + ".json");
    QFile chatFile(chatPath);

    // 채팅 로그에 파일 메시지 추가
    QJsonArray messageArray;
    if (chatFile.exists() && chatFile.open(QIODevice::ReadOnly)) {
        QByteArray readData = chatFile.readAll();
        QJsonDocument dataDoc = QJsonDocument::fromJson(readData);
        if (dataDoc.isArray()) {
            messageArray = dataDoc.array();
        }
        chatFile.close();
    } else {
        // 파일이 없으면 새로 생성
        if (chatFile.open(QIODevice::WriteOnly)) {
            QJsonArray emptyArray;
            QJsonDocument doc(emptyArray);
            chatFile.write(doc.toJson());
            chatFile.close();
        }
    }

    // 파일 메시지를 채팅 로그에 추가
    messageArray.append(fileMessage);
    if (chatFile.open(QIODevice::WriteOnly)) {
        QJsonDocument newDoc(messageArray);
        chatFile.write(newDoc.toJson());
        chatFile.close();
        qDebug() << "채팅 로그에 파일 메시지 추가 완료";
    }

    // 다른 클라이언트들에게 파일 공유 알림 브로드캐스트
    QJsonObject broadcastObj;
    broadcastObj["type"] = "messagesend";
    broadcastObj["textMessage"] = fileMessage;
    broadcastObj["chatViewName"] = chatViewName;
    broadcastObj["fileId"] = fileId;  // 파일 다운로드용 ID 포함

    QJsonDocument broadcastDoc(broadcastObj);
    QByteArray broadcastData = broadcastDoc.toJson(QJsonDocument::Compact);
    broadcastData.append('\n');

    ServerManager::getInstance().broadcastMessage(broadcastData);
    qDebug() << "파일 업로드 완료 알림 브로드캐스트 완료:" << senderName;
}

void ClientHandler::readyRead_FileDownload(const QJsonObject &obj)
{
    qDebug() << "파일 다운로드 요청 받음";

    QString fileId = obj.value("fileId").toString();
    QString requesterName = obj.value("requesterName").toString();

    qDebug() << "요청자:" << requesterName << "| 파일ID:" << fileId;

    // chatFiles.json에서 파일 정보 찾기
    QString chatFilesPath = usermanage->getDBPath().replace("userInfo.json", "chatFiles.json");
    QFile chatFilesFile(chatFilesPath);

    QJsonObject fileInfo;
    bool fileFound = false;

    if (chatFilesFile.exists() && chatFilesFile.open(QIODevice::ReadOnly)) {
        QByteArray readData = chatFilesFile.readAll();
        QJsonDocument dataDoc = QJsonDocument::fromJson(readData);

        if (dataDoc.isArray()) {
            QJsonArray fileArray = dataDoc.array();

            // 파일 ID로 파일 정보 검색
            for (const QJsonValue& value : fileArray) {
                QJsonObject file = value.toObject();
                if (file["fileId"].toString() == fileId) {
                    fileInfo = file;
                    fileFound = true;
                    break;
                }
            }
        }
        chatFilesFile.close();
    }

    QJsonObject response;
    response["type"] = "filedownload";
    response["fileId"] = fileId;

    if (fileFound) {
        QString serverPath = fileInfo["serverPath"].toString();
        QString fileName = fileInfo["fileName"].toString();

        // 서버에서 파일 읽기
        QFile serverFile(serverPath);
        if (serverFile.exists() && serverFile.open(QIODevice::ReadOnly)) {
            QByteArray fileData = serverFile.readAll();
            serverFile.close();

            // Base64로 인코딩
            QString base64Data = fileData.toBase64();

            response["success"] = true;
            response["fileName"] = fileName;
            response["fileData"] = base64Data;
            response["fileSize"] = fileData.size();

            qDebug() << "파일 다운로드 준비 완료:" << fileName << "(" << fileData.size() << "bytes)";

        } else {
            qWarning() << "서버 파일 읽기 실패:" << serverPath;
            response["success"] = false;
            response["error"] = "파일을 읽을 수 없습니다";
        }
    } else {
        qWarning() << "파일 ID를 찾을 수 없음:" << fileId;
        response["success"] = false;
        response["error"] = "파일을 찾을 수 없습니다";
    }

    // 요청한 클라이언트에게만 응답 전송
    QJsonDocument responseDoc(response);
    QByteArray responseData = responseDoc.toJson(QJsonDocument::Compact);
    responseData.append('\n');

    socket->write(responseData);
    socket->flush();
    qDebug() << "파일 다운로드 응답 전송 완료";
}

void ClientHandler::readyRead_GiveLog(const QJsonObject &obj)
{
    QString chatViewName = obj.value("chatViewName").toString();
    // 폴더에 있는 파일 읽어옴
    QString chatLogPath = usermanage->getDBPath().replace("userInfo.json", chatViewName + ".json");
    QFile file(chatLogPath);
    QJsonObject JsonResponse;
    JsonResponse["type"] = "messagelog";

    // 파일이 없을 경우 로그 없음 전송
    if (!file.exists()) {
        JsonResponse["exist"] = "no";   // 존재여부 no
        QJsonDocument respDoc(JsonResponse);
        QByteArray respData = respDoc.toJson(QJsonDocument::Compact);
        respData.append('\n');
        socket->write(respData);
    } else{     // 파일이 있을 경우 파일 전부 읽어와 내용물 전송
        file.open(QIODevice::ReadOnly);
        // 읽어오는데에 쓰는 변수
        QByteArray readData = file.readAll();
        QJsonDocument dataDoc = QJsonDocument::fromJson(readData);
        QJsonArray messageArray = dataDoc.array();
        JsonResponse["exist"] = "yes";
        JsonResponse["log"] = messageArray;
        file.close();
        // 전송하는데에 쓰는 변수
        QJsonDocument sendingDoc(JsonResponse);
        QByteArray sendingData = sendingDoc.toJson(QJsonDocument::Compact);
        sendingData.append('\n');
        socket->write(sendingData);
    }
}

void ClientHandler::readyRead_Trade(const QJsonObject &obj)
{
    QString userInfoPath = usermanage->getDBPath();
    QFile file(userInfoPath);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug()<<"거래 신호 수신, 유저 정보 읽기 실패";
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray userList = doc.array();

    QString action = obj.value("action").toString();
    QString coin = obj.value("coin").toString();
    double price = obj.value("price").toDouble();
    int amount = obj.value("amount").toInt();
    QString senderName = obj.value("senderName").toString();

    bool updated = false;
    QJsonObject resultObj; // 응답용

    for(int i = 0; i < userList.size(); ++i){
        QJsonObject userObj = userList[i].toObject();
        if(userObj["name"].toString() == senderName){
            QJsonObject coins = userObj["coins"].toObject();
            double money = userObj["money"].toDouble();
            double payment = userObj["payment"].toDouble();

            int currentCoinCnt = coins.value(coin).toInt();

            // 신규 코인 거래 대응
            if(!coins.contains(coin))
                coins[coin] = 0;

            if(action == "buy"){
                double totalCost = price * amount;
                if(money >= totalCost){
                    money -= totalCost;
                    payment += totalCost;
                    coins[coin] = currentCoinCnt + amount;
                    userObj["money"] = money;
                    userObj["coins"] = coins;
                    userObj["payment"] = payment;
                    updated = true;
                    qDebug()<<"매수 성공";
                } else {
                    qDebug()<<"매수 실패: 잔액 부족";
                }
            }
            else if(action == "sell"){
                if(currentCoinCnt >= amount){
                    money += price * amount;
                    payment -= price * amount;
                    coins[coin] = currentCoinCnt - amount;
                    userObj["money"] = money;
                    userObj["coins"] = coins;
                    userObj["payment"] = payment;
                    updated = true;
                    qDebug()<<"매도 성공";
                } else {
                    qDebug()<<"매도 실패: 코인 부족";
                }
            }

            // 거래내역 기록
            QJsonArray tradingHis = userObj["tradingHis"].toArray();
            QJsonObject record;
            record["action"] = action;
            record["coin"] = coin;
            record["price"] = price;
            record["amount"] = amount;
            record["datetime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            tradingHis.append(record);
            userObj["tradingHis"] = tradingHis;

            userList[i] = userObj;
            resultObj = userObj; // 응답으로 현재 유저정보를 그대로 내려주고 싶다면
            break;
        }
    }

    // 변경 사항 저장
    if(updated){
        if(file.open(QIODevice::WriteOnly)){
            QJsonDocument newDoc(userList);
            file.write(newDoc.toJson());
            file.close();
        }
    }

    // 거래 응답 전송
    QJsonObject resp;
    resp["type"] = "traderesponse";
    resp["result"] = updated ? "success" : "fail";
    resp["action"] = action;
    resp["coin"] = coin;
    resp["amount"] = amount;
    resp["history"] = resultObj["tradingHis"];

    // [수정된 부분] user 객체 통째로 넘기는 대신 핵심 정보만 직접 넘김
    resp["money"] = resultObj["money"];
    resp["payment"] = resultObj["payment"];
    resp["coins"] = resultObj["coins"];
    // resp["user"] = resultObj; // 이 라인은 이제 필요 없어!

    QJsonDocument respDoc(resp);
    QByteArray respData = respDoc.toJson(QJsonDocument::Compact);
    respData.append('\n');
    socket->write(respData);
    qDebug() << "거래 응답 전송";
}

void ClientHandler::readyRead_Report(const QJsonObject &obj)
{
    QString targetName = obj.value("targetName").toString();
    QString reason = obj.value("reason").toString();

    // 신고 처리 (userManage에 함수 분리 추천)
    usermanage->increaseReport(targetName, reason);

    // 필요시 신고 누적 결과 응답
    bool isBanned = usermanage->isBanned(targetName);
    QJsonObject resp;
    resp["type"] = "reportresult";
    resp["result"] = "ok";
    resp["targetBanned"] = isBanned;
    socket->write(QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n");
}

void ClientHandler::readyRead_EmailCheck(const QJsonObject &obj)
{
    qDebug() << "server emailcheck";
    QString email = obj.value("email").toString();
    QSslSocket *sslSocket = new QSslSocket(this);

    connect(sslSocket, &QSslSocket::encrypted, [=]() {
        qDebug() << "✓ Gmail SSL 연결 성공!";

        QString myEmail = "woomstest@gmail.com";
        QString myPassword = "tpxzttfhztaawewm";

        savedCode = QString::number(QRandomGenerator::global()->bounded(100000, 999999));

        QStringList commands;
            // 누군지 확인
        commands << "EHLO localhost"
                 << "AUTH LOGIN"
                 << myEmail.toUtf8().toBase64()
                 << myPassword.toUtf8().toBase64()
                 // 송신자
                 << QString("MAIL FROM:<%1>").arg(myEmail)
                 // 수신자
                 << QString("RCPT TO:<%1>").arg(email)
                 // 메일 내용
                 << "DATA"
                 << QString("Subject: 인증코드\r\n\r\n인증코드: %1\r\n.").arg(savedCode);

        int step = 0;
        QTimer *timer = new QTimer();

        connect(timer, &QTimer::timeout, [=]() mutable {
            if (step < commands.size()) {
                sslSocket->write((commands[step] + "\r\n").toUtf8());
                sslSocket->flush();
                qDebug() << "Step" << step << ":" << commands[step];
                step++;
            } else {
                timer->stop();
                timer->deleteLater();

                // 메일로 더 이상 보낼것이 없다고 알려줌
                sslSocket->write("QUIT\r\n");
                sslSocket->flush();
                qDebug() << "QUIT 전송";
                qDebug() << "✓ 이메일 발송 완료!";

                // 즉시 모든 시그널 연결 해제 후 삭제
                sslSocket->disconnect(); // 모든 시그널 연결 해제
                sslSocket->abort();      // 강제 연결 종료
                sslSocket->deleteLater(); // 한 번만 삭제
            }
        });
        timer->start(500);
    });

    // SSL 에러만 처리 (연결/에러 시그널은 제거)
    connect(sslSocket, &QSslSocket::sslErrors, [sslSocket]() {
        qDebug()<<"QSslSocket::sslErrors : "<<sslSocket;
        // 이걸 하게 되면 보안 취약
        // socket->ignoreSslErrors();
    });

    qDebug() << "🔒 Gmail SSL 연결 시도...";
    sslSocket->connectToHostEncrypted("smtp.gmail.com", 465);
}

void ClientHandler::readyRead_Emailcodecheck(const QJsonObject &obj)
{
    QString code = obj.value("code").toString();
    QJsonObject resp;
    if (code == savedCode){
        resp["type"] = "emailtrue";
        QJsonDocument respDoc(resp);
        QByteArray respData = respDoc.toJson(QJsonDocument::Compact);
        respData.append('\n');
        socket->write(respData);
        qDebug() << "이메일 코드 성공";
    } else {
        resp["type"] = "emailfalse";
        QJsonDocument respDoc(resp);
        QByteArray respData = respDoc.toJson(QJsonDocument::Compact);
        respData.append('\n');
        socket->write(respData);
        qDebug() << "이메일 코드 실패";
    }
}

