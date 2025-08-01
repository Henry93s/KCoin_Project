#include "mainwindow.h"
#include "views/splashview.h"
#include <views/homeview.h>

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QTimer>
//#include <QSqlDatabase>
//#include <QSqlError>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // MariaDB 연결
//    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
//    db.setHostName("127.0.0.1");           // 또는 localhost
//    db.setDatabaseName("your_db_name");    // 예: postsdb
//    db.setUserName("your_username");       // 예: root
//    db.setPassword("your_password");       // 예: 1234

//    if (!db.open()) {
//        qDebug() << "DB 연결 실패:" << db.lastError().text();
//        return -1;
//    } else {
//        qDebug() << "DB 연결 성공!";
//    }

    MainWindow w;

    // 스플래쉬 뷰
    SplashView *splash = new SplashView;
    splash->show();

    // 2초 후 전환
    QTimer::singleShot(2000, &w, [&]() {
        splash->close();
        w.show();
    });

    return a.exec();
}

