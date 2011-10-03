#ifndef MB_H
#define MB_H

#include <QtGui>
#include <QtNetwork>
#include <QDomDocument>

struct MBQ
{
  int fun;
  int in;
  int out;
  int len;
};

class MBC : public QThread
{
	Q_OBJECT
public:
  MBC(QDomNode);
  QHostAddress addr;
  int port;
  int interval;
  int timeout;
	QUdpSocket *udp;
	QTimer *timer;
  QList<MBQ> Q;
  int q;
  int id;
  QString T;
public slots:
	void mb(char,short,short,short);
	void tx();
  void rx();
};

class MBS : public QThread
{
	Q_OBJECT
public:
  MBS(int);
	QUdpSocket *udp;
public slots:
  void rx();
};

#endif
