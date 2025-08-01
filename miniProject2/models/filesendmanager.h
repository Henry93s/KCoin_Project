#ifndef FILESENDMANAGER_H
#define FILESENDMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

class FileSendManager : public QObject
{
    Q_OBJECT
public:
    explicit FileSendManager(QObject *parent = nullptr);
    void sendFile(QTcpSocket *socket, QString type, QString& chatViewName, QStringList filePaths, const QString& senderName, const QString& senderID);

private:
    bool sendSingleFileAsJson(QTcpSocket *socket, const QString& filePath, const QString& chatViewName, const QString& type, const QString& senderName, const QString& senderID);
    
    // 헬퍼 함수들
    QString getMimeType(const QString& extension);
    QString calculateFileChecksum(const QString& filePath);

signals:
};

#endif // FILESENDMANAGER_H
