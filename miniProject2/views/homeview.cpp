#include "homeview.h"
#include "linechartview.h"
#include "models/sendingManage.h"
#include "chartstoolbox.h"
#include "models/chattinglistmanager.h"
#include <QIcon>
#include <QSizePolicy>
#include <QDebug>
#include "models/sendingManage.h"
//#include "post.h"
//#include "postmanager.h"

HomeView::HomeView(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignal();

    // 소켓에서 거래완료 신호 받을 경우 할 행동 호출
    connect(&SocketManage::instance(), &SocketManage::tradeResponseReceived, this, &HomeView::handleTradeResponse);

    // ==============================
    // 거래 탭 - devwooms
    // ==============================

    // ChartsToolBox 인스턴스 생성 - devwooms
    chartBox = new ChartsToolBox(this); // 멤버 변수로 저장 - devwooms
    chartBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    comboBox->addItem(QIcon(":/assets/assets/Bitcoin.png"), "KRW-BTC / 비트코인 / Bitcoin");
    comboBox->addItem(QIcon(":/assets/assets/Ethereum.jpg"), "KRW-ETH / 이더리움 / Ethereum");
    comboBox->addItem(QIcon(":/assets/assets/XRP.png"), "KRW-XRP / 엑스알피 [리플] / XRP");
    comboBox->addItem(QIcon(":/assets/assets/Dogecoin.svg"), "KRW-DOGE / 도지코인 / Dogecoin");
    comboBox->addItem(QIcon(":/assets/assets/PEPE.jpg"), "KRW-PEPE / 페페 / PEPE");

    // tab의 layout에 추가 - devwooms
    chartTab->addWidget(chartBox);

    // ==============================
    // 오른쪽 채팅 ToolBox - devwooms
    // ==============================

    ChattingListManager *chattingListManager = new ChattingListManager;

    chattingListManager->settingChattingToolBox(chatting_ToolBox);

    chattingListManager->settingConnectListWidget(connect_listWidget);
    chattingListManager->settingOneByOneListWidget(oneByone_listWidget);
    chattingListManager->settingOneByMoreListWidget(oneByMore_listWidget);

    // geonwoo
    update_price = chartBox->getLineChart()->getLatestPrice();
    set_update_price(update_price);

    // kimsungwon
    sendingManage::instance()->sendPostAllRead();
}

// 특정 이벤트 필터 ( 전체적인 widget의 override임
bool HomeView::eventFilter(QObject *obj, QEvent *event) {
    qDebug() << "eventFilter 호출 - obj:" << obj << "event type:" << event->type();
    
    // obj가 lineEdit일 때
    if (obj == searchLineEdit) {
        qDebug() << "searchLineEdit 이벤트 감지";
        
        if (event->type() == QEvent::FocusIn) {
            qDebug() << "Focus In - coinSearchWidget 보이기";
            coinSearchWidget->show();
            coinSearchWidget->raise(); // 위에 나타나도록
            return true;
        } 
        else if (event->type() == QEvent::FocusOut) {
            qDebug() << "Focus Out - coinSearchWidget 숨기기";
            // 약간의 지연을 두어 다른 위젯이 포커스를 받을 수 있도록 함
            QTimer::singleShot(100, this, [this]() {
                if (!searchLineEdit->hasFocus() && !coinSearchWidget->hasFocus()) {
                    coinSearchWidget->hide();
                }
            });
            return true;
        // 키보드 입력시
        } else if (event->type() == QEvent::KeyPress){
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                qDebug() << "Enter 키 감지";
                QString searchText = searchLineEdit->text();

                if (!searchText.isEmpty()) {
                    // Enter 입력 시 동작
                    // coinSearchWidget->selectFirstResult();
                    coinSearchWidget->hide();
                    searchLineEdit->clearFocus();
                }
                return true; // 이벤트 처리 완료
            }


        }
    }
    
    return QWidget::eventFilter(obj, event);
}

