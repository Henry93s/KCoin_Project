#include "sendingManage.h"
#include "filesendmanager.h"

// 무언가 전달할 때 무조건 메세지 type 포함해서!!!
// 메세지 type의 종류(모두 소문자로 작성할것) :
//      login, signup, response(success,fail만 result에 대입), messagesend, givemelog, messagelog

//==========================
//       ID, PW 전달
//==========================
void sendingManage::sendLogIn(QString& ID, QString& PW){
    // Json 형태로 전달
    QJsonObject sendingObj;
    sendingObj["type"] = "login";
    sendingObj["ID"] = ID;
    sendingObj["PW"] = PW;
    QJsonDocument doc(sendingObj);
    QByteArray sendingArray(doc.toJson(QJsonDocument::Compact));
    sendingArray.append('\n');
    QTcpSocket* socket = SocketManage::instance().socket();
    qDebug() << "현재 소켓 : " << socket;
    socket->write(sendingArray);
    socket->flush();
    qDebug() << "서버로 로그인 요청";
}


//==========================
//       회원가입 요청
//==========================
void sendingManage::sendSignUp(QString& Name, QString& ID, QString& PW, QString& PhoneNum){
    QJsonObject sendingObj;
    sendingObj["type"] = "signup";
    sendingObj["Name"] = Name;
    sendingObj["ID"] = ID;
    sendingObj["PW"] = PW;
    sendingObj["Phone"] = PhoneNum;
    sendingObj["money"] = 10000000;
    QJsonDocument doc(sendingObj);
    QByteArray sendingArray(doc.toJson(QJsonDocument::Compact));
    sendingArray.append('\n');
    QTcpSocket* socket = SocketManage::instance().socket();
    socket->write(sendingArray);
    qDebug() << "현재 소켓 : " << socket;
    qDebug() << "서버로 회원가입 요청";
}

//==========================
//      채팅메세지 전송
//==========================
void sendingManage::sendMessage(const QString& chatViewName, const QString& textMessage){
    
    QJsonObject sendingObj;
    sendingObj["type"] = "messagesend";
    sendingObj["chatViewName"] = chatViewName;
    sendingObj["senderName"] = senderName;
    sendingObj["textMessage"] = textMessage;
    // geonwoo
    // 채팅 메시지 전송 시 senderID 추가 전송
    sendingObj["senderID"] = senderID;
    
    QJsonDocument doc(sendingObj);
    
    QByteArray sendingArray(doc.toJson(QJsonDocument::Compact));
    sendingArray.append('\n');
    
    QTcpSocket* socket = SocketManage::instance().socket();

    qDebug() << "현재 소켓 : " << socket;
    socket->write(sendingArray);
    qDebug()<< chatViewName << "으로[" << senderID << "](" << senderName << ")의" << textMessage <<" 전송";
}

//==========================
//      파일 전송
//==========================
void sendingManage::sendFile(QStringList filePaths, QString& chatViewName){
    qDebug() << "sendingManage.cpp sendFile";
    QTcpSocket* socket = SocketManage::instance().socket();
    FileSendManager *fileSendManager = new FileSendManager();
    // geonwoo
    // senderID 같이 전송 추가
    fileSendManager->sendFile(socket, "filesend", chatViewName, filePaths, senderName, senderID);
}

//==========================
//    파일 다운로드 요청
//==========================
void sendingManage::requestFileDownload(const QString& fileId){
    qDebug() << "sendingManage.cpp requestFileDownload";
    QTcpSocket* socket = SocketManage::instance().socket();
    QJsonObject sendingObj;
    sendingObj["type"] = "filedownload";
    sendingObj["fileId"] = fileId;
    sendingObj["requesterName"] = senderName;
    sendingObj["requesterID"] = senderID;
    
    QJsonDocument doc(sendingObj);
    QByteArray sendingArray(doc.toJson(QJsonDocument::Compact));
    sendingArray.append('\n');
    
    socket->write(sendingArray);
    socket->flush();
    qDebug() << "서버로 파일 다운로드 요청:" << fileId;
}

