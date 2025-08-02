#include "loginview.h"
#include "models/sendingManage.h"
#include "models/socketManage.h"
#include "mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QIcon>
#include <QFont>
#include <QTcpSocket>

LoginView::LoginView(QWidget *parent) : QWidget(parent)
{
    setupUI();
    connectSignals();
    setupSocketConnection();
}

//==========================
//        소켓 연결
//==========================
void LoginView::setupSocketConnection()
{
    // 소켓 연겴 초기화
    // socket Manage에서 생성된 socket 객체 가져오기
    QTcpSocket *socket = SocketManage::instance().socket();
    // 소켓 연결 시도 (LoginView 생성 시점에 한 번만 시도)
    // 이미 연결되어 있다면 다시 연결 시도하지 않음
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        // 192.168.2.235 => devwooms
        // 192.168.1.44 => donjizzkan
        // 192.168.2.207 => hyungoo
        socket->connectToHost("192.168.2.207", 51234);
        // socket->connectToHost("192.168.2.26", 51234);
        // 192.168.2.190 => geonwoo

        // 서버 연결 완료 시그널은 SocketManage에서 처리하므로 여기서 직접 연결할
        // 필요 없음 connect(socket, &QTcpSocket::connected, this, [](){
        //     qDebug("서버 연결 완료");
        // });
    }

}


//==========================
// 통과 신호(로그인 응답) 받으면 메인화면으로
//==========================
// SocketManage에서 방출하는 loginResponseReceived 시그널을 받도록 변경
void LoginView::handleLoginResponse(const QJsonObject &obj)
{
    qDebug() << "LoginView: loginResponseReceived 시그널 받음";
    // 이 obj는 이미 SocketManage에서 파싱된 JSON 객체임
    if (obj.value("type").toString() == "response") { // 타입이 'response'일 경우
        QString result = obj.value("result").toString();
        // 로그인 성공/실패에 따라 UI 전환
        if (result == "success") {
            // 해당 로그인 정보에 해당하는 이름 받아 옴
            // geonwoo ID 를 sendingManage 에 추가 관리
            QJsonObject userObj = obj.value("user").toObject();  // "user" 객체 추출
            QString ID = userObj.value("ID").toString();  // "ID" 추출

            QString senderName = obj.value("name").toString();
            QJsonObject userInfo = obj["user"].toObject();
            QJsonArray history = obj["history"].toArray();

            qDebug() << ID << "[" << senderName << "]" << " 으로 로그인 성공";
            // 이전 : sendingManage에 있는 이름 setter로 내 이름 저장
            // 파일이나 메세지 전송시 저장된 senderName 불러옴
            // 이후 : setter 로 내 ID, 이름 저장 및 파일이나 메시지 전송 시 저장된 ID 로 불러옴
            // 출력은 동일(이름)
            sendingManage::instance()->setSenderID(ID);

            sendingManage::instance()->setSenderName(senderName);
            emit loginSuccessWithInfo(userInfo, history);   // 유저 정보 띄우기
            emit goToMain(); // 메인 화면으로 이동
        } else {
            QMessageBox::warning(this, "로그인 실패", "ID 또는 PW가 틀렸습니다.");
            qDebug() << "로그인 실패: ID 또는 PW 불일치";
        }
    }
}

void LoginView::clearInputFields()
{
    pw_LineEdit->clear();
    pw_LineEdit->setFocus();
}

void LoginView::connectSignals()
{
    // 로그인 응답 처리
    connect(&SocketManage::instance(), &SocketManage::loginResponseReceived, this, &LoginView::handleLoginResponse);

    // UI 이벤트 연결
    connect(pw_LineEdit, &QLineEdit::returnPressed, this, [this]() { login_Button->click(); });
    connect(signup_Button, &QPushButton::clicked, this, &LoginView::goToSignup);
    connect(login_Button, &QPushButton::clicked, this, &LoginView::doSignIn);
}

void LoginView::setupUI()
{
    // 메인 레이아웃 설정
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QVBoxLayout *centerLayout = new QVBoxLayout();
    
    // 상단 여백
    centerLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    // 타이틀 영역 생성
    centerLayout->addLayout(createTitleLayout());
    
    // 로그인 폼 생성
    centerLayout->addLayout(createLoginFormLayout());
    
    // 하단 여백
    centerLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    mainLayout->addLayout(centerLayout);
}

QHBoxLayout* LoginView::createTitleLayout()
{
    QHBoxLayout *titleLayout = new QHBoxLayout();
    
    // 좌우 여백
    titleLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    
    // 타이틀 컴포넌트들
    QHBoxLayout *titleComponents = new QHBoxLayout();
    
    // "Down Bit" 레이블
    QLabel *titleLabel = new QLabel("Down Bit");
    QFont titleFont;
    titleFont.setPointSize(30);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleComponents->addWidget(titleLabel);
    
    // 비트코인 아이콘
    QLabel *iconLabel = new QLabel();
    iconLabel->setMaximumSize(100, 100);
    iconLabel->setPixmap(QPixmap(":/assets/assets/Bitcoin.png"));
    iconLabel->setScaledContents(true);
    titleComponents->addWidget(iconLabel);
    
    titleLayout->addLayout(titleComponents);
    titleLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    
    return titleLayout;
}

QHBoxLayout* LoginView::createLoginFormLayout()
{
    QHBoxLayout *formLayout = new QHBoxLayout();
    
    // 좌우 여백
    formLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    
    // 입력 필드들
    QVBoxLayout *inputLayout = new QVBoxLayout();
    
    // 폰트 설정
    QFont inputFont;
    inputFont.setPointSize(20);
    
    // ID 입력 필드
    id_LineEdit = new QLineEdit();
    id_LineEdit->setMinimumHeight(30);
    id_LineEdit->setFont(inputFont);
    id_LineEdit->setPlaceholderText("아이디를 입력하세요");
    inputLayout->addWidget(id_LineEdit);
    
    // 비밀번호 입력 필드
    pw_LineEdit = new QLineEdit();
    pw_LineEdit->setMinimumHeight(30);
    pw_LineEdit->setFont(inputFont);
    pw_LineEdit->setEchoMode(QLineEdit::Password);
    pw_LineEdit->setPlaceholderText("비밀번호를 입력하세요");
    inputLayout->addWidget(pw_LineEdit);
    
    // 버튼 레이아웃
    inputLayout->addLayout(createButtonLayout());
    
    formLayout->addLayout(inputLayout);
    formLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    
    return formLayout;
}

QHBoxLayout* LoginView::createButtonLayout()
{
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    // 버튼 폰트 설정
    QFont buttonFont;
    buttonFont.setPointSize(15);
    buttonFont.setBold(true);
    
    // 회원가입 버튼
    signup_Button = new QPushButton("회원가입");
    signup_Button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    signup_Button->setMinimumSize(200, 30);
    signup_Button->setMaximumSize(200, 30);
    signup_Button->setFont(buttonFont);
    buttonLayout->addWidget(signup_Button);
    
    // 로그인 버튼
    login_Button = new QPushButton("로그인");
    login_Button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    login_Button->setMinimumSize(200, 30);
    login_Button->setMaximumSize(200, 30);
    login_Button->setFont(buttonFont);
    login_Button->setDefault(true); // 기본 버튼으로 설정
    buttonLayout->addWidget(login_Button);
    
    return buttonLayout;
}

LoginView::~LoginView() { }

QString LoginView::getInsertedID() const
{
    return id_LineEdit->text().trimmed();
}

QString LoginView::getInsertedPW() const
{
    return pw_LineEdit->text();
}
