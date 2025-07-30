#include "linechartdatamanager.h"

LineChartDataManager::LineChartDataManager(QObject *parent)
    : QObject{parent}
{
    // 네트워크 매니저 생성 - Devwooms
    manager = new QNetworkAccessManager(this);
    
    // 응답 시 연결 - Devwooms
    connect(manager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        // 답이 에러인지 확인 후 모두 읽어 오기 - Devwooms
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument json = QJsonDocument::fromJson(responseData);
            dataFormat(json);
        } else {
            qWarning() << "API 요청 에러:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

// URL 및 HTTP 등등 세팅 - devwooms
void LineChartDataManager::setting(QString coin)
{
    // 요청 URL 지정 - devwooms
    QUrl apiUrl("https://api.bithumb.com/v1/trades/ticks?market="+coin+"&count=100");
    request.setUrl(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // HTTP/2 문제 해결을 위한 설정 - devwooms
    // 서버에 "정상적인 클라이언트입니다" 라고 알려줌 - Devwooms
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qt Application");
    // HTTP/1.1 강제 사용 - Devwooms
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    // HTTP 파이프라이닝은 여러 요청을 한번에 보내는 방식 - Devwooms
    // 순차 처리 강제 - Devwooms
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);
    // Keep-alive 비활성화 - Devwooms
    request.setRawHeader("Connection", "close");
}

// 데이터 요청 시작 -devwooms
void LineChartDataManager::start()
{
    timer = new QTimer(this);
    timer->start(1000);
    // timer를 update에 연결 - Devwooms
    connect(timer, &QTimer::timeout, this, &LineChartDataManager::update);
}

void LineChartDataManager::update()
{
    manager->get(request);
}

void LineChartDataManager::dataFormat(QJsonDocument json){
    QJsonArray dataArray = json.array();
    
    // 거래가와 시간 데이터를 담을 리스트 생성 - Devwooms
    QList<QPointF> lineDataList;
    
    // 각 거래 데이터 파싱 - Devwooms
    for (int i = 0; i < dataArray.size(); ++i) {
        QJsonObject serverData = dataArray[i].toObject();
        
        // 거래가와 시간 정보 추출 - Devwooms
        double tradePrice = serverData["trade_price"].toDouble();
        double timestamp = serverData["timestamp"].toDouble();
        
        // QPointF로 데이터 저장 (x: 시간, y: 거래가) - Devwooms
        QPointF tradePoint(timestamp, tradePrice);
        lineDataList.append(tradePoint);

    }

    // 데이터 전달 -> lineChart로 - Devwooms
    emit lineData(lineDataList);
}
