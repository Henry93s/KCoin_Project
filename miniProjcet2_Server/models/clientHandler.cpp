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
            // geonwoo
            // 게시판 글 write 처리
            else if (type == "postWrite"){
                readyRead_sendPostWrite(obj);
            }
            // 글 read
            else if (type == "postRead"){
                readyRead_sendPostRead(obj);
            }
            // geonwoo
            // 글 delete
            else if (type == "postDelete"){
                readyRead_sendPostDelete(obj);
            }
            // 글 all read
            else if (type == "postAllRead"){
                readyRead_sendPostAllRead(obj);
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
    QString senderID = obj.value("senderID").toString();
    QString sendString = senderID + "[" + senderName + "] : " + text;

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

    // 이전 : 전송 내용 파일로 저장, DB 폴더에 "채팅방 이름"의 json파일을 생성 및 열람
    // 이후 : geonwoo : 채팅 전송 내용 채팅 로그 db table 에 저장
    // 채팅 로그 테이블 찾기
    bool roomFindQ_isSuccess;
    QSqlQuery roomFindQuery = emit requestQuery(QString("SELECT TABLE_NAME FROM \
        information_schema.TABLES WHERE TABLE_SCHEMA = 'coin' \
        AND TABLE_NAME = '%1'").arg(chatViewName), roomFindQ_isSuccess);
    if(roomFindQ_isSuccess && roomFindQuery.next()){
        qDebug() << "find Room";

    } else {
        qDebug() << "not find room";
        // 채팅 로그 테이블 생성
        bool roomCreate_isSuccess;
        QSqlQuery roomCreateQuery = emit requestQuery( \
            QString("CREATE TABLE %1 (\
                    id INT AUTO_INCREMENT PRIMARY KEY,\
                    message TEXT NOT NULL,\
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\
                    )").arg(chatViewName), roomCreate_isSuccess);
        if(roomCreate_isSuccess){
            qDebug() << chatViewName << " 채팅 로그 테이블 생성 완료";
        } else {
            qWarning() << chatViewName << " 채팅 로그 테이블 생성 실패";
            return;
        }
    }

    // geonwoo : 채팅 로그 테이블에 새 메시지 추가
    // 채팅 로그 테이블에 메시지 INSERT
    bool roomChatInsert_isSuccess;
    // geonwoo : FIX: 채팅 메시지에 SQL 인젝션을 방지하기 위한 bind query 적용
    // cf. SQL 인젝션 : 사용자가 입력한 값이 SQL 쿼리 문자열에 그대로 삽입되어 악의적인 쿼리가 실행되는 공격
    QSqlQuery query;
    QString sqlText = QString("INSERT INTO %1 (message) VALUES (?)").arg(chatViewName);
    query.prepare(sqlText);
    query.addBindValue(sendString);
    QSqlQuery roomChatInsertQuery = emit requestBindQuery(query, roomChatInsert_isSuccess);
    if(roomChatInsert_isSuccess){
        qDebug() << chatViewName << " 에 message 삽입 추가 완료";
    } else {
        qDebug() << chatViewName << " 에 message 삽입 실패  " << roomChatInsertQuery.lastError().text();
    }
}