//==========================
//  채팅 로그 달라는 신호 전달
//==========================
void sendingManage::giveMeLog(QString& chatViewName){
    qDebug()<<"서버로 채팅 로그 전송 요청";
    QTcpSocket* socket = SocketManage::instance().socket();
    QJsonObject sendingObj;
    sendingObj["type"] = "givemelog";
    sendingObj["chatViewName"] = chatViewName;
    QJsonDocument doc(sendingObj);
    QByteArray sendingArray(doc.toJson(QJsonDocument::Compact));
    sendingArray.append('\n');

    socket->write(sendingArray);
}
//==========================
//  거래 신호 전달
//==========================
void sendingManage::sendTrade(const QString& action, const QString& coin, double price, int amount){
    QJsonObject sendingObj;
    sendingObj["type"] = "trade";
    sendingObj["action"] = action;
    sendingObj["coin"] = coin;
    sendingObj["price"] = price;
    sendingObj["amount"] = amount;
    sendingObj["senderName"] = senderName;

    QJsonDocument doc(sendingObj);
    QByteArray sendingArray(doc.toJson(QJsonDocument::Compact));
    sendingArray.append('\n');
    QTcpSocket* socket = SocketManage::instance().socket();
    socket->write(sendingArray);
}

//==========================
//  이메일 확인 신호 전달
//==========================
void sendingManage::sendEmailCheck(QString email){
    QTcpSocket* socket = SocketManage::instance().socket();
    QJsonObject sendingObj;
    sendingObj["type"] = "emailcheck";
    sendingObj["email"] = email;

    QJsonDocument doc(sendingObj);
    QByteArray sendingArray(doc.toJson(QJsonDocument::Compact));
    sendingArray.append('\n');
    socket->write(sendingArray);
}

//==========================
//  이메일 코드 확인 신호 전달
//==========================
void sendingManage::sendCodeEmailCheck(QString code){
    QTcpSocket* socket = SocketManage::instance().socket();
    QJsonObject sendingObj;
    sendingObj["type"] = "emailcodecheck";
    sendingObj["code"] = code;

    QJsonDocument doc(sendingObj);
    QByteArray sendingArray(doc.toJson(QJsonDocument::Compact));
    sendingArray.append('\n');
    socket->write(sendingArray);
}



sendingManage* sendingManage::m_instance = nullptr;
sendingManage::sendingManage(){

}
sendingManage* sendingManage::instance(){
    if(!m_instance){
        m_instance = new sendingManage;
        return m_instance;
    }
    return m_instance;
}

void sendingManage::setSenderName(const QString& name) {
    senderName = name;
}

// geonwoo
void sendingManage::setSenderID(const QString& ID){
    this->senderID = ID;
}

// geonwoo
QString sendingManage::getSenderID(){
    return this->senderID;
}

// geonwoo
// 게시판 글 write 요청 전달
void sendingManage::sendPostWrite(const QString& title, const QString& contents){



}

// geonwoo
// 게시판 전체 글 read 요청 전달
void sendingManage::sendPostAllRead(){

}

// geonwoo
// 게시판 특정 글 read 요청 전달
void sendingManage::sendPostRead(const int& postID){

}

// geonwoo
// 게시판 특정 글 delete 요청 전달
void sendingManage::sendPostDelete(const int& postID){

}


void sendingManage::sendReport(const QString& name, const QString& reason) {
    QJsonObject obj;
    obj["type"] = "report";
    obj["targetName"] = name;
    obj["reason"] = reason;
    QJsonDocument doc(obj);
    QByteArray arr = doc.toJson(QJsonDocument::Compact);
    arr.append('\n');

    QTcpSocket* socket = SocketManage::instance().socket();
    socket->write(arr);
    socket->flush();

    qDebug() << "서버로 신고 전송:" << arr;
}
