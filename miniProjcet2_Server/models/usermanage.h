#ifndef USERMANAGE_H
#define USERMANAGE_H

#include <QObject>
#include <QDateTime>
#include <QMainWindow>
#include <QDateTime>        // 날짜 및 시간 관련 라이브러리
#include <QFile>            // 파일 관련 라이브러리
#include <QJsonDocument>    // Json관련 라이브러리
#include <QJsonArray>       // Json관련 라이브러리
#include <QJsonObject>      // Json관련 라이브러리
#include <QDebug>           // 디버깅 출력 라이브러리
#include <QSqlQuery>

using namespace std;

// 유저 정보 저장
class userInfo {
public:

    //========== 개인정보
    QString name;           // 이름
    QString ID;             // 아이디
    QString PW;             // 비밀번호
    QString phoneNum;           // 전화번호


    //========== 보유 자산
    int coinCnt;              // 코인 개수
    int payment;              // 매입 금액 (매수하는데에 쓴 총액)
    int money;                // 보유 현금


    //========== 거래 관련 정보
    struct tradeInfo{
        QDateTime tradeDate;      // 거래 일시
        int tradeCnt;             // 거래 코인수량
        int tradePrice;           // 거래당시 코인금액
    };

    QList<tradeInfo> tradingHis;    // 거래시마다 위 구조체 저장할 QList
};

// 유저 관리 기능 구현
class userManage : public QObject {
    Q_OBJECT
public:
    userManage(QObject* parent = nullptr);
    ~userManage();

    //========== 기능 구현 함수
    void signUp(userInfo& info);                    // 회원가입 기능 구현
    bool signIn(QString& ID, QString& PW, QString& nameOut);          // 로그인 기능 구현, 로그인 성공여부에 따라 참,거짓값 반환하므로 bool
    QJsonObject getUserDetailByName(const QString &name);
    void increaseReport(const QString& name, const QString& reason);
    bool isBanned(const QString& name);
    QString getDBPath();                           // DB 파일 경로 찾기
signals:
    QSqlQuery requestQuery(const QString& strQuery, bool& isSuccess);
private:

    //========== 시그널
signals:

};

#endif // USERMANAGE_H