void ClientHandler::readyRead_FileSend(const QJsonObject &obj)
{
    qDebug() << "파일 데이터 전달받음";

    // JSON에서 파일 메타데이터 및 실제 데이터 추출
    QString chatViewName = obj.value("chatViewName").toString();
    QString senderName = obj.value("senderName").toString();  // ← 전송자 이름 추출
    QString senderID = obj.value("senderID").toString(); // <- 전송자 ID 추가 geonwoo
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
    qDebug() << "  - 전송자 ID: " << senderID;
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

    // 이전 : 파일 메타데이터를 chatFiles.json에 저장
    // 이후 : geonwoo : 파일 메타데이터를 DB 테이블에 저장
    bool fileMetaTableFind_isSuccess;
    QSqlQuery fileMetaTableFindQuery = emit requestQuery(QString(\
            "SELECT TABLE_NAME FROM \
        information_schema.TABLES WHERE TABLE_SCHEMA = 'coin' \
        AND TABLE_NAME = 'chatFiles'"), fileMetaTableFind_isSuccess);
    if(!fileMetaTableFind_isSuccess){
        qDebug() << "chatFiles 파일 메타데이터 테이블 찾기 쿼리 동작 실패";
        return;
    }

    // 이전 : 배열에 추가
    // 이후 : geonwoo : 파일 메타데이터 테이블에 추가
    bool fileMetaTableInsert_isSuccess;
    QSqlQuery query;
    query.prepare("INSERT INTO chatFiles (chatViewName, checksum, fileExtension, fileId, fileName, fileSize, mimeType, originalPath, senderName, senderID, serverPath, timestamp, uploadTime) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(chatViewName);
    query.addBindValue(checksum);
    query.addBindValue(fileExtension);
    query.addBindValue(fileId);
    query.addBindValue(fileName);
    query.addBindValue(fileSize);
    query.addBindValue(mimeType);
    query.addBindValue(originalPath);
    query.addBindValue(senderName);
    query.addBindValue(senderID);
    query.addBindValue(serverFilePath);
    query.addBindValue(timestamp);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    QSqlQuery fileMetaTableInsertQuery = emit requestBindQuery(query, fileMetaTableInsert_isSuccess);
    if(!fileMetaTableInsert_isSuccess){
        qDebug() << "fileMetaTableInsert 처리 에러 : " << fileMetaTableInsertQuery.lastError().text();
    } else {
        qDebug() << "fileMetaTableInsert 처리 완료";
    }

    // 채팅 메시지로 파일 공유 알림 추가 (전송자 이름 포함)
    QString fileMessage = QString("<a href='download://%1'>%2[%3] [파일] %4 (%5 bytes) - 클릭하여 다운로드</a>")
                              .arg(fileId).arg(senderID).arg(senderName).arg(fileName).arg(fileSize);

    // 이전 : 채팅 로그에 파일 메시지 추가
    // 이후 : geonwoo : 채팅 로그 테이블에 파일 메시지를 추가
    // 1. 먼저 채팅 로그 테이블 찾기
    bool roomFindQ_isSuccess;
    QSqlQuery roomFindQuery = emit requestQuery(QString("SELECT TABLE_NAME FROM \
        information_schema.TABLES WHERE TABLE_SCHEMA = 'coin' \
        AND TABLE_NAME = '%1'").arg(chatViewName), roomFindQ_isSuccess);
    if(roomFindQ_isSuccess && roomFindQuery.next()){
        qDebug() << "find Room";

    } else {
        qDebug() << "not find room";
        // 채팅 로그 테이블 생성
        bool roomCreate_isSuccess;
        QSqlQuery roomCreateQuery = emit requestQuery( \
            QString("CREATE TABLE %1 (\
                    id INT AUTO_INCREMENT PRIMARY KEY,\
                    message TEXT NOT NULL,\
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\
                    )").arg(chatViewName), roomCreate_isSuccess);
        if(roomCreate_isSuccess){
            qDebug() << chatViewName << " 채팅 로그 테이블 생성 완료";
        } else {
            qWarning() << roomCreateQuery.lastError().text();
        }
    }

    // 2. 채팅 로그 테이블에 파일 메시지를 추가
    bool roomChatInsert_isSuccess;
    QSqlQuery query2;
    QString sqlText = QString("INSERT INTO %1 (message) VALUES (?)").arg(chatViewName);
    query2.prepare(sqlText);
    query2.addBindValue(fileMessage);
    QSqlQuery roomChatInsertQuery = emit requestBindQuery(query2, roomChatInsert_isSuccess);
    if(roomChatInsert_isSuccess){
        qDebug() << chatViewName << " 에 file msg 삽입 추가 완료";
    } else {
        qDebug() << roomChatInsertQuery.lastError().text();
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
    qDebug() << "파일 업로드 완료 알림 브로드캐스트 완료: ID: " << senderID << " name: " << senderName;
}

void ClientHandler::readyRead_FileDownload(const QJsonObject &obj)
{
    qDebug() << "파일 다운로드 요청 받음";

    QString fileId = obj.value("fileId").toString();
    QString requesterName = obj.value("requesterName").toString();

    qDebug() << "요청자:" << requesterName << "| 파일ID:" << fileId;

    QJsonObject response;
    response["type"] = "filedownload";
    response["fileId"] = fileId;

    // 이전 : chatFiles.json에서 파일 정보 찾기
    // 이후 : geonwoo : 채팅 로그 테이블에서 다운로드 요청 파일 찾기
    bool chatFileDownQuery_isSuccess;
    QString serverPath;
    QString fileName;
    QSqlQuery chatFileDownQuery = emit requestQuery(QString(\
        "SELECT serverPath, fileName FROM \
        chatFiles WHERE fileId = '%1'").arg(fileId), chatFileDownQuery_isSuccess);
    if(chatFileDownQuery_isSuccess && chatFileDownQuery.next()){
        serverPath = chatFileDownQuery.value(0).toString();
        fileName = chatFileDownQuery.value(1).toString();
        qDebug() << "파일 찾기 성공 ! serverPath : " << serverPath << "  fileName: " << fileName;
    } else {
        qDebug() << "chatFiles 테이블에서 요청한 파일 찾기 쿼리 동작 실패 또는 결과 없음";
        response["error"] = "파일을 찾을 수 없습니다";
    }

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

        qDebug() << "파일 다운로드 준비 완료!!! :" << fileName << "(" << fileData.size() << "bytes)";

    } else {
        qWarning() << "서버 파일 읽기 실패!!! :" << serverPath;
        response["success"] = false;
        response["error"] = "파일을 읽을 수 없습니다!!!";
    }

    // 요청한 클라이언트에게만 응답 전송
    QJsonDocument responseDoc(response);
    QByteArray responseData = responseDoc.toJson(QJsonDocument::Compact);
    responseData.append('\n');

    socket->write(responseData);
    socket->flush();
    qDebug() << "파일 다운로드 응답 전송 완료!!!";
}

void ClientHandler::readyRead_GiveLog(const QJsonObject &obj)
{
    QString chatViewName = obj.value("chatViewName").toString();
    QJsonObject JsonResponse;
    JsonResponse["type"] = "messagelog";

    // 이전 : 폴더에 있는 파일 읽어옴
    // 이후 : geonwoo : db 에서 채팅로그를 읽어옴
    bool roomChatRead_isSucccess;
    QSqlQuery roomChatReadQuery = emit requestQuery(\
        QString("SELECT * FROM %1").arg(chatViewName)\
        , roomChatRead_isSucccess);
    if(roomChatRead_isSucccess){
        qDebug() << chatViewName << " 채팅 로그 테이블을 정상적으로 Read 했습니다.";
    } else {
        qDebug() << chatViewName << " 채팅 로그 테이블 Read 실패";
        return;
    }

    QJsonArray messageArray;
    while(roomChatReadQuery.next()){
        QString message = roomChatReadQuery.value("message").toString();
        messageArray.append(message);
    }

    JsonResponse["exist"] = "yes";
    JsonResponse["log"] = messageArray;

    // 전송하는데에 쓰는 변수
    QJsonDocument sendingDoc(JsonResponse);
    QByteArray sendingData = sendingDoc.toJson(QJsonDocument::Compact);
    sendingData.append('\n');
    socket->write(sendingData);
}

void ClientHandler::readyRead_Trade(const QJsonObject &obj)
{


    QString action = obj.value("action").toString();
    QString coin = obj.value("coin").toString();
    double price = obj.value("price").toDouble();
    int amount = obj.value("amount").toInt();
    QString senderName = obj.value("senderName").toString();

    bool updated = false;
    QJsonObject resultObj; // 응답용

    bool isSuccess;
    QSqlQuery query = emit requestQuery(QString("SELECT ID, password, money, name, payment, phoneNum FROM coin.`User`"), isSuccess);
    if(!isSuccess){
        qDebug() << query.lastError();
    }

    while (query.next()) {
        QString listID = query.value(0).toString();
        QString listPWD = query.value(1).toString();
        double listMoney = query.value(2).toDouble();
        QString listName = query.value(3).toString();
        double listPayment = query.value(4).toDouble();

        if(listName == senderName){
            bool isSuccess;
            QSqlQuery coin_userHasQuery = emit requestQuery(QString("SELECT CoinID, amount FROM Coin_UserHas WHERE UserID = '%1'").arg(listID), isSuccess);
            if(!isSuccess){
                qDebug() << coin_userHasQuery.lastError();
            }

            double totalCost = price * amount;

            QString coinID;
            int currentCoinCnt;
            int afterCoinCnt;
            bool thereIsNoCoin = true;
            while(coin_userHasQuery.next()){
                if(coin_userHasQuery.value(0).toString() == coin) {
                    coinID = coin_userHasQuery.value(0).toString();
                    currentCoinCnt = coin_userHasQuery.value(1).toInt();
                    afterCoinCnt = currentCoinCnt;
                    thereIsNoCoin = false;
                    break;
                }
            }
            // 신규 코인 거래 대응
            if(thereIsNoCoin){
                bool isSuccess_noCoin;
                QSqlQuery coin_newCoinQuery = emit requestQuery(QString("INSERT INTO coin.Coin_UserHas(CoinID, UserID, amount) VALUES('%1', '%2', 0);").arg(coin).arg(listID), isSuccess_noCoin);
                if(!isSuccess_noCoin){
                    qDebug() << coin_newCoinQuery.lastError();
                }
                else{
                    coinID = coin;
                    currentCoinCnt = 0;
                    afterCoinCnt = currentCoinCnt;
                }
            }

            if(action == "buy"){
                if(listMoney >= totalCost){
                    listMoney -= totalCost;
                    listPayment += totalCost;
                    afterCoinCnt += amount;
                    updated = true;
                    qDebug() << "매수 성공";
                } else{
                    qDebug() << "매수 실패: 잔액 부족";
                }
            }
            else if(action == "sell"){
                if(currentCoinCnt >= amount){
                    listMoney += totalCost;
                    listPayment -= totalCost;
                    afterCoinCnt -= amount;
                    updated = true;
                    qDebug() << "매도 성동";
                }
                else{
                    qDebug() << "매도 실패: 코인 부족";
                }
            }

            // User's Trading INSERT
            bool isSuccess_trading;
            QSqlQuery tradingOfUserHasQuery = emit requestQuery(QString("INSERT INTO coin.tradingOfUser(traderID, `action`, amount, coinID, tradedTime, price) VALUES('%1', '%2', %3, '%4', '%5', %6);").arg(listID).arg(action).arg(amount).arg(coinID).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")).arg(price), isSuccess_trading);
            if(!isSuccess_trading){
                qDebug() << "trading INSERT Query Fail!";
                qDebug() << tradingOfUserHasQuery.lastError();
            }

            // Coin_UserHas Update
            bool isSuccess_CoinUserHas;
            QSqlQuery CoinUserHasQuery = emit requestQuery(QString("UPDATE coin.Coin_UserHas SET amount=%1 WHERE CoinID='%2' AND UserID='%3';").arg(afterCoinCnt).arg(coinID).arg(listID), isSuccess_CoinUserHas);
            if(!isSuccess_CoinUserHas){
                qDebug() << "Coin User Has INSERT Query Fail!";
                qDebug() << CoinUserHasQuery.lastError();
            }

            // User update
            bool isSuccess_UserUpdate;
            QSqlQuery userQuery = emit requestQuery(QString("UPDATE coin.`User` SET money=%1, payment=%2 WHERE ID='%3';").arg(listMoney).arg(listPayment).arg(listID), isSuccess_UserUpdate);
            if(!isSuccess_UserUpdate){
                qDebug() << "User Update Query Fail!";
                qDebug() << userQuery.lastError();
            }

            break;
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

    return;

    // QString userInfoPath = usermanage->getDBPath();
    // QFile file(userInfoPath);
    // if(!file.open(QIODevice::ReadOnly)){
    //     qDebug()<<"거래 신호 수신, 유저 정보 읽기 실패";
    //     return;
    // }
    // QByteArray data = file.readAll();
    // file.close();
    // QJsonDocument doc = QJsonDocument::fromJson(data);
    // QJsonArray userList = doc.array();

    // QString action = obj.value("action").toString();
    // QString coin = obj.value("coin").toString();
    // double price = obj.value("price").toDouble();
    // int amount = obj.value("amount").toInt();
    // QString senderName = obj.value("senderName").toString();

    // bool updated = false;
    // QJsonObject resultObj; // 응답용

    // for(int i = 0; i < userList.size(); ++i){
    //     QJsonObject userObj = userList[i].toObject();
    //     if(userObj["name"].toString() == senderName){
    //         QJsonObject coins = userObj["coins"].toObject();
    //         double money = userObj["money"].toDouble();
    //         double payment = userObj["payment"].toDouble();

    //         int currentCoinCnt = coins.value(coin).toInt();

    //         // 신규 코인 거래 대응
    //         if(!coins.contains(coin))
    //             coins[coin] = 0;

    //         if(action == "buy"){
    //             double totalCost = price * amount;
    //             if(money >= totalCost){
    //                 money -= totalCost;
    //                 payment += totalCost;
    //                 coins[coin] = currentCoinCnt + amount;
    //                 userObj["money"] = money;
    //                 userObj["coins"] = coins;
    //                 userObj["payment"] = payment;
    //                 updated = true;
    //                 qDebug()<<"매수 성공";
    //             } else {
    //                 qDebug()<<"매수 실패: 잔액 부족";
    //             }
    //         }
    //         else if(action == "sell"){
    //             if(currentCoinCnt >= amount){
    //                 money += price * amount;
    //                 payment -= price * amount;
    //                 coins[coin] = currentCoinCnt - amount;
    //                 userObj["money"] = money;
    //                 userObj["coins"] = coins;
    //                 userObj["payment"] = payment;
    //                 updated = true;
    //                 qDebug()<<"매도 성공";
    //             } else {
    //                 qDebug()<<"매도 실패: 코인 부족";
    //             }
    //         }

    //         // 거래내역 기록
    //         QJsonArray tradingHis = userObj["tradingHis"].toArray();
    //         QJsonObject record;
    //         record["action"] = action;
    //         record["coin"] = coin;
    //         record["price"] = price;
    //         record["amount"] = amount;
    //         record["datetime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    //         tradingHis.append(record);
    //         userObj["tradingHis"] = tradingHis;

    //         userList[i] = userObj;
    //         resultObj = userObj; // 응답으로 현재 유저정보를 그대로 내려주고 싶다면
    //         break;
    //     }
    // }

    // // 변경 사항 저장
    // if(updated){
    //     if(file.open(QIODevice::WriteOnly)){
    //         QJsonDocument newDoc(userList);
    //         file.write(newDoc.toJson());
    //         file.close();
    //     }
    // }

    // // 거래 응답 전송
    // QJsonObject resp;
    // resp["type"] = "traderesponse";
    // resp["result"] = updated ? "success" : "fail";
    // resp["action"] = action;
    // resp["coin"] = coin;
    // resp["amount"] = amount;
    // resp["history"] = resultObj["tradingHis"];

    // // [수정된 부분] user 객체 통째로 넘기는 대신 핵심 정보만 직접 넘김
    // resp["money"] = resultObj["money"];
    // resp["payment"] = resultObj["payment"];
    // resp["coins"] = resultObj["coins"];
    // // resp["user"] = resultObj; // 이 라인은 이제 필요 없어!

    // QJsonDocument respDoc(resp);
    // QByteArray respData = respDoc.toJson(QJsonDocument::Compact);
    // respData.append('\n');
    // socket->write(respData);
    // qDebug() << "거래 응답 전송";
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

// geonwoo
// 게시판 글 write 처리
void ClientHandler::readyRead_sendPostWrite(const QJsonObject &obj){
    qDebug() << "게시판 글 추가 요청 받음";

    QString ID = obj.value("ID").toString();
    QString title = obj.value("title").toString();
    QString contents = obj.value("contents").toString();

    qDebug() << "글 추가 요청 user ID : " << ID;
    qDebug() << "글 타이틀 : " << title;
    qDebug() << "글 내용 : " << contents;

    QJsonObject response;

    bool sendPostWrite_isSuccess;
    QSqlQuery query;
    QString sqlText = QString("INSERT INTO Post (userID, title, contents) VALUES (?, ?, ?)");
    query.prepare(sqlText);
    query.addBindValue(ID);
    query.addBindValue(title);
    query.addBindValue(contents);
    QSqlQuery sendPostWriteQuery = emit requestBindQuery(query, sendPostWrite_isSuccess);
    if(sendPostWrite_isSuccess){
        qDebug() << "post 삽입 추가 완료";
        response["success"] = true;
    } else {
        qDebug() << "post 삽입 실패  " << sendPostWriteQuery.lastError().text();
        response["success"] = false;
    }

    int postID = sendPostWriteQuery.lastInsertId().toInt();
    response["type"] = "postWrite";
    response["title"] = title;
    response["contents"] = contents;
    response["postID"] = postID;
    response["userID"] = ID;

    // 요청한 클라이언트에게만 응답 전송
    QJsonDocument responseDoc(response);
    QByteArray responseData = responseDoc.toJson(QJsonDocument::Compact);
    responseData.append('\n');

    socket->write(responseData);
    socket->flush();
    qDebug() << "게시판 글 추가 처리 응답 전송 완료!!!";
}
void ClientHandler::readyRead_sendPostRead(const QJsonObject &obj){
    QString IDString = obj["ID"].toString();
    QJsonObject resp;

    resp["type"] = "postRead";

    QJsonArray postArray;

    bool isSuccess;
    auto query = emit requestQuery(QString("SELECT postID, userID, title, contents FROM coin.Post WHERE postID = %1;").arg(IDString), isSuccess);
    if(!isSuccess){
        qDebug() << "Failed to query sendPostAllRead";
    }
    QJsonObject post;
    post["userID"] = query.value(1).toString();
    post["title"] = query.value(2).toString();
    post["contents"] = query.value(3).toString();
    resp["post"] = post;

    QJsonDocument respDoc(resp);
    QByteArray respData = respDoc.toJson(QJsonDocument::Compact);
    respData.append('\n');
    socket->write(respData);
    qDebug() << "포스트 송출 성공";
}

void ClientHandler::readyRead_sendPostAllRead(const QJsonObject &obj)
{
    QJsonObject resp;

    resp["type"] = "postAllRead";

    QJsonArray postArray;

    bool isSuccess;
    auto query = emit requestQuery(QString("SELECT postID, userID, title, contents FROM coin.Post;"), isSuccess);
    if(!isSuccess){
        qDebug() << "Failed to query sendPostAllRead";
    }
    while(query.next()){
        QJsonObject perPost;
        perPost["postID"] = query.value(0).toString();
        perPost["userID"] = query.value(1).toString();
        perPost["title"] = query.value(2).toString();
        perPost["contents"] = query.value(3).toString();
        postArray.append(perPost);
    }

    resp["posts"] = postArray;

    QJsonDocument respDoc(resp);
    QByteArray respData = respDoc.toJson(QJsonDocument::Compact);
    respData.append('\n');
    socket->write(respData);
    qDebug() << "포스트 전부 송출 성공";
}

// geonwoo
// 게시판 특정 글 delete 처리 (post column의 ID 와 로그인한 ID 일치 여부 판단 필수)
void ClientHandler::readyRead_sendPostDelete(const QJsonObject &obj){
    qDebug() << "게시판 글 삭제 요청 받음";

    QString ID = obj.value("ID").toString();
    QString postUserID = obj.value("postUserID").toString();
    int postID = obj.value("postID").toInt();

    qDebug() << "글 삭제 요청 user ID : " << ID;
    qDebug() << "글 작성자 ID : " << postUserID;
    qDebug() << "글 번호 ID : " << postID;

    QJsonObject response;
    response["type"] = "postDelete";

    // 글 작성자 ID 와 요청자 ID 가 일치할 때 삭제 쿼리를 동작한다.
    if(ID == postUserID){
        bool sendPostDelete_isSuccess;
        QSqlQuery query;
        QString sqlText = QString("DELETE FROM Post WHERE postID = ?");
        query.prepare(sqlText);
        query.addBindValue(postID);

        QSqlQuery sendPostDeleteQuery = emit requestBindQuery(query, sendPostDelete_isSuccess);
        if(sendPostDelete_isSuccess){
            qDebug() << "post 삭제 완료";
            response["success"] = true;
        } else {
            qDebug() << "post 삭제 실패  " << sendPostDeleteQuery.lastError().text();
            response["success"] = false;
            response["reason"] = "쿼리 동작 실패 또는 postID 와 일치한 post 가 없음";
        }
    } else {
        response["success"] = false;
        response["reason"] = "글 작성자가 아니기 때문에 삭제 명령 실패 처리";
    }

    // 요청한 클라이언트에게만 응답 전송
    QJsonDocument responseDoc(response);
    QByteArray responseData = responseDoc.toJson(QJsonDocument::Compact);
    responseData.append('\n');

    socket->write(responseData);
    socket->flush();
    qDebug() << "게시판 글 삭제 처리 응답 전송 완료!!!";
}
