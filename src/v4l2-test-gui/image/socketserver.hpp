// Copyright (c) 2026 Peter Martienssen
// SPDX-License-Identifier: MIT

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

public slots:
    void listen(int port);

private slots:
    void onNewConnection();
    void onDisconnected();
    void readData();

private:
    QTcpServer *    m_server;
    Image *         m_image;

    enum State {
        WAITING_FOR_HEADER,
        WAITING_FOR_PLANE_SIZE,
        WAITING_FOR_PLANE_DATA
    };
    State           m_state;
    int             m_currentPlane;
    unsigned int    m_planeSize;
};