void HomeView::connectSignal(){

    connect(pushButton, &QPushButton::clicked, this, [this]() {
        QString action;
        int amount = 0;     // 수량

        if (radioButton->isChecked()) {
            action = "buy";
            amount = spinBox->value();
            spinBox->setValue(0);

        } else if (radioButton_2->isChecked()) {
            action = "sell";
            amount = spinBox_2->value();
            spinBox_2->setValue(0);
        }

        // 콤보박스에서 코인 심볼(krw-btc 등) 추출
        QString comboText = comboBox->itemText(comboBox->currentIndex());
        QString coinName = comboText.split(" / ").first().toLower();

        // 최신 가격 불러오기
        double price = chartBox->getLineChart()->getLatestPrice(); // 또는 getCurrentPrice()

        // 거래 요청 보내기
        sendingManage::instance()->sendTrade(action, coinName, price, amount);

        qDebug() << "거래신호 전송:" << coinName << action << ", 수량:" << amount << ", 가격:" << price;
    });

    // comboBox에서 선택시 선택 코인의 데이터를 가져오도록 연결
    connect(comboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        QString text = comboBox->itemText(index);
        qDebug() << "선택된 항목:" << text;

        // 코인 종목 추출 (예: "KRW-BTC / 비트코인 / Bitcoin" → "krw-btc") - devwooms
        QString coinSymbol = text.split(" / ").first().toLower();
        qDebug() << "추출된 코인 종목:" << coinSymbol;

        // 코인 설정 변경 - devwooms
        if (chartBox && chartBox->getLineChart()) {
            chartBox->getLineChart()->setCoin(coinSymbol);
            chartBox->getLineChart()->lineDataManager->update();
        }
        if (chartBox && chartBox->getCandleChart()) {
            chartBox->getCandleChart()->setCoin(coinSymbol);
            chartBox->getCandleChart()->candleDataManager->update();
        }
    });
    
    // searchLineEdit 텍스트 변경시 CoinSearchWidget에 전달
    connect(searchLineEdit, &QLineEdit::textChanged, coinSearchWidget, &CoinSearchWidget::updateSearchText);
    
    // CoinSearchWidget에서 코인 선택시 comboBox 업데이트
    connect(coinSearchWidget, &CoinSearchWidget::coinSelected, this, [this](const QString& symbol, const QString& koreanName, const QString& englishName) {
        qDebug() << "코인 선택됨 - 종목:" << symbol << "한국어명:" << koreanName << "영어명:" << englishName;
        
        // comboBox에서 해당 코인 찾아서 선택
        QString targetText = QString("%1 / %2 / %3").arg(symbol, koreanName, englishName);
        
        for (int i = 0; i < comboBox->count(); ++i) {
            if (comboBox->itemText(i) == targetText) {
                comboBox->setCurrentIndex(i);
                break;
            }
        }
        
        // 검색 위젯 숨기기
        coinSearchWidget->hide();
        searchLineEdit->clearFocus();
    });

    // geonwoo
    // 코인 현재 가격 을 5초마다 한 번씩 가져옴
    /*  코인 현재 가격 변경될 때마다 알림 가격 체크 박스가 체크되어 있을 때
        현재 값에 따른 GPIO 점등 명령 전송함
        알림 가격 이상일 때 : red, 알림 가격 이하일 때 : blue
    */
    connect(this, &HomeView::update_price_changed, this, &HomeView::on_update_price_changed);
    update_price_timer = new QTimer(this);
    connect(update_price_timer, &QTimer::timeout, this, [=] () {
        qDebug() << "코인 가격 조회 주기(5초) timer 동작 중";
        set_update_price(chartBox->getLineChart()->getLatestPrice());
    });
    update_price_timer->start(5000);

    // geonwoo
    /* 지정한 알림 가격의 값이 변경될 때 -> 알림 가격 체크 박스가 체크되어 있을 때
        현재 값에 따른 GPIO 점등 명령 전송함
        알림 가격 이상일 때 : red, 알림 가격 이하일 때 : blue
    */
    connect(doubleSpinBox_1, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){
        if(checkBox_1->isChecked()){
            qDebug() << "doubleSpinBox 값 : " << val;
            double update_prices = chartBox->getLineChart()->getLatestPrice();

            // 코인 현재가가 알림 가격보다 더 크거나 같을 땐 RED
            if(update_prices > val){
                qDebug() << "코인 현재가가 알림 가격보다 큼 RED";
                gpio_Up_LED();
            } else {
                qDebug() << "코인 현재가가 알림 가격보다 작을 때는 BLUE";
                gpio_Down_LED();
            }
        }
    });
    /* 알림 가격이 checked 되어 있을 때 -> 현재 알림 가격 값
        현재 값에 따른 GPIO 점등 명령 전송함
        알림 가격 이상일 때 : red, 알림 가격 이하일 때 : blue
    */
    connect(checkBox_1, &QCheckBox::checkStateChanged, this, [=] (bool is_check){
        if(is_check){
            qDebug() << "checkbox 이후 doubleSpinBox 값 : " << doubleSpinBox_1->value();
            double update_prices = chartBox->getLineChart()->getLatestPrice();

            // 코인 현재가가 알림 가격보다 더 크거나 같을 땐 RED
            if(update_prices > doubleSpinBox_1->value()){
                qDebug() << "코인 현재가가 알림 가격보다 큼 RED";
                gpio_Up_LED();
            } else {
                qDebug() << "코인 현재가가 알림 가격보다 작을 때는 BLUE";
                gpio_Down_LED();
            }
        }
    });
}

// geonwoo
void HomeView::set_update_price(double new_price){

    if(new_price != update_price){
        update_price = new_price;
        emit update_price_changed(update_price);
    }
}

// geonwoo
// update_price (코인 현재 가격) 이 변동될 때 (5초 타이머 동작)
//        현재 값에 따른 GPIO 점등 명령 전송함
//        알림 가격 이상일 때 : red, 알림 가격 이하일 때 : blue
void HomeView::on_update_price_changed(double new_price){
    if(checkBox_1->isChecked()){
        qDebug() << "5초 타이머 마다 체크로 인한 update_price 변동 이후 update_price 값 : " << new_price;
        qDebug() << "5초 타이머 마다 체크로 인한 update_price 변동 이후 doubleSpinBox 값 : " << doubleSpinBox_1->value();

        // 코인 현재가가 알림 가격보다 더 크거나 같을 땐 RED
        if(new_price > doubleSpinBox_1->value()){
            qDebug() << "코인 현재가가 알림 가격보다 큼 RED";
            gpio_Up_LED();
        } else {
            qDebug() << "코인 현재가가 알림 가격보다 작을 때는 BLUE";
            gpio_Down_LED();
        }
    }
}

