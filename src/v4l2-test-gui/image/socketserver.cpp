#include "socketserver.hpp"
#include "sockettypes.hpp"

SocketServer::SocketServer(QObject* parent) :
    QObject(parent), 
    m_server(new QTcpServer(this)),
    m_image(new Image()),
    m_state(WAITING_FOR_HEADER),
    m_currentPlane(0),
    m_planeSize(0)
{
    connect(m_server, &QTcpServer::newConnection, this, &SocketServer::onNewConnection);
}

SocketServer::~SocketServer()
{
    delete m_image;
    m_image = NULL;
}

void SocketServer::listen(int port)
{
    m_server->close();
    if (!m_server->listen(QHostAddress::Any, port)) {
        qDebug() << tr("Unable to bind port %1").arg(m_server->errorString());
    } else {
        qDebug() << tr("Listen on port %1").arg(m_server->serverPort());
    }
}

void SocketServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* clientSocket = m_server->nextPendingConnection();

        connect(clientSocket, &QTcpSocket::readyRead, this, &SocketServer::readData);
        connect(clientSocket, &QTcpSocket::disconnected, this, &SocketServer::onDisconnected);

        m_state = WAITING_FOR_HEADER;
        m_currentPlane = 0;
        m_planeSize = 0;

        qDebug() << tr("Client connected!");
    }
}

void SocketServer::onDisconnected()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        emit disconnected();
        clientSocket->close();
        clientSocket->deleteLater();

        qDebug() << tr("Client disconnected!");
    }
}

void SocketServer::readData()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        while (true){
            switch (m_state) {
            case WAITING_FOR_HEADER:
                if (clientSocket->bytesAvailable() < sizeof(ImageHeader)) {
                    return; 
                }
                {
                    ImageHeader header;
                    int size = clientSocket->read((char *)&header, sizeof(header));
                    m_image->init(header.width, header.height, 
                        header.pixelformat, 
                        header.imageSize, header.bytesPerLine, 
                        header.sequence, header.timestamp);
                    m_image->setShift(header.shift);
                    m_image->planes().resize(header.numPlanes);
                }

                m_currentPlane = 0;
                m_planeSize = 0;
                m_state = WAITING_FOR_PLANE_SIZE;
                break;

            case WAITING_FOR_PLANE_SIZE:
                if (clientSocket->bytesAvailable() < sizeof(m_planeSize)) {
                    return; 
                }
                clientSocket->read(reinterpret_cast<char*>(&m_planeSize), sizeof(m_planeSize));
                m_state = WAITING_FOR_PLANE_DATA;
                break;

            case WAITING_FOR_PLANE_DATA:
                if (clientSocket->bytesAvailable() < m_planeSize) {
                    return; 
                }
                m_image->plane(m_currentPlane).init(m_planeSize);
                unsigned char *plane = (unsigned char *)m_image->plane(m_currentPlane).data();
                int size = clientSocket->read((char *)plane, m_planeSize);
                if (size < 0) {
                    clientSocket->close();
                    return;
                }
                m_currentPlane++;
                if (m_currentPlane < m_image->planes().size()) {
                    m_planeSize = 0;
                    m_state = WAITING_FOR_PLANE_SIZE;

                } else {
                    m_state = WAITING_FOR_HEADER;
                    emit imageReceived(*m_image);
                }
                break;
            }
        }
    }
}
