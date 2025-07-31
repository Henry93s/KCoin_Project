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
    QPushButton *pushButton;
    QPushButton *writePostButton;
    QPushButton *deletePostButton;

    QTextBrowser *orderType;
    QTextBrowser *orderDate;
    QTextBrowser *orderAmount;
    QTextBrowser *orderPrice;
    QTextBrowser *orderTotal;
    QTextEdit *postContentEdit;

    QToolBox *chatting_ToolBox;
    QListWidget *connect_listWidget;
    QListWidget *oneByone_listWidget;
    QListWidget *oneByMore_listWidget;
    QListWidget *postListWidget;

    QLineEdit* searchLineEdit;
    // CoinSearchLineEdit* searchLineEdit;
    CoinSearchWidget *coinSearchWidget;

    void setupUI();
    void connectSignal();
    void loadPosts();

private slots:
    void handleTradeResponse(const QJsonObject &obj);
};

#endif // HOMEVIEW_H
