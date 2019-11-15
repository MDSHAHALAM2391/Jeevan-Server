#include <QCoreApplication>
#include "server.h"
#include <QHostAddress>
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    server *s = new server();
    QHostAddress add = QHostAddress::AnyIPv4;
   s->listen(QHostAddress::LocalHost,4200);
   qDebug()<<"addre"<<s->serverAddress();
    return a.exec();

}
