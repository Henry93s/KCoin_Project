#ifndef HOMEVIEW_H
#define HOMEVIEW_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QComboBox>
#include <QTextBrowser>
#include <QToolBox>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QSpacerItem>
#include <QFrame>
#include <QTimer>
#include <QEvent>
#include <QCheckBox>
#include <QString>
#include <QTcpSocket>

#include "coinsearchwidget.h"
#include "coinsearchlineedit.h"

class ChartsToolBox; // forward declaration

class HomeView : public QWidget
{
    Q_OBJECT

public:
    explicit HomeView(QWidget *parent = nullptr);
    ~HomeView();
    void setAccountInfo(const QJsonObject &userInfo, const QJsonArray &history);
    void set_update_price(double new_price);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    ChartsToolBox *chartBox; // ChartsToolBox 인스턴스 - devwooms
    
    // UI 요소들
    QSplitter *splitter;
    QTabWidget *tapwidget;
    QComboBox *comboBox;
    QVBoxLayout *chartTab;
    QTextBrowser *textBrowser;
    QTextBrowser *purchasePrice;
    QTextBrowser *gain;
    QTextBrowser *gainPercent;
    QTextBrowser *recentPrice;
    QTextBrowser* accountBrowser;
    QSpinBox *spinBox;
    QSpinBox *spinBox_2;
    QRadioButton *radioButton;
    QRadioButton *radioButton_2;
    // geonwoo
    // 지정 가격 checkBox_1, 지정 가격 textEdit_1 추가
    QCheckBox* checkBox_1;
    QDoubleSpinBox* doubleSpinBox_1;

    QPushButton *pushButton;
    QTextBrowser *orderType;
    QTextBrowser *orderDate;
    QTextBrowser *orderAmount;
    QTextBrowser *orderPrice;
    QTextBrowser *orderTotal;
    QToolBox *chatting_ToolBox;
    QListWidget *connect_listWidget;
    QListWidget *oneByone_listWidget;
    QListWidget *oneByMore_listWidget;

    QLineEdit* searchLineEdit;
    // CoinSearchLineEdit* searchLineEdit;
    CoinSearchWidget *coinSearchWidget;
    
    void setupUI();
    void connectSignal();

    // geonwoo
    // 코인 현재 가격 을 5초마다 한 번씩 가져옴
    QTimer* update_price_timer;
    double update_price = -1.00;

    // geonwoo
    // 알림 설정 값보다 현재 코인 가격이 낮을 때 BLUE LED 점등 GPIO 값 전달(1)
    void gpio_BLUE();

signals:
  void update_price_changed(double new_price);

private slots:
   void handleTradeResponse(const QJsonObject &obj);
   void on_update_price_changed(double new_price);
};

#endif // HOMEVIEW_H
