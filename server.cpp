#include "server.h"
#include <QRegularExpression>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QtMath>
#include <QVector>
#include <QtAlgorithms>
server::server(QObject *parent):QTcpServer(parent)
{

    qDebug()<<"started";

    db = QSqlDatabase::addDatabase("QMYSQL");

        db.setHostName("localhost");
        db.setDatabaseName("jeevan");

        db.setUserName("root");
        db.setPassword("");
        if(db.open()){
            qDebug()<<"Database Connected";
        }else{
            qDebug()<<"Database Not Connected";
        }
}

void server::incomingConnection(int socketfd)
{
    QTcpSocket *client = new QTcpSocket(this);
    client->setSocketDescriptor(socketfd);
    clients.insert(client);

    qDebug() << "New client from:" << client->peerAddress().toString();

    connect(client, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
}

void server::readyRead()
{
    QTcpSocket *client = (QTcpSocket*)sender();
    QString code,data,type;
    QSqlQuery query;

        QString line = QString::fromUtf8(client->readLine()).trimmed();
        qDebug() << "Read line:" << line;

        QRegularExpression meRegex("^/type:(\\w)/code:(\\d+)/data:(.*)$");
        QRegularExpressionMatch match = meRegex.match(line);
        if(match.hasMatch())
        {   type = match.captured(1);
            code = match.captured(2);
            qDebug()<<"code :"<<code;
            data = match.captured(3);

            if(code == "4"){

                            data = data.trimmed();
                            QStringList sym = data.split(",");
                            qDebug()<<"sym : "<<sym;
                            QString disease,diet,med;
                            QStringList sym2;
                            int matchCount = 0,maxMatch =0;
                            query.exec("select * from prediction");
                            QSqlError err = query.lastError();
                            qDebug()<<"error:"<<err.text();

                            while(query.next()){
                                matchCount = 0;

                                sym2 = (query.value(1).toString()).split(" ");
                                qDebug()<<"sym2 :"<<sym2;
                                foreach(QString s,sym){

                                    if(sym2.contains(s)){
                                        matchCount++;
                                  }
                                }
                                if(matchCount>maxMatch){
                                    disease = query.value(0).toString();
                                    diet = query.value(2).toString();
                                    med = query.value(3).toString();
                                    maxMatch = matchCount;
                                }
                            }
                            qDebug()<<"match :"<<matchCount<<" :"<<disease;
                            QString data;
                            if(maxMatch != -1)
                                data = "/disease:"+disease+"/diet:"+diet+"/med:"+med;
                            else {
                                data = "0";
                            }
                            client->write(data.toUtf8());
                    }
            else if(code=="1"){
                QRegularExpression meRegex("^/user:(.*)/pass:(.*)$");
                 match = meRegex.match(data);

                qDebug()<<"inn";
                if(match.hasMatch())
                {
                    QString user = match.captured(1);
                    QString pass = match.captured(2);
                    QString q = "select userID,password from users where userID = '"+user;
                    q+="' AND password = '"+pass+"'";
                    query.exec(q);
                    QSqlError err = query.lastError();
                    qDebug()<<"error:"<<err.text();
                    if(query.next()){
                        client->write(QString("1").toUtf8());
                       users[client] = user;
                    }else {
                        client->write(QString("0").toUtf8());
                       users[client] = user;
                    }
                }
            }else if(code == "2"){
                QRegularExpression meRegex("^/user:(.*)/pass:(.*)/phone:(.*)/email:(.*)$");
                 match = meRegex.match(data);
                 if(match.hasMatch())
                 {
                     QString user = match.captured(1);
                     QString pass = match.captured(2);
                     QString phone = match.captured(3);
                     QString email = match.captured(4);
                     QString q = "insert into users values('"+user+"','"+pass+"','"+phone+"','"+email+"')";
                     query.exec(q);
                     QSqlError err = query.lastError();
                     qDebug()<<"error:"<<err.text();
                     client->write(QString("2").toUtf8());
                 }
            }
        else if (code == "0"|| code == "3") {
            QRegularExpression meRegex("^/lat:(.*)/long:(.*)$");
             match = meRegex.match(data);
             if(match.hasMatch())
             {
                 double latitude = (match.captured(1)).toDouble();
                 double longitude = (match.captured(2)).toDouble();
                 QMap<double,QString> dist;
                  QVector<double> distVec;
                 QString q = "select * from hospitals";
                 query.exec(q);
                 QSqlError err = query.lastError();
                 qDebug()<<"error:"<<err.text();
                 while(query.next()){
                    QString hName = query.value(1).toString();
                    double hLatitude = query.value(2).toDouble();
                    double hLongitude = query.value(3).toDouble();
                    double distance = qSqrt(qPow(latitude-hLatitude,2)+qPow(longitude-hLongitude,2));
                    dist[distance] = hName;
                    distVec.push_back(distance);
                 }
                qSort(distVec.begin(),distVec.end());
                if(code == "0"){
                    double near = distVec.at(0);
                    QTcpSocket * hos = onlineHospital.key(dist[near]);

                    hos->write(QString("/pat:"+users[client]+"emergency").toUtf8());
                }else{
                    QString h1("/h1:"+dist[distVec.at(0)]+"/d1:"+QString::number(distVec.at(0)));
                    QString h2("/h2:"+dist[distVec.at(1)]+"/d2:"+QString::number(distVec.at(1)));
                    QString h3("/h3:"+dist[distVec.at(2)]+"/d3:"+QString::number(distVec.at(2)));
                    QString h4("/h4:"+dist[distVec.at(3)]+"/d4:"+QString::number(distVec.at(3)));
                    QString h5("/h5:"+dist[distVec.at(4)]+"/d5:"+QString::number(distVec.at(4)));
                    QString hospitalData = h1+h2+h3+h4+h5;
                    client->write(hospitalData.toUtf8());
                }
             }

        }else if(code == "5"){
                query.exec("select * from users where userID = '" +users[client]+"'");
                if(query.next()){
                    QString user = query.value(0).toString();
                    QString phone = query.value(2).toString();
                    QString email = query.value(3).toString();
                    client->write(QString("/user:"+user+"/phone:"+phone+"/email:"+email).toUtf8());
                }
           }
           if(type=="h"){
                if(code=="1"){
                    QRegularExpression meRegex("^/user:(.*)/pass:(.*)$");
                     match = meRegex.match(data);

                    if(match.hasMatch())
                    {
                        QString user = match.captured(1);
                        QString pass = match.captured(2);
                        QString news = "1234";
                        qDebug()<<"User :"<<user<<" Pass:"<<pass<<" new:"<<news;

                        QString q = "select hospitalID,password from hospitalLogin where hospitalID = '"+user+"' AND password = '"+pass+"'";
                        query.exec(q);
                        QSqlError err = query.lastError();
                        qDebug()<<"error:"<<err.text();
                        if(query.next()){
                            client->write(QString("1").toUtf8());
                           onlineHospital[client] = user;
                        }else {
                            client->write(QString("0").toUtf8());
                        }
                    }
                }
            }
}
}
void server::disconnected()
{
    QTcpSocket *client = (QTcpSocket*)sender();
    qDebug() << "Client disconnected:" << client->peerAddress().toString();

    clients.remove(client);

    QString user = users[client];
    users.remove(client);


}