// geonwoo
// 알림 설정 값보다 현재 코인 가격이 낮을 때 느린 LED 점등 GPIO 값과 비프음 시그널 전달(1)
void HomeView::gpio_Down_LED(){
    // 라즈베리파이 wipi IP
    QTcpSocket gpio_socket;
    gpio_socket.connectToHost("192.168.2.97", 51234);

    if (gpio_socket.waitForConnected(3000)) {
        gpio_socket.write("1"); // LED 켜기 데이터 write
        gpio_socket.flush(); // 버퍼 바로 비워서 즉시 write 되도록 함
        gpio_socket.waitForBytesWritten();  // write 완료 대기
        /* 리스너 코드에서 계속 client 연결을 받아야 하므로 점등 한 번 시행 시
         (임시) socket 의 연결은 끊어주도록 처리함 */
        gpio_socket.disconnectFromHost();
    }
}

// geonwoo
// 알림 설정 값보다 현재 코인 가격이 클 때 빠른 LED 점등 GPIO 값과 비프음 시그널 전달(2)
void HomeView::gpio_Up_LED(){
    // 라즈베리파이 wipi IP
    QTcpSocket gpio_socket;
    gpio_socket.connectToHost("192.168.2.97", 51234);

    if (gpio_socket.waitForConnected(3000)) {
        gpio_socket.write("2"); // LED 켜기 데이터 write
        gpio_socket.flush(); // 버퍼 바로 비워서 즉시 write 되도록 함
        gpio_socket.waitForBytesWritten();  // write 완료 대기
        /* 리스너 코드에서 계속 client 연결을 받아야 하므로 점등 한 번 시행 시
         (임시) socket 의 연결은 끊어주도록 처리함 */
        gpio_socket.disconnectFromHost();
    }
}

