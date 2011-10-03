#include "main.h"
#include "mb.h"

#define swap16(x) (((x&0xff)<<8)|((x&0xff00)>>8))
#define swap32(x) (((x&0xff)<<8)|((x&0xff00)>>8)|((x&0xff0000)<<8)|((x&0xff000000)>>8))

MBC::MBC(QDomNode node)
{
  dbg("%s %s %s %s %s",
    node.nodeName().toAscii().data(),
    node.toElement().attribute("ip").toAscii().data(),
    node.toElement().attribute("port").toAscii().data(),
    node.toElement().attribute("interval").toAscii().data(),
    node.toElement().attribute("timeout").toAscii().data());

  q = id = 0;

  addr.setAddress(node.toElement().attribute("ip"));
  port = node.toElement().attribute("port").toInt();
  interval = node.toElement().attribute("interval").toInt();
  timeout = node.toElement().attribute("timeout").toInt();
  if (addr.isNull() || !port || !interval || !timeout || !node.childNodes().count())
  { 
    err("n:%s a:%s=%s p:%s=%d t:%s=%d t:%s=%d (%d)",
      node.nodeName().toAscii().data(),
      node.toElement().attribute("ip").toAscii().data(),addr.toString().toAscii().data(),
      node.toElement().attribute("port").toAscii().data(),port,
      node.toElement().attribute("interval").toAscii().data(),interval,
      node.toElement().attribute("timeout").toAscii().data(),timeout,
      node.childNodes().count());
    return;
  }

  for(int i=0;i<node.childNodes().count();i++)
  {
    QDomNode node1 = node.childNodes().item(i);
    dbg("%s %s %s %s %s",
      node1.nodeName().toAscii().data(),
      node1.toElement().attribute("fun").toAscii().data(),
      node1.toElement().attribute("in").toAscii().data(),
      node1.toElement().attribute("out").toAscii().data(),
      node1.toElement().attribute("len").toAscii().data());
    MBQ mbq;
    mbq.fun = node1.toElement().attribute("fun").toInt(0,(node1.toElement().attribute("fun").left(2)=="0x")?16:10);
    mbq.in  = node1.toElement().attribute("in" ).toInt(0,(node1.toElement().attribute("in" ).left(2)=="0x")?16:10);
    mbq.out = node1.toElement().attribute("out").toInt(0,(node1.toElement().attribute("out").left(2)=="0x")?16:10);
    mbq.len = node1.toElement().attribute("len").toInt(0,(node1.toElement().attribute("len").left(2)=="0x")?16:10);
    if (mbq.fun<2 || mbq.fun>6 || (mbq.fun==2 && mbq.len>2000) || (mbq.fun!=2 && mbq.len>125) || (mbq.fun==2 && mbq.out>BAZA_B_MAX) || (mbq.fun!=2 && mbq.out>BAZA_A_MAX))
    {
      err("%s:%d fun:%d len:%d in:%d out:%d/%d,%d",addr.toString().toAscii().data(),port,mbq.fun,mbq.len,mbq.in,mbq.out,BAZA_A_MAX,BAZA_B_MAX);
      return;
    }
    Q << mbq;
    //dbg("%d %d %d %d",Q[i].fun,Q[i].in,Q[i].out,Q[i].len);
  }

  udp = new QUdpSocket(this);
  udp->disconnectFromHost();
  if (!udp->bind(0)) { err("udp bind %s:%d",addr.toString().toAscii().data(),port); return; }
  //if (!connect(udp,SIGNAL(readyRead()),this,SLOT(rx()))) { qDebug("udp connect %s:%d\n",addr.toString().toAscii().data(),port); return; }
  timer = new QTimer(this); connect(timer,SIGNAL(timeout()),this,SLOT(tx())); timer->start(interval);

  dbg("%p %s:%d (%d/%d)",udp,addr.toString().toAscii().data(),port,interval,timeout);
}

void MBC::mb(char fun, short adr, short len, short val)
{
  dbg("MBUS %d %d %d %d",fun,adr,len,val);
  char b[12];
  *(int*)&b[0]=swap32(id);id++;
  *(short*)&b[4]=swap16(6);
  b[6]=1;
  b[7]=fun;
  *(short*)&b[8]=swap16(adr);
  switch(fun)
  {
    case 2:
    case 3:
    case 4: { *(short*)&b[10]=swap16(len); break; }
    case 5:
    case 6: { *(short*)&b[10]=val/*swap16(val)*/; break; }
  }
  udp->writeDatagram((const char*)&b,12,addr,port);
}

