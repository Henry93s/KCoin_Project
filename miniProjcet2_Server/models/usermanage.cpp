#include "usermanage.h"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QtSql>

#include "servermanager.h"

userManage::userManage(QObject* parent) : QObject(parent){
}

QString userManage::getDBPath() {
    // 애플리케이션 실행 경로에서 DB 폴더 찾기
    QString appDir = QCoreApplication::applicationDirPath();
    
    // macOS 앱 번들 내부에서 실행되는 경우 처리
    if (appDir.contains(".app/Contents/MacOS")) {
        // .app/Contents/MacOS에서 5단계 위로 올라가서 프로젝트 루트 찾기
        QDir dir(appDir);
        dir.cdUp(); // MacOS
        dir.cdUp(); // Contents
        dir.cdUp(); // .app
        dir.cdUp(); // build/Qt_...
        dir.cdUp(); // build
        dir.cdUp(); // miniProjcet2_Server
        QString dbPath = dir.absolutePath() + "/DB/userInfo.json";
        
        if (QFile::exists(dbPath)) {
            return dbPath;
        }
    }
    
    // 일반 실행 환경에서 DB 폴더 찾기
    QStringList possiblePaths = {
        "DB/userInfo.json",                    // 현재 디렉토리
        "../DB/userInfo.json",                 // 한 단계 위
        "../../DB/userInfo.json",              // 두 단계 위
        "../../../DB/userInfo.json",           // 세 단계 위
        QDir::homePath() + "/Desktop/WorkSpace/DevWooms/miniProject2/miniProjcet2_Server/DB/userInfo.json"  // 절대 경로
    };
    
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            qDebug() << "DB 파일 발견:" << path;
            return path;
        }
    }
    
    // 기본 경로 반환 (새로 생성용)
    QString defaultPath = "DB/userInfo.json";
    qWarning() << "DB 파일을 찾을 수 없음. 기본 경로 사용:" << defaultPath;
    return defaultPath;
}

userManage::~userManage(){
}

//=========================================
//                  로그인
//=========================================
bool userManage::signIn(QString& ID, QString& PW, QString& nameOut){
    bool isSuccess;
    QSqlQuery query = emit requestQuery(QString("SELECT ID, password, money, name, payment, phoneNum FROM coin.`User`"), isSuccess);
    if(!isSuccess){
        qDebug() << query.lastError();
    }
    // query.prepare(QString("SELECT ID, password, money, name, payment, phoneNum FROM coin.`User`"));
    // if(!query.exec()){
    //     qDebug() << query.lastError();
    // }

    while (query.next()) {
        QString listID = query.value(0).toString();
        QString listPWD = query.value(1).toString();
        QString listName = query.value(3).toString();
        if(ID == listID && PW == listPWD){
            qDebug() << "ID : "<<ID<<" 님이 입장하셨습니다.";
            nameOut = listName;
            return true;
        }
    }
    qDebug()<<"ID 또는 PW 불일치";

    return false;

    // QString dbPath = getDBPath();
    // QFile file(dbPath);                        // userInfo 파일 가져오기
    // qDebug() << "Current working directory:" << QDir::currentPath();
    // qDebug() << "DB 파일 경로:" << dbPath;

    // if (!file.open(QIODevice::ReadOnly)) {
    //     qWarning() << "파일 열기 실패:" << file.errorString();
    //     return false;
    // }
    // QByteArray data = file.readAll();                   // 가져온 파일 내용물 다 읽어옴
    // file.close();                                       // 파일 닫기
    // QJsonDocument doc = QJsonDocument::fromJson(data);
    // QJsonArray userList = doc.array();

    // for(const QJsonValue& val : userList){              // 저장되어있던 리스트 훑기
    //     QJsonObject userObj = val.toObject();

    //     QString listedID = userObj["ID"].toString();
    //     QString listedPW = userObj["PW"].toString();
    //     QString listedName = userObj["name"].toString();
    //     if(ID==listedID && PW==listedPW){
    //         qDebug() << "ID : "<<ID<<" 님이 입장하셨습니다.";
    //         nameOut = listedName;       // 이름 반환
    //         return true;
    //     }
    // }
    // qDebug()<<"ID 또는 PW 불일치";

    return false;
}




