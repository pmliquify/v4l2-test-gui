#pragma once

#include <QtNetwork>
#include "image.hpp"

class SocketServer : public QObject
{
    Q_OBJECT
    
public:
    explicit SocketServer(QObject* parent = nullptr);
    ~SocketServer();

signals:
    void imageReceived(const Image &image);
    void disconnected();

private slots:
    void onNewConnection();
    void onDisconnected();
    void readData();

private:
    QTcpServer *    m_server;
    Image *         m_image;
    bool            m_headerReceived;
};