void MBC::rx()
{
  int ret=0;
  while (udp->hasPendingDatagrams())
  {
    char b[1500];
    QHostAddress addr2; quint16 port2;
    int size = udp->pendingDatagramSize();
    ret = udp->readDatagram(b,size,&addr2,&port2);
    int fun = b[7]&0xff;
    if (addr != addr2) { err("ADDR %s:%d = %s:%d",addr.toString().toAscii().data(),port,addr2.toString().toAscii().data(),port2); continue; }
    if (port != port2) { err("PORT %s:%d = %s:%d",addr.toString().toAscii().data(),port,addr2.toString().toAscii().data(),port2); continue; }
    if (size != ret) { qDebug("SIZE %s:%d fun:%d=%d len:%d=%d",addr.toString().toAscii().data(),port,fun,Q[q].fun,size,ret); continue; }
    if ((b[7]&0x80)==0x80) { qDebug("MBUS %s:%d fun:%d=%d %02x %02x",addr.toString().toAscii().data(),port,fun,Q[q].fun,b[7]&0xff,b[8]&0xff); continue; }
    if (fun != Q[q].fun) { qDebug("FUN %s:%d fun:%d=%d",addr.toString().toAscii().data(),port,fun,Q[q].fun); continue; }
    dbg("rx %p %s:%d %d=%d %d=%d %d",udp,addr2.toString().toAscii().data(),port2,fun,Q[q].fun,size,ret,q);
    ret -= 9;
    switch (fun)
    {
      case 2:
      case 3:
      case 4:
      {
        short *in = (short*)&b[9]; for (int i=0,w=ret>>1;i<w;i++,in++) if (fun==2) baza.a[Q[q].out+i] = *in; else baza.b[Q[q].out+i] = swap16(*in); //stekot fun 2 error
/*
//      extern int MbusTRX; if (MbusTRX)
        {
          int t; char b[4096],*mem=(char*)&f[mb_scb[q].group][mb_scb[q].word];
          t=sprintf(b,"mbus:"); for (int i=0;i<size;i++) t+=sprintf(b+t," %02x",b[i]&0xff); qDebug(b);
          t=sprintf(b,"memo:"); for (int i=0;i<ret;i++) t+=sprintf(b+t," %02x",mem[i]&0xff); qDebug(b);
        }
*/
        dbg("rx fun234 %d",fun);
        break;
      }
      case 5:
      case 6:
      {
/*
        if (memcmp(b1,b,12))
        {
          int t=0; char bb[4096];
          t+=sprintf(b+t,"b1"); for (int i=0;i<12;i++) t+=sprintf(b+t," %02x",b1[i]&0xff); t+=sprintf(b+t,"\n");
          t+=sprintf(b+t,"b2"); for (int i=0;i<12;i++) t+=sprintf(b+t," %02x",b2[i]&0xff); t+=sprintf(b+t,"\n");
          t+=sprintf(b+t,"SCB: %d %s:%d FUN %d RESPONSE ERROR",nr,addr.toString().toAscii().data(),port,fun);
          qDebug(b);
          goto mb_repeat;
        }
        qDebug("SCB: %d %s:%d FUN %d RESPONSE OK",nr,addr.toString().toAscii().data(),port,fun);
*/
        dbg("rx fun56 %d",fun);
        break;
      }
    }
  }
  T=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

void MBC::tx()
{
  dbg("tx %p %s:%d (%d/%d) %d",udp,addr.toString().toAscii().data(),port,interval,timeout,q);
  rx();
  if(++q>=Q.count())q=0;
  mb(Q[q].fun,Q[q].in,Q[q].len,0);
}

MBS::MBS(int port)
{
  udp = new QUdpSocket(this); //connect(tcp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(err(QAbstractSocket::SocketError)));
  udp->disconnectFromHost();
  if (!udp->bind(port)) { err("modbus server udp %d bind",port); return; }
  if (!connect(udp,SIGNAL(readyRead()),this,SLOT(rx()))) { err("modbus server udp %d connect rx",port); return; }
  dbg("modbus server udp %d start",port);
}

void MBS::rx()
{
  while (udp->hasPendingDatagrams())
  {
    QHostAddress host; quint16 port;
    char data[1500]; int size = udp->pendingDatagramSize();
    udp->readDatagram(data,size,&host,&port);
    dbg("modbus server %s:%d %d",host.toString().toAscii().data(),port,size); //for (int i=0;i<size;i++) printf(" %02x",data[i]&0xff); printf("\n");
    if (size!=6+swap16(*(short*)&data[4])) { err("modbus server size(%d)=@4:0x%04x",size,swap16(*(short*)&data[4])); continue; }
    short fun = data[7];
    short adr = swap16(*(short*)&data[8]);
    short len = swap16(*(short*)&data[10]);
    switch(fun)
    {
      case 1:
      break;
      case 2:
      break;
      case 3:
      break;
      case 4:
      {
        char buf[259];
        *(int*)&buf[0] = *(int*)&data[0];
        *(short*)&buf[4] = swap16((len<<1)+3);
        buf[6] = 1;
        buf[7] = 4;
        buf[8] = len<<1;
        short * dst = (short*)&buf[9];
        short * src = (fun==2)?(short*)&baza.b:(short*)&baza.a;
        for (int i=0;i<len;i++,src++,dst++) *dst = swap16(*src);
        udp->writeDatagram((const char *)&buf,(len<<1)+9,host,port);
        dbg("modbus server %d 0x%04x(%d) %x(%d)",fun,adr,adr,len,len);
        break;
      }
      case 5:
      case 15:
      case 16:
      {
        dbg("modbus server %d 0x%04x(%d) %x(%d)",fun,adr,adr,len,len);
        break;
      }
      default:
      {
        char buf[9];
        *(int*)&buf[0] = *(int*)&data[0];
        *(short*)&buf[4] = swap16(3);
        buf[6] = 1;
        buf[7] = 0x84;
        buf[8] = 2;
        udp->writeDatagram((const char *)&buf,9,host,port);
        err("modbus server fun %d 0x%04x(%d) %x(%d)",fun,adr,adr,len,len);
      }
    }
  }
}

/*
MBV::MBV(QDomNode node, QWidget *parent) : QWidget(parent)
{
  H = new QHBoxLayout;
  for (int h=0;h<node.nodeChild().count();h++)
  {
    V[h] = new QVBoxLayout;
    T[h] = new QLabel("0000-00-00 00:00:00.000");
    T[h]->setAlignment(Qt::AlignCenter);
    V[h]->addWidget(T[h]);
    t[h][0] = new QTableWidget(2,8);
    t[h][1] = new QTableWidget(1250,8);
    t[h][2] = new QTableWidget(138,8);
    for (int v=0;v<3;v++)
    {
      if (v==0) t[h][v]->setMaximumHeight(90);
      for (int r=0;r<t[h][v]->rowCount();r++) for (int c=0;c<t[h][v]->columnCount();c++)
      {
        if (c==0)
        {
          t[h][v]->setRowHeight(r,20);
          t[h][v]->setVerticalHeaderItem(r,new QTableWidgetItem(tr("%1").arg(r<<3)));
        }    
        if (r==0)
        {
          t[h][v]->setColumnWidth(c,50);
          t[h][v]->setHorizontalHeaderItem(c,new QTableWidgetItem(tr("%1").arg(c,4,10)));
        }
        t[h][v]->setItem(r,c,new QTableWidgetItem(tr("%1").arg(0,4,16,QLatin1Char('0'))));
        t[h][v]->item(r,c)->setTextAlignment(Qt::AlignCenter);
      }
      V[h]->addWidget(t[h][v]);
    }
    H->addLayout(V[h]);
  }
  setLayout(H);
  setWindowTitle("SCB MODBUS");
  resize(1000,800);
  //move(CENTER);
  show();
  timer = new QTimer(this); connect(timer, SIGNAL(timeout()), this, SLOT(up())); timer->start(1000);
}

void MBV::up()
{
  extern MBUScli * mbus[4];
  for (int h=0;h<4;h++) if (mbus[h])
  {
    if (!mbus[h]->T.isNull()) T[h]->setText(mbus[h]->T);
    for (int v=0;v<3;v++) for (int r=0;r<t[h][v]->rowCount();r++) for (int c=0;c<t[h][v]->columnCount();c++)
    {
      unsigned short val = mbus[h]->f[v][(r<<3)|c];
      t[h][v]->item(r,c)->setBackground((t[h][v]->item(r,c)->text().toUShort(0,16)==val)?QBrush(QColor(Qt::lightGray)):QBrush(QColor(Qt::green)));
      t[h][v]->item(r,c)->setText(tr("%1").arg(val&0xffff,4,16,QLatin1Char('0')));
    }
  }
}

#include <QDesktopWidget>
#include <QApplication>
#include <QtGui>
#include "mbus.h"

MBUScli * mbus[4];

int main(int argc,char **argv)
{
  QApplication a(argc,argv);
  QTextCodec::setCodecForTr       (QTextCodec::codecForName("Windows-1250"));
  QTextCodec::setCodecForCStrings (QTextCodec::codecForName("Windows-1250"));
  QTextCodec::setCodecForLocale   (QTextCodec::codecForName("Windows-1250"));
  for(int i=0;i<4;i++) mbus[i] = new MBUScli(i+1);
  new MBUSview;
  a.exec();
  return 0;
}
*/