//=========================================
//                 회원 가입
//=========================================
void userManage::signUp(userInfo& info){
    // QString dbPath = getDBPath();
    // QFile file(dbPath);        // userInfo 파일 가져오기
    // qDebug() << "Signup - DB 파일 경로:" << dbPath;
    // // DB 디렉토리가 없을 경우 생성
    // QFileInfo fileInfo(file);
    // QDir dir = fileInfo.absoluteDir();
    // if (!dir.exists()) {
    //     if (dir.mkpath(dir.absolutePath())) {
    //         qDebug() << "DB 디렉토리 생성됨:" << dir.absolutePath();
    //     } else {
    //         qWarning() << "DB 디렉토리 생성 실패:" << dir.absolutePath();
    //         return;
    //     }
    // }
    // // userInfo 파일이 없을 경우 생성
    // if(!file.exists()){
    //     if(file.open(QIODevice::WriteOnly)){
    //         QJsonArray emptyArray;
    //         QJsonDocument doc(emptyArray);
    //         file.write(doc.toJson());
    //         file.close();
    //         qDebug()<<"userInfo Json 파일 생성됨:";
    //     }
    //     else{
    //         qWarning()<<"파일 생성 실패:" << file.errorString();
    //         return;
    //     }
    // }

    // // 전달받은 userInfo를 json Obejct로
    // QJsonObject userObj;
    // QJsonObject coins;
    // coins["krw-btc"] = 0;
    // coins["krw-eth"] = 0;
    // coins["krw-xrp"] = 0;
    // coins["krw-doge"] = 0;

    // userObj["name"] = info.name;
    // userObj["ID"] = info.ID;
    // userObj["PW"] = info.PW;
    // userObj["phoneNum"] = info.phoneNum;
    // userObj["coins"] = coins;
    // userObj["payment"] = info.payment;
    // userObj["money"] = 10000000;

    bool isSuccess;
    QSqlQuery query = emit requestQuery(
        QString("INSERT INTO coin.`User`(ID, password, money, name, payment, phoneNum) VALUES('%1', '%2', 10000000, '%3', %4, %5)").arg(info.ID).arg(info.PW).arg(info.name).arg(info.payment).arg(info.phoneNum),
        isSuccess);
    qDebug() << query.lastError();
    // // 저장할 내용 이어붙이기 위해 앞내용 읽어옴
    // QJsonArray userArray;                   // Json파일내용물 저장될 JsonArray
    // if(file.open(QIODevice::ReadOnly)){
    //     QByteArray readData = file.readAll();
    //     QJsonDocument dataDoc = QJsonDocument::fromJson(readData);
    //     userArray = dataDoc.array();
    //     file.close();
    // }
    // else{
    //     qWarning()<<"파일 읽기 실패";
    //     return;
    // }

    // userArray.append(userObj);      // json Object를 userArray로

    // if(file.open(QIODevice::WriteOnly)){
    //     QJsonDocument newDoc(userArray);
    //     file.write(newDoc.toJson());
    //     file.close();
    //     qDebug()<<info.name<<" 가입완료";
    // }
}

QJsonObject userManage::getUserDetailByName(const QString &name) {
    bool isSuccess;
    QSqlQuery query = emit requestQuery(QString("SELECT ID, password, money, name, payment, phoneNum FROM coin.`User`"), isSuccess);
    if(!isSuccess){
        qDebug() << query.lastError();
    }
    // query.prepare(QString("SELECT ID, password, money, name, payment, phoneNum FROM coin.`User`"));
    // if(!query.exec()){
    //     qDebug() << query.lastError();
    // }

    while (query.next()) {
        QString listName = query.value(3).toString();
        if(listName == name){
            QJsonObject userObj;
            QString listID = query.value(0).toString();
            userObj["ID"] = listID;
            userObj["PW"] = query.value(1).toString();
            userObj["name"] = listName;
            userObj["money"] = query.value(2).toDouble();
            userObj["payment"] = query.value(4).toDouble();
            userObj["phoneNum"] = query.value(5).toString();
            bool isSucc;
            QSqlQuery tradingQuery = emit requestQuery(QString("SELECT action, amount, coinID, tradedTime, price FROM tradingOfUser WHERE userID = '%1'").arg(listID), isSucc);

            QJsonArray trades;
            while(tradingQuery.next()){
                QJsonObject perTrade;
                perTrade["action"] = tradingQuery.value(0).toString();
                perTrade["amount"] = tradingQuery.value(1).toInt();
                perTrade["coin"] = tradingQuery.value(2).toString();
                perTrade["datetime"] = tradingQuery.value(3).toString();
                perTrade["price"] = tradingQuery.value(4).toDouble();
                trades.append(perTrade);
            }

            userObj["tradingHis"] = trades;

            return userObj;
        }
    }
    return QJsonObject();

    // QString dbPath = getDBPath();
    // QFile file(dbPath);
    // if (!file.open(QIODevice::ReadOnly))
    //     return QJsonObject();

    // QByteArray data = file.readAll();
    // file.close();
    // QJsonDocument doc = QJsonDocument::fromJson(data);
    // QJsonArray userList = doc.array();

    // for (const auto &val : userList) {
    //     QJsonObject userObj = val.toObject();
    //     if (userObj["name"].toString() == name) {
    //         return userObj;
    //     }
    // }
    // return QJsonObject(); // 못찾으면 빈 객체 반환
}

void userManage::increaseReport(const QString& name, const QString& reason) {
    QString reportPath = getDBPath().replace("userInfo.json", "report.json");
    QFile file(reportPath);
    QJsonArray arr;

    // 파일 읽기
    if (file.open(QIODevice::ReadOnly)) {
        arr = QJsonDocument::fromJson(file.readAll()).array();
        file.close();
    }
    // 찾으면 count++, reason 추가
    bool found = false;
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        if (o["name"].toString() == name) {
            o["count"] = o["count"].toInt() + 1;
            // reason 추가
            QJsonArray reasons = o.contains("reasons") ? o["reasons"].toArray() : QJsonArray();
            reasons.append(reason);
            o["reasons"] = reasons;
            arr[i] = o;
            found = true;
            break;
        }
    }
    if (!found) {
        QJsonObject o;
        o["name"] = name;
        o["count"] = 1;
        QJsonArray reasons;
        reasons.append(reason);
        o["reasons"] = reasons;
        arr.append(o);
    }
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson());
        file.close();
    }
}


bool userManage::isBanned(const QString& name) {
    QString reportPath = getDBPath().replace("userInfo.json", "report.json");
    QFile file(reportPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
        file.close();
        for (const auto& v : arr) {
            QJsonObject o = v.toObject();
            if (o["name"].toString() == name && o["count"].toInt() >= 5)
                return true;
        }
    }
    return false;
}