void HomeView::setupUI()
{
    // 메인 레이아웃
    QHBoxLayout *horizontalLayout_4 = new QHBoxLayout(this);
    horizontalLayout_4->setContentsMargins(5, 5, 5, 5);
    
    // 스플리터
    splitter = new QSplitter(Qt::Horizontal);
    
    // 왼쪽 탭 위젯
    tapwidget = new QTabWidget();
    tapwidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // 시세 탭
    QWidget *tab = new QWidget();
    chartTab = new QVBoxLayout(tab);


    // 코인 검색 기능 추가
    searchLineEdit = new QLineEdit();
    searchLineEdit->setPlaceholderText("코인 검색...");
    searchLineEdit->setClearButtonEnabled(true); // X 버튼 추가
    // 이벤트 필터 설치
    searchLineEdit->installEventFilter(this);
    chartTab->addWidget(searchLineEdit);

    // 검색 결과 위젯
    coinSearchWidget = new CoinSearchWidget(this);
    coinSearchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    coinSearchWidget->setMinimumHeight(100);
    coinSearchWidget->hide(); // 초기에는 숨김
    chartTab->addWidget(coinSearchWidget);

    // 드롭다운
    comboBox = new QComboBox();
    chartTab->addWidget(comboBox);

    tapwidget->addTab(tab, "시세");

    // 계좌 탭
    QWidget *tab_2 = new QWidget();
    QVBoxLayout *verticalLayout = new QVBoxLayout(tab_2);
    
    // 상단 텍스트 브라우저
    textBrowser = new QTextBrowser();
    textBrowser->setHtml("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                         "<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
                         "p, li { white-space: pre-wrap; }\n"
                         "hr { height: 1px; border-width: 0; }\n"
                         "li.unchecked::marker { content: \"\2610\"; }\n"
                         "li.checked::marker { content: \"\2612\"; }\n"
                         "</style></head><body style=\" font-family:'.AppleSystemUIFont'; font-size:13pt; font-weight:400; font-style:normal;\">\n"
                         "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'맑은 고딕'; font-size:9pt;\">이름 / &lt;현재 보유중인 현금&gt;</span></p></body></html>");
    verticalLayout->addWidget(textBrowser);
    
    // 그리드 레이아웃
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(0);
    
    // 그리드 요소들 추가
    purchasePrice = new QTextBrowser();
    gridLayout->addWidget(purchasePrice, 0, 1, 1, 2);
    
    QTextBrowser *textBrowser_8 = new QTextBrowser();
    textBrowser_8->setHtml("<p align=\"center\">수익률(%)</p>");
    gridLayout->addWidget(textBrowser_8, 1, 3);
    
    QTextBrowser *textBrowser_6 = new QTextBrowser();
    textBrowser_6->setHtml("<p align=\"center\">평가손익</p>");
    gridLayout->addWidget(textBrowser_6, 0, 3);
    
    QTextBrowser *textBrowser_4 = new QTextBrowser();
    textBrowser_4->setHtml("<p align=\"center\">평가금액</p>");
    gridLayout->addWidget(textBrowser_4, 1, 0, 1, 2);
    
    gainPercent = new QTextBrowser();
    gridLayout->addWidget(gainPercent, 1, 4);
    
    gain = new QTextBrowser();
    gridLayout->addWidget(gain, 0, 4);
    
    QTextBrowser *textBrowser_3 = new QTextBrowser();
    textBrowser_3->setHtml("<p align=\"center\">매입금액</p>");
    gridLayout->addWidget(textBrowser_3, 0, 0);
    
    recentPrice = new QTextBrowser();
    gridLayout->addWidget(recentPrice, 1, 2);
    
    verticalLayout->addLayout(gridLayout);
    
    // 구분선
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    verticalLayout->addWidget(line);
    
    // 거래내역 제목
    QTextBrowser *textBrowser_23 = new QTextBrowser();
    textBrowser_23->setHtml("<p align=\"center\">거래내역</p>");
    verticalLayout->addWidget(textBrowser_23);
    
    // 거래내역 헤더
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(0);
    
    QTextBrowser *textBrowser_14 = new QTextBrowser();
    textBrowser_14->setHtml("<p align=\"center\">주문정보</p>");
    horizontalLayout->addWidget(textBrowser_14);
    
    QTextBrowser *textBrowser_10 = new QTextBrowser();
    textBrowser_10->setHtml("<p align=\"center\">거래일시</p>");
    horizontalLayout->addWidget(textBrowser_10);
    
    QTextBrowser *textBrowser_11 = new QTextBrowser();
    textBrowser_11->setHtml("<p align=\"center\">거래수량</p>");
    horizontalLayout->addWidget(textBrowser_11);
    
    QTextBrowser *textBrowser_12 = new QTextBrowser();
    textBrowser_12->setHtml("<p align=\"center\">거래액수</p>");
    horizontalLayout->addWidget(textBrowser_12);
    
    QTextBrowser *textBrowser_13 = new QTextBrowser();
    textBrowser_13->setHtml("<p align=\"center\">총액수</p>");
    horizontalLayout->addWidget(textBrowser_13);
    
    verticalLayout->addLayout(horizontalLayout);
    
    // 거래내역 데이터
    QHBoxLayout *horizontalLayout_3 = new QHBoxLayout();
    horizontalLayout_3->setSpacing(0);
    
    orderType = new QTextBrowser();
    horizontalLayout_3->addWidget(orderType);
    
    orderDate = new QTextBrowser();
    horizontalLayout_3->addWidget(orderDate);
    
    orderAmount = new QTextBrowser();
    horizontalLayout_3->addWidget(orderAmount);
    
    orderPrice = new QTextBrowser();
    horizontalLayout_3->addWidget(orderPrice);
    
    orderTotal = new QTextBrowser();
    horizontalLayout_3->addWidget(orderTotal);
    
    verticalLayout->addLayout(horizontalLayout_3);
    
    tapwidget->addTab(tab_2, "계좌");

    //hyungoo
    // 게시판 탭 - tab_5
    QWidget *tab_5 = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(tab_5);

    // 1. "자유게시판" 라벨 (상단 1/10 비율)
    QLabel *titleLabel = new QLabel("자유게시판");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    mainLayout->addWidget(titleLabel, 1);  // stretch: 1

    // 2. 글 제목 목록 (QListWidget) (중간 8/10 비율)
    postListWidget = new QListWidget();
    postListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(postListWidget, 8);  // stretch: 8

    // 전체 버튼을 감싸는 수직 레이아웃
    QVBoxLayout *buttonLayout = new QVBoxLayout();

    // =====================
    // 글 생성 영역
    // =====================
    QPushButton* uploadWriting = new QPushButton("글쓰기");
    uploadWriting->setIcon(QIcon(":/assets/assets/writing.png"));
    uploadWriting->setIconSize(QSize(20, 20));
    uploadWriting->setMinimumHeight(35);

    // =====================
    // 삭제 영역
    // =====================
    QPushButton* deleteWriting = new QPushButton("삭제하기");
    deleteWriting->setIcon(QIcon(":/assets/assets/eraser.png"));
    deleteWriting->setIconSize(QSize(20, 20));
    deleteWriting->setMinimumHeight(35);

    // 버튼을 수직으로 추가
    buttonLayout->addWidget(uploadWriting);
    buttonLayout->addWidget(deleteWriting);

    // 버튼 간 여백 조절하고, stretch 없앰
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    // 최종 배치 (예: 글 목록 하단)
    mainLayout->addLayout(buttonLayout);  // ← stretch 제거
/*
    postListWidget = new QListWidget();
    postListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    postListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);  // 스크롤 항상 보이기
    postListWidget->setStyleSheet(R"(
    QListWidget::item {
        font-size: 16px;
        font-weight: bold;
        padding: 10px;
        margin: 5px;
        border: 1px solid #888;
        border-radius: 6px;
        background-color: #f4f4f4;
    }
    QListWidget::item:selected {
        background-color: #d0ebff;
        border: 2px solid #0078d7;
    }
)");
    mainLayout->addWidget(postListWidget, 8);  // 기존 8/10 비율 유지
*/
    // kimsungwon
    connect(&SocketManage::instance(), &SocketManage::allPostsReceived, this, [&](const QJsonObject& obj) {
        const auto& posts = obj["posts"].toArray();
        for(const auto& perPost : posts){
            auto perPostObj = perPost.toObject();
            QString postID = perPostObj["postID"].toString();
            QString userID = perPostObj["userID"].toString();
            QString title = perPostObj["title"].toString();
            QString contents = perPostObj["contents"].toString();

            if (!title.isEmpty()) {
                // TODO: 서버 또는 DB 저장 로직 여기에
                // 글 작성 요청 전달
                sendingManage::instance()->sendPostWrite(title, contents);

                // 리스트 최상단에 글 제목 추가
                QListWidgetItem* item = new QListWidgetItem(title);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Unchecked);
                item->setSizeHint(QSize(0, 40));  // 아이템 높이 크게

                item->setData(Qt::UserRole, contents);  // 본문 저장
                // item->setData(Qt::UserRole + 1, currentUserId);

                postListWidget->insertItem(0, item);   // 최신순 (위쪽에 삽입)
            }
        }
    });
    // QString currentUserId = "jhn00162";
    connect(uploadWriting, &QPushButton::clicked, [this]() {
        QDialog dialog(this);
        dialog.setWindowTitle("Welcome");

        QVBoxLayout* vbox = new QVBoxLayout(&dialog);
        QLineEdit* TitleEdit=new QLineEdit(&dialog);
        TitleEdit->setPlaceholderText("글의 제목을 입력하세요");
        vbox->addWidget(new QLabel("글의 제목"));
        vbox->addWidget(TitleEdit);

        QTextEdit* mainWriting=new QTextEdit(&dialog);
        mainWriting->setPlaceholderText("본문을 입력하세요");
        vbox->addWidget(new QLabel("본문 내용"));
        vbox->addWidget(mainWriting);

        // 글 올리기 버튼
//        QPushButton* submitBtn = new QPushButton("올리기", &dialog);
//        vbox->addWidget(submitBtn);

        QDialogButtonBox* buttonBox=new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        vbox->addWidget(buttonBox);

        // [확인] 버튼 처리
        connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
            QString title = TitleEdit->text().trimmed();
            QString content = mainWriting->toPlainText().trimmed();

            if (!title.isEmpty()) {
                // TODO: 서버 또는 DB 저장 로직 여기에
                // 글 작성 요청 전달
                sendingManage::instance()->sendPostWrite(title, content);

                // 리스트 최상단에 글 제목 추가
                QListWidgetItem* item = new QListWidgetItem(title);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Unchecked);
                item->setSizeHint(QSize(0, 40));  // 아이템 높이 크게

                item->setData(Qt::UserRole, content);  // 본문 저장
                // item->setData(Qt::UserRole + 1, currentUserId);

                postListWidget->insertItem(0, item);   // 최신순 (위쪽에 삽입)
            }
            dialog.accept();
        });

        // [취소] 버튼
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        dialog.exec();  // 다이얼로그 실행
    });

    connect(postListWidget, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        QString title = item->text();
        QString content = item->data(Qt::UserRole).toString();

        // 본문 보기 다이얼로그
        QDialog readDialog(this);
        readDialog.setWindowTitle("[" + title + "]");

        QVBoxLayout* layout = new QVBoxLayout(&readDialog);
        QTextEdit* contentView = new QTextEdit(content);
        contentView->setReadOnly(true);
        layout->addWidget(new QLabel("본문 내용"));
        layout->addWidget(contentView);

        QPushButton* closeBtn = new QPushButton("닫기");
        layout->addWidget(closeBtn);
        connect(closeBtn, &QPushButton::clicked, &readDialog, &QDialog::accept);

        readDialog.resize(400, 300);
        readDialog.exec();
    });

    connect(deleteWriting, &QPushButton::clicked, this, [this]() {
        for (int i = postListWidget->count() - 1; i >= 0; --i) {
            QListWidgetItem* item = postListWidget->item(i);
            if (item->checkState() == Qt::Checked) {
                delete postListWidget->takeItem(i);  // 리스트에서 제거 + 메모리 해제
            }
        }
    });

    postListWidget->setStyleSheet(R"(
    QListWidget::item {
        font-size: 14px;
        padding: 5px;
    }
)");



    /*
    connect(deleteWriting, &QPushButton::clicked, [=, this]() {
        for (int i = postListWidget->count() - 1; i >= 0; --i) {
            QListWidgetItem* item = postListWidget->item(i);
            if (item->checkState() == Qt::Checked) {
                QString writerId = item->data(Qt::UserRole + 1).toString();
                if (writerId == currentUserId) {
                    delete postListWidget->takeItem(i); // 삭제
                } else {
                    QMessageBox::warning(this, "삭제 권한 없음", "해당 글은 본인 글이 아닙니다.");
                }
            }
        }
    });
*/
/*
        // 올리기 버튼 클릭 시 동작
        connect(submitBtn, &QPushButton::clicked, [&]() {
            QString title = TitleEdit->text().trimmed();
            QString content = mainWriting->toPlainText().trimmed();
            if (title.isEmpty() || content.isEmpty()) {
                QMessageBox::warning(&dialog, "입력 오류", "제목과 내용을 모두 입력해주세요.");
                return;
            }

            // TODO: 서버 or DB에 글 저장하는 로직 호출
            qDebug() << "[글쓰기] 제목:" << title << "/ 내용:" << content;

            dialog.accept();  // 다이얼로그 닫기
        });

        dialog.exec();  // ← 다이얼로그 띄우기 (모달)
    });
*/
    // 탭 추가
    tapwidget->addTab(tab_5, "게시판");
    
    splitter->addWidget(tapwidget);
    
    // 오른쪽 탭 위젯
    QTabWidget *tabWidget = new QTabWidget();
    
    // 매수/매도 탭
    QWidget *tab_3 = new QWidget();
    QWidget *layoutWidget = new QWidget(tab_3);
    layoutWidget->setGeometry(QRect(21, 13, 213, 427));
    
    QVBoxLayout *verticalLayout_4 = new QVBoxLayout(layoutWidget);
    
    // 여백
    QSpacerItem *verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout_4->addItem(verticalSpacer_2);
    
    // 현재 잔고
    QLabel *label = new QLabel("현재 잔고");
    label->setAlignment(Qt::AlignCenter);
    verticalLayout_4->addWidget(label);
    
    // lineEdit = new QLineEdit();
    // lineEdit->setMinimumHeight(30);
    // lineEdit->setReadOnly(true);
    // verticalLayout_4->addWidget(lineEdit);
    accountBrowser = new QTextBrowser();
    accountBrowser->setMinimumHeight(30);
    accountBrowser->setMaximumHeight(30);
    accountBrowser->setReadOnly(true);
    verticalLayout_4->addWidget(accountBrowser);
    
    QSpacerItem *verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout_4->addItem(verticalSpacer_4);
    
    // 매수/매도 옵션
    QVBoxLayout *verticalLayout_3 = new QVBoxLayout();
    
    radioButton = new QRadioButton("매수");
    radioButton->setMinimumHeight(30);
    QFont font;
    font.setPointSize(20);
    radioButton->setFont(font);
    verticalLayout_3->addWidget(radioButton);
    
    spinBox = new QSpinBox();
    spinBox->setMinimumHeight(30);
    spinBox->setMaximum(99999);
    verticalLayout_3->addWidget(spinBox);
    
    radioButton_2 = new QRadioButton("매도");
    radioButton_2->setMinimumHeight(30);
    radioButton_2->setFont(font);
    verticalLayout_3->addWidget(radioButton_2);
    
    spinBox_2 = new QSpinBox();
    spinBox_2->setMinimumHeight(30);
    spinBox_2->setMaximum(99999);
    verticalLayout_3->addWidget(spinBox_2);

    // geonwoo
    // GPIO_CONTROL
    // 알림 가격 체크박스, doubleSpinBox 추가
    // 알림 가격 체크박스 체크 시 -> textedit -> 값 read -> up : red, down : blue
    checkBox_1 = new QCheckBox("알림 설정");
    checkBox_1->setMinimumHeight(40);
    checkBox_1->setFont(font);
    verticalLayout_3->addWidget(checkBox_1);

    doubleSpinBox_1 = new QDoubleSpinBox();
    doubleSpinBox_1->resize(150, 40);
    doubleSpinBox_1->setMinimum(0.00);

    // 9억 99999999
    doubleSpinBox_1->setMaximum(999999999.99);
    doubleSpinBox_1->setDecimals(2);
    doubleSpinBox_1->setValue(0.00);
    doubleSpinBox_1->setSingleStep(0.01);
    verticalLayout_3->addWidget(doubleSpinBox_1);


    verticalLayout_4->addLayout(verticalLayout_3);
    
    QSpacerItem *verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout_4->addItem(verticalSpacer_3);
    
    // 버튼 레이아웃
    QHBoxLayout *horizontalLayout_5 = new QHBoxLayout();
    QSpacerItem *horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout_5->addItem(horizontalSpacer_2);
    
    pushButton = new QPushButton("PushButton");
    pushButton->setMinimumHeight(50);
    horizontalLayout_5->addWidget(pushButton);

    QSpacerItem *horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout_5->addItem(horizontalSpacer_3);
    
    verticalLayout_4->addLayout(horizontalLayout_5);
    
    QSpacerItem *verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout_4->addItem(verticalSpacer);
    
    tabWidget->addTab(tab_3, "매수 / 매도");
    
    // 채팅 탭
    QWidget *tab_4 = new QWidget();
    QHBoxLayout *horizontalLayout_2 = new QHBoxLayout(tab_4);
    
    chatting_ToolBox = new QToolBox();
    chatting_ToolBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chatting_ToolBox->setMinimumWidth(200);
    chatting_ToolBox->setCurrentIndex(2);
    
    // 접속자 목록
    QWidget *connect_list = new QWidget();
    connect_list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *verticalLayout_2 = new QVBoxLayout(connect_list);

    // // 접속자 목록 verticalLayout에 신고 기능 추가 - donjizzkan
    // QHBoxLayout* topLayout = new QHBoxLayout;
    // QLabel* connectLabel = new QLabel("접속자 목록");
    // QPushButton* reportBtn = new QPushButton;
    // reportBtn->setIcon(QIcon("report.png"));
    // reportBtn->setFixedSize(20, 20);
    // reportBtn->setToolTip("신고");

    // topLayout->addWidget(connectLabel);
    // topLayout->addStretch();
    // topLayout->addWidget(reportBtn);
    // verticalLayout_2->addLayout(topLayout);

    // // 신고 버튼 누를경우 신고 신호 전송 - donjizzkan
    // connect(reportBtn, &QPushButton::clicked, [this]() {
    //     // 선택된 닉네임이 있을 경우 자동입력
    //     QString selectedNick;
    //     auto item = connect_listWidget->currentItem();
    //     if (item) selectedNick = item->text();

    //     QDialog dialog(this);
    //     dialog.setWindowTitle("신고하기");
    //     QVBoxLayout* vbox = new QVBoxLayout(&dialog);

    //     QLineEdit* nickEdit = new QLineEdit(selectedNick, &dialog);
    //     nickEdit->setPlaceholderText("신고 대상 닉네임");
    //     vbox->addWidget(new QLabel("신고 대상 닉네임:"));
    //     vbox->addWidget(nickEdit);

    //     QTextEdit* reasonEdit = new QTextEdit(&dialog);
    //     reasonEdit->setPlaceholderText("신고 사유를 입력하세요...");
    //     vbox->addWidget(new QLabel("신고 사유:"));
    //     vbox->addWidget(reasonEdit);

    //     QPushButton* okBtn = new QPushButton("신고", &dialog);
    //     vbox->addWidget(okBtn);

    //     connect(okBtn, &QPushButton::clicked, [&]() {
    //         QString name = nickEdit->text().trimmed();
    //         QString reason = reasonEdit->toPlainText().trimmed();
    //         if (name.isEmpty() || reason.isEmpty()) {
    //             QMessageBox::warning(&dialog, "전송 불가", "닉네임과 사유를 모두 입력하세요");
    //             return;
    //         }
    //         sendingManage::instance()->sendReport(name, reason);
    //         dialog.accept();
    //     });

    //     dialog.exec();
    // });

    connect_listWidget = new QListWidget();
    verticalLayout_2->addWidget(connect_listWidget);
    // chatting_ToolBox->addItem(connect_list, "접속자 목록");
    
    // 개인 채팅방
    QWidget *oneByone_list = new QWidget();
    QVBoxLayout *verticalLayout_6 = new QVBoxLayout(oneByone_list);
    verticalLayout_6->setContentsMargins(0, 0, 0, 0);
    oneByone_listWidget = new QListWidget();
    verticalLayout_6->addWidget(oneByone_listWidget);
    // chatting_ToolBox->addItem(oneByone_list, "개인 채팅방");
    
    // 오픈 채팅방
    QWidget *oneByMore_list = new QWidget();
    oneByMore_list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *verticalLayout_7 = new QVBoxLayout(oneByMore_list);
    // verticalLayout_7->setContentsMargins(20, 20, 20, 20);

    // 접속자 목록 verticalLayout에 신고 기능 추가 - donjizzkan
    QHBoxLayout* topLayout = new QHBoxLayout;
    // topLayout->setAlignment(Qt::AlignVCenter);  // 레이아웃 자체를 수직 가운데 정렬
    
    QLabel* connectLabel = new QLabel("신고하기");
    // connectLabel->setAlignment(Qt::AlignVCenter);  // 라벨 내용도 가운데 정렬
    
    QPushButton* reportBtn = new QPushButton;
    reportBtn->setIcon(QIcon("report.png"));
    reportBtn->setFixedSize(20, 20);
    reportBtn->setToolTip("신고");

    topLayout->addWidget(connectLabel);
    topLayout->addStretch();
    topLayout->addWidget(reportBtn);
    verticalLayout_7->addLayout(topLayout);

    // 신고 버튼 누를경우 신고 신호 전송 - donjizzkan
    connect(reportBtn, &QPushButton::clicked, [this]() {
        // 선택된 닉네임이 있을 경우 자동입력
        QString selectedNick;
        auto item = connect_listWidget->currentItem();
        if (item) selectedNick = item->text();

        QDialog dialog(this);
        dialog.setWindowTitle("신고하기");
        QVBoxLayout* vbox = new QVBoxLayout(&dialog);

        QLineEdit* nickEdit = new QLineEdit(selectedNick, &dialog);
        nickEdit->setPlaceholderText("신고 대상 닉네임");
        vbox->addWidget(new QLabel("신고 대상 닉네임:"));
        vbox->addWidget(nickEdit);

        QTextEdit* reasonEdit = new QTextEdit(&dialog);
        reasonEdit->setPlaceholderText("신고 사유를 입력하세요...");
        vbox->addWidget(new QLabel("신고 사유:"));
        vbox->addWidget(reasonEdit);

        QPushButton* okBtn = new QPushButton("신고", &dialog);
        vbox->addWidget(okBtn);

        connect(okBtn, &QPushButton::clicked, [&]() {
            QString name = nickEdit->text().trimmed();
            QString reason = reasonEdit->toPlainText().trimmed();
            if (name.isEmpty() || reason.isEmpty()) {
                QMessageBox::warning(&dialog, "전송 불가", "닉네임과 사유를 모두 입력하세요");
                return;
            }
            sendingManage::instance()->sendReport(name, reason);
            dialog.accept();
        });

        dialog.exec();
    });

    oneByMore_listWidget = new QListWidget();
    verticalLayout_7->addWidget(oneByMore_listWidget);
    chatting_ToolBox->addItem(oneByMore_list, "오픈 채팅방");

    horizontalLayout_2->addWidget(chatting_ToolBox);
    
    tabWidget->addTab(tab_4, "채팅");
    
    splitter->addWidget(tabWidget);
    
    horizontalLayout_4->addWidget(splitter);
    
    // 오른쪽 여백
    QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout_4->addItem(horizontalSpacer);
}

