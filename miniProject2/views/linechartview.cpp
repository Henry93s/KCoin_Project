#include "linechartview.h"

#include <QDateTimeAxis>

LineChartView::LineChartView(QObject *parent)
    : QObject{parent}
{
    // 데이터 매니저 설정 - Devwooms
    lineDataManager = new LineChartDataManager;
    lineCoin = "krw-btc";
    lineDataManager->setting(lineCoin);
    lineDataManager->start();

    // 데이터 매니저에서 데이터 수신 처리 - Devwooms
    connect(lineDataManager, &LineChartDataManager::lineData, this, &LineChartView::getLineData);

    // Line 그래프 생성 - devwooms
    lineChart = new QChart();
    // 차트 뷰 생성 - devwooms
    lineChartView = new QChartView(lineChart);
    lineChartView->setRenderHint(QPainter::Antialiasing);
    lineChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 선 series - Devwooms
    lineSeries = new QLineSeries;
    lineChart->addSeries(lineSeries);
    lineChart->createDefaultAxes();
}

// 코인 변경 - Devwooms
void LineChartView::setCoin(const QString &coin){
    lineDataManager->setting(coin);
    lineCoin = coin;
}

// 데이터 매니저에서 데이터 연결 - Devwooms
void LineChartView::getLineData(const QList<QPointF> &lineDataList){
    if (!lineDataList.isEmpty()) {
        updateLineData(lineDataList);
    }
}

void LineChartView::updateLineData(const QList<QPointF> &lineDataList){
    
    // 기존 데이터 클리어 - Devwooms
    lineSeries->clear();
    
    // 시간 범위 찾기 - Devwooms
    // min -> 가장 오래된 시간 - Devwooms
    // max -> 가장 최신 시간 - Devwooms
    double minTime = lineDataList.last().x();
    double maxTime = lineDataList.first().x();
    
    // Y축 값 범위 찾기 및 스케일링 - Devwooms
    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::lowest();

    // 단위 스케일링을 위한 설정 - Devwooms
    // 기본 단위 - Devwooms
    QString unitLabel = "원";
    double scaleFactor = 1.0;

    // 첫번째 값을 받아오기 - Devwooms
    double firstPrice = lineDataList.first().y();

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
    
    // 데이터를 만원 단위로 스케일링하여 차트에 추가 - Devwooms
    for (const QPointF &point : lineDataList) {
        // 단위에 따라서 지정된 scaleFactor로 나누기 ( 천만원단위 ) - Devwooms
        double scaledPrice = point.y() / scaleFactor;
        minPrice = qMin(minPrice, scaledPrice);
        maxPrice = qMax(maxPrice, scaledPrice);
        
        // 시간을 상대적 위치로 변환 (0~100 범위) - Devwooms
        double relativeTime = ((point.x() - minTime) / (maxTime - minTime)) * 100;
        lineSeries->append(relativeTime, scaledPrice);
    }
    
    // 차트 축 업데이트 - Devwooms
    if (lineChart) {
        // 기존 X축 제거 - Devwooms
        QList<QAbstractAxis*> axes = lineChart->axes(Qt::Horizontal);
        for (QAbstractAxis* axis : std::as_const(axes)) {
            lineChart->removeAxis(axis);
        }
        
        // 새로운 X축 생성 (3개 라벨만 표시) - Devwooms
        QCategoryAxis *axisX = new QCategoryAxis();
        axisX->setRange(0, 100);
        
        // 시작시간(오래된), 중간시간, 끝시간(최신) 라벨 추가 - Devwooms
        QDateTime startTime = QDateTime::fromMSecsSinceEpoch(minTime);
        QDateTime midTime = QDateTime::fromMSecsSinceEpoch((minTime + maxTime) / 2);
        QDateTime endTime = QDateTime::fromMSecsSinceEpoch(maxTime);
        
        axisX->append(startTime.toString("hh:mm:ss"), 0);
        axisX->append(midTime.toString("hh:mm:ss"), 50);
        axisX->append(endTime.toString("hh:mm:ss"), 100);
        
        axisX->setLabelsAngle(-45);

        
        // 새로운 X축 추가 - Devwooms
        lineChart->addAxis(axisX, Qt::AlignBottom);
        lineSeries->attachAxis(axisX);
        
        // Y축 업데이트 (만원 단위) - Devwooms
        QList<QAbstractAxis*> yAxes = lineChart->axes(Qt::Vertical);
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
        double latestPrice = lineDataList.last().y();
        QString priceStr = QString::number(latestPrice / scaleFactor, 'f', (isDecimalCoin ? 5 : 1));
        QString title = QString("%1 실시간 차트 (최신: %2%3, %4)")
                            .arg(lineCoin)
                            .arg(priceStr)
                            .arg(unitLabel)
                            .arg(latestTime.toString("hh:mm:ss"));
        lineChart->setTitle(title);

        // line 차트에서 실시간 가격 확인
        // qDebug() << priceStr;
    }

    // LineChartView.cpp의 updateLineData 함수 마지막에
    if (!lineDataList.isEmpty()) {
        latestPrice = lineDataList.last().y(); // 마지막이 최신
    }
}

// double LineChartView::getLatestPrice() const{
//     return latestPrice;
// }
