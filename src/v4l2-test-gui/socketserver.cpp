#include "socketserver.hpp"
#include "sockettypes.hpp"


SocketServer::SocketServer(QObject* parent) :
    QObject(parent), 
    m_server(new QTcpServer(this)),
    m_image(new Image()),
    m_headerReceived(false)
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
        m_headerReceived = false;

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
            if (!m_headerReceived) {
                if (clientSocket->bytesAvailable() < sizeof(ImageHeader)) {
                    return; 
                }
                m_headerReceived = true;
                
                QByteArray data = clientSocket->read(sizeof(ImageHeader));
                ImageHeader* header = reinterpret_cast<ImageHeader*>(data.data());
                if (m_image->imageSize() != header->imageSize) {
                    if (m_image->imageSize() != 0) {
                            for (int index=0; index<m_image->planes().size(); index++) {
                                    free(m_image->planes().at(index));
                            }
                    }
                        m_image->planes().resize(header->numPlanes);
                        m_image->setImageSize(header->imageSize);
                    for (int index=0; index<m_image->planes().size(); index++) {
                            m_image->planes()[index] = (unsigned char *)malloc(m_image->imageSize());
                    }
                }

                m_image->init(header->width, header->height, header->bytesPerLine, header->imageSize,
                    header->bytesUsed, header->pixelformat, header->sequence, header->timestamp);
                m_image->setShift(header->shift);

            } else {
                unsigned int receiveSize = (m_image->bytesUsed() > 0) ? m_image->bytesUsed() : m_image->imageSize();
                if (clientSocket->bytesAvailable() < receiveSize) {
                    return; 
                }
                m_headerReceived = false;

                unsigned char *plane = (unsigned char *)m_image->planes()[0];
                int size = clientSocket->read((char *)plane, receiveSize);
                if (size < 0) {
                    clientSocket->close();
                    return;
                }
                
                emit imageReceived(*m_image);
            }
        }
    }
}