void HomeView::handleTradeResponse(const QJsonObject &obj){
    // [수정된 부분] obj 전체가 아니라 user 키 안의 객체를 먼저 꺼내야 함 -> 이제 obj에서 바로 받음
    // QJsonObject userInfo = obj["user"].toObject(); // 이제 필요 없음

    qDebug() << "받은 tradeResponse 전체:" << obj; // 이제 obj에 money, payment, coins가 직접 담김
    // qDebug() << "userInfo 전체:" << userInfo; // 이제 필요 없음
    qDebug() << "payment raw:" << obj["payment"]; // [수정된 부분] obj에서 바로 payment를 확인
    qDebug() << "payment toDouble:" << obj["payment"].toDouble();
    qDebug() << "money raw:" << obj["money"]; // [수정된 부분] obj에서 바로 money를 확인
    qDebug() << "money toDouble:" << obj["money"].toDouble();


    QString result = obj["result"].toString();
    if(result != "success"){
        textBrowser->append("거래 실패, 잔고를 확인하세요");
        return;
    }

    // [수정된 부분] money, payment, coins를 obj에서 직접 받음
    double money = obj["money"].toDouble();
    double payment = obj["payment"].toDouble();
    QJsonObject coins = obj["coins"].toObject(); // [수정된 부분] obj에서 바로 coins를 받음

    QString selectedCoin = comboBox->currentText().split(" / ").first().toLower();

    int coinAmount = coins[selectedCoin].toInt();

    // 최신 거래 가격 (history 마지막 값에서 price 추출)
    QJsonArray history = obj["history"].toArray(); // history는 원래 obj에서 직접 받음

    double latestPrice = 0;
    if(!history.isEmpty()) {
        QJsonObject lastRecord = history.last().toObject();
        latestPrice = lastRecord["price"].toDouble();
    }

    // 평가금액 = 현재 보유수량 * 최신가격
    double evalAmount = coinAmount * latestPrice;
    // 평가손익 = 평가금액 - payment (단순화, 실제 계산 로직은 상황에 따라 보정)
    double evalProfit = evalAmount - payment;
    // 수익률 = (평가손익 / payment) * 100
    double profitPercent = payment > 0 ? (evalProfit / payment) * 100.0 : 0;

    purchasePrice->setText(QString::number(payment, 'f', 2));      // 매입금액
    gain->setText(QString::number(evalProfit, 'f', 2));            // 평가손익
    recentPrice->setText(QString::number(evalAmount, 'f', 2));     // 평가금액
    gainPercent->setText(QString::number(profitPercent, 'f', 2));  // 수익률(%)

    orderType->clear();
    orderDate->clear();
    orderAmount->clear();
    orderPrice->clear();
    orderTotal->clear();
    textBrowser->setHtml(QString("<p align=\"center\">계좌 잔고 : %1</p>").arg(QString::number(money, 'f', 2)));
    accountBrowser->setText(QString::number(money, 'f', 2));

    for (const auto &val : history) {
        QJsonObject rec = val.toObject();

        // 주문정보: 매수/매도 + 코인명
        QString orderInfo = QString("%1(%2)").arg(rec["action"].toString()).arg(rec["coin"].toString());
        // 거래일시: datetime
        QString date = rec["datetime"].toString();
        // 거래수량: amount
        QString amount = QString::number(rec["amount"].toInt());
        // 거래액수: price
        QString price = QString::number(rec["price"].toDouble());
        // 총액수: price * amount
        QString total = QString::number(rec["price"].toDouble() * rec["amount"].toInt());

        orderType->append(orderInfo);
        orderDate->append(date);
        orderAmount->append(amount);
        orderPrice->append(price);
        orderTotal->append(total);
    }
}

