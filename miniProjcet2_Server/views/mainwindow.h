#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    /*
    QTcpServer *tcpServer;
    QList<QTcpSocket*> *clientSocketList;
    */
    QString getMyIP();

private slots:
    void clientConnect();
};
#endif // MAINWINDOW_H
