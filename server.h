#ifndef SERVER_H
#define SERVER_H
#include <QTcpServer>
#include <QTcpSocket>
#include <QSet>
#include <QMap>
#include <QSqlDatabase>
class server:public QTcpServer
{   Q_OBJECT
public:
    server(QObject *parent=nullptr);

private slots:
    void readyRead();
    void disconnected();

protected:
    void incomingConnection(int socketfd);
private:
    QSet<QTcpSocket *> clients;
    QMap<QTcpSocket*,QString> users;
    QSet<QTcpSocket *> hospitals;
    QMap<QTcpSocket*,QString> onlineHospital;
    QSqlDatabase db;
};

#endif // SERVER_H