void HomeView::setAccountInfo(const QJsonObject &userInfo, const QJsonArray &history) {
    qDebug() << "userInfo 전체:" << userInfo;
    qDebug() << "payment raw:" << userInfo["payment"];
    qDebug() << "payment toDouble:" << userInfo["payment"].toDouble();
    qDebug() << "money raw:" << userInfo["money"];
    qDebug() << "money toDouble:" << userInfo["money"].toDouble();

    double money = userInfo["money"].toDouble();
    double payment = userInfo["payment"].toDouble();
    QJsonObject coins = userInfo["coins"].toObject();
    QString selectedCoin = comboBox->currentText().split(" / ").first().toLower();
    int coinAmount = coins[selectedCoin].toInt();

    // 거래내역 최신가 구하기
    double latestPrice = 0;
    if (!history.isEmpty()) {
        QJsonObject lastRec = history.last().toObject();
        latestPrice = lastRec["price"].toDouble();
    }
    if (latestPrice == 0 && chartBox && chartBox->getLineChart()) {
        latestPrice = chartBox->getLineChart()->getLatestPrice();
    }

    double evalAmount = coinAmount * latestPrice;
    double evalProfit = evalAmount - payment;
    double profitPercent = payment > 0 ? (evalProfit / payment) * 100.0 : 0;

    // 각 칸 채우기
    purchasePrice->setText(QString::number(payment, 'f', 2));
    gain->setText(QString::number(evalProfit, 'f', 2));
    recentPrice->setText(QString::number(evalAmount, 'f', 2));
    gainPercent->setText(QString::number(profitPercent, 'f', 2));
    textBrowser->setHtml(QString("<p align=\"center\">계좌 잔고 : %1</p>").arg(QString::number(money, 'f', 2)));
    accountBrowser->setText(QString::number(money, 'f', 2));

    // 거래내역 표 채우기
    orderType->clear();
    orderDate->clear();
    orderAmount->clear();
    orderPrice->clear();
    orderTotal->clear();
    for (const auto &val : history) {
        QJsonObject rec = val.toObject();
        QString orderInfo = QString("%1(%2)").arg(rec["action"].toString()).arg(rec["coin"].toString());
        QString date = rec["datetime"].toString();
        QString amount = QString::number(rec["amount"].toInt());
        QString price = QString::number(rec["price"].toDouble());
        QString total = QString::number(rec["price"].toDouble() * rec["amount"].toInt());
        orderType->append(orderInfo);
        orderDate->append(date);
        orderAmount->append(amount);
        orderPrice->append(price);
        orderTotal->append(total);
    }
}

HomeView::~HomeView()
{
}
