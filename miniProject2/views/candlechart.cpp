#include "candlechart.h"

#include "models/candlechartdatamanager.h"
#include <QDateTimeAxis>
#include <QCandlestickSet>
#include <algorithm>

CandleChart::CandleChart(QObject *parent)
    : QObject{parent}
{
    // 데이터 매니저 설정 - Devwooms
    candleDataManager = new CandleChartDataManager;
    candleCoin = "krw-btc";
    candleDataManager->setting(candleCoin);
    candleDataManager->start();

    // 데이터 매니저에서 데이터 수신 처리 - Devwooms
    connect(candleDataManager, &CandleChartDataManager::candleData, this, &CandleChart::getCandleData);

    // Candle 그래프 생성 - devwooms
    candleChart = new QChart();
    // 차트 뷰 생성 - devwooms
    candleChartView = new QChartView(candleChart);
    candleChartView->setRenderHint(QPainter::Antialiasing);
    candleChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 캔들스틱 series - Devwooms
    candleSeries = new QCandlestickSeries;
    candleChart->addSeries(candleSeries);
    candleChart->createDefaultAxes();
}

// 코인 변경 - Devwooms
void CandleChart::setCoin(const QString &coin){
    candleDataManager->setting(coin);
    candleCoin = coin;
}

// 데이터 매니저에서 데이터 연결 - Devwooms
void CandleChart::getCandleData(const QList<CandleData> &candleDataList){
    if (!candleDataList.isEmpty()) {
        updateCandleData(candleDataList);
    }
}

void CandleChart::updateCandleData(const QList<CandleData> &candleDataList){
    
    // 기존 데이터 클리어 - Devwooms
    candleSeries->clear();
    
    // 시간 범위 찾기 - Devwooms
    // min -> 가장 오래된 시간 - Devwooms
    // max -> 가장 최신 시간 - Devwooms
    double minTime = candleDataList.last().time;
    double maxTime = candleDataList.first().time;
    
    // Y축 값 범위 찾기 및 스케일링 - Devwooms
    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::lowest();

    // 단위 스케일링을 위한 설정 - Devwooms
    // 기본 단위 - Devwooms
    QString unitLabel = "원";
    double scaleFactor = 1.0;

    // 첫번째 값을 받아오기 - Devwooms
    double firstPrice = candleDataList.first().trade;

    // 자리수 측정 - Devwooms
    int digitCount = static_cast<int>(std::floor(std::log10(std::abs(firstPrice)))) + 1;

    // 너무 큰 단위는 10000으로 나눈 후 만원을 붙이기 위해서 - Devwooms
    if (digitCount > 5) {
        scaleFactor = 10000.0;
        unitLabel = "만원";
    } else if (firstPrice < 1.0) {
        scaleFactor = 1;
        unitLabel = "원";
    }
    
    // 소수점 코인인지 확인하는 플래그 추가 - Devwooms
    bool isDecimalCoin = (firstPrice < 1.0);
    
    // 캔들스틱 데이터를 차트에 추가 - Devwooms
    for (const CandleData &candleData : candleDataList) {
        // 단위에 따라서 지정된 scaleFactor로 나누기 ( 천만원단위 ) - Devwooms
        double scaledOpen = candleData.open / scaleFactor;
        double scaledHigh = candleData.high / scaleFactor;
        double scaledLow = candleData.low / scaleFactor;
        double scaledClose = candleData.trade / scaleFactor;
        
        minPrice = std::min({minPrice, scaledOpen, scaledHigh, scaledLow, scaledClose});
        maxPrice = std::max({maxPrice, scaledOpen, scaledHigh, scaledLow, scaledClose});
        
        // 캔들스틱 세트 생성 (실제 timestamp 사용) - Devwooms
        QCandlestickSet *candleSet = new QCandlestickSet(candleData.time);
        candleSet->setOpen(scaledOpen);
        candleSet->setHigh(scaledHigh);
        candleSet->setLow(scaledLow);
        candleSet->setClose(scaledClose);
        
        candleSeries->append(candleSet);
    }
    
    // 차트 축 업데이트 - Devwooms
    if (candleChart) {
        // 기존 X축 제거 - Devwooms
        QList<QAbstractAxis*> axes = candleChart->axes(Qt::Horizontal);
        for (QAbstractAxis* axis : std::as_const(axes)) {
            candleChart->removeAxis(axis);
        }
        
        // 새로운 DateTime X축 생성 - Devwooms
        QDateTimeAxis *axisX = new QDateTimeAxis();
        
        // 시간 범위 설정 - Devwooms
        QDateTime startTime = QDateTime::fromMSecsSinceEpoch(minTime);
        QDateTime endTime = QDateTime::fromMSecsSinceEpoch(maxTime);
        
        axisX->setRange(startTime, endTime);
        axisX->setFormat("hh:mm:ss");
        axisX->setLabelsAngle(-45);
        
        // 새로운 X축 추가 - Devwooms
        candleChart->addAxis(axisX, Qt::AlignBottom);
        candleSeries->attachAxis(axisX);
        
        // Y축 업데이트 (만원 단위) - Devwooms
        QList<QAbstractAxis*> yAxes = candleChart->axes(Qt::Vertical);
        if (!yAxes.isEmpty()) {
            QValueAxis *axisY = qobject_cast<QValueAxis*>(yAxes.first());
            if (axisY) {
                // 소수점 코인의 경우 더 작은 margin 적용 - Devwooms
                double minMargin = isDecimalCoin ? qMax(maxPrice * 0.01, 0.00001) : 0.0001;
                const double margin = qMax((maxPrice - minPrice) * 0.05, minMargin); // 5% 여유
                
                // 범위 설정  - Devwooms
                axisY->setRange(minPrice - margin, maxPrice + margin);

                QString formatUsed;
                if (scaleFactor == 10000.0) {
                    axisY->setLabelFormat("%.0f");  // 만원 단위 정수 - Devwooms
                    formatUsed = "%.0f (만원 단위)";
                } else if (isDecimalCoin) {
                    axisY->setLabelFormat("%.5f");  // 소수점 5자리까지 - Devwooms
                    formatUsed = "%.5f (소수점 5자리)";
                } else {
                    axisY->setLabelFormat("%.1f");
                    formatUsed = "%.1f (소수점 1자리)";
                }
                axisY->setLabelsAngle(-45);
            }

        }
        
        // 차트 제목 업데이트 ( 어떤 코인 / 실시간 가격 / 단위 / 시간 ) - Devwooms
        QDateTime latestTime = QDateTime::fromMSecsSinceEpoch(maxTime);
        double latestPrice = candleDataList.last().trade;
        QString priceStr = QString::number(latestPrice / scaleFactor, 'f', (isDecimalCoin ? 5 : 1));
        QString title = QString("%1 캔들차트 (최신: %2%3, %4)")
                            .arg(candleCoin)
                            .arg(priceStr)
                            .arg(unitLabel)
                            .arg(latestTime.toString("hh:mm:ss"));
        candleChart->setTitle(title);


        // candle 차트에서 실시간 가격 확인
        // qDebug() << priceStr;
    }
    
}
