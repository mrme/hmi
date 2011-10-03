#include <QtGui>
#include "mbus.h"

#define swap16(x) (((x&0xff)<<8)|((x&0xff00)>>8))
#define swap32(x) (((x&0xff)<<8)|((x&0xff00)>>8)|((x&0xff0000)<<8)|((x&0xff000000)>>8))

static short tmp[10000];

int mbus_sequence_exit = 0;

const struct mb_addr_s
{
  char addr[16];
  int port;
}
mb_addr[]=
{
  {"192.168.0.222",5000},
  {"192.168.0.222",5000},
  {"192.168.0.222",5000},
  {"192.168.0.222",5000},
  {"192.168.0.222",5000},
  {"192.168.0.222",5000},
  {"192.168.0.222",5000},
  {"192.168.0.222",5000},
  {"192.168.0.222",5000},
  {"192.168.0.222",5000},
};

const struct mb_scb_s
{
  int group;
  int word;
  int fun;
  int adr;
  int len;
  int val;
}
mb_scb[]=
{
  {0,0,2,0,160,0},
  {0,160/16,2,160,64,0},
  {0,224/16,2,224,16,0},
  {1,0,3,0,50,0},
  {1,100,3,100,2,0},
//{1,1000,3,1000,100,0},
//{1,2000,3,2000,100,0},
//{1,3000,3,3000,100,0},
//{1,4000,3,4000,100,0},
//{1,5000,3,5000,100,0},
//{1,6000,3,6000,100,0},
//{2,0,4,0,100,0},
  {2,100,4,100,100,0},
  {2,200,4,200,100,0},
  {2,380,4,380,6,0},
//{2,300,4,300,100,0},
//{2,400,4,400,100,0},
  {2,500,4,500,100,0},
  {2,1000,4,1000,80,0},
//{1,50,3,50,6,0},
//{2,380,4,380,6,0},
  {0,0,0,0,0,0}
};

MBUScli::MBUScli(int nr)
{
  q = id = 0;
  this->nr = nr;
  addr.setAddress(mb_addr[nr-1].addr); port=mb_addr[nr-1].port;
  udp = new QUdpSocket(this); 
  udp->disconnectFromHost();
  if (!udp->bind(0)) { qDebug("SCB: %d udp socket bind error\n",nr); return; }
  if (!connect(udp,SIGNAL(readyRead()), this,SLOT(rx()))) { qDebug("SCB: %d udp socket connect rx error\n",nr); return; }
  timer = new QTimer(this); connect(timer, SIGNAL(timeout()), this, SLOT(tx())); timer->start(200);
  qDebug("SCB: %d %s:%d %p",nr,addr.toString().toAscii().data(),port,udp);
}

void MBUScli::rx()
{
  int ret=0;
  while (udp->hasPendingDatagrams())
  {
    char b2[1500];
    QHostAddress addr2; quint16 port2;
    int size = udp->pendingDatagramSize();
    ret = udp->readDatagram(b2,size,&addr2,&port2);
    int fun = b2[7]&0xff;
    if (addr != addr2) { qDebug("SCB: %d %s:%d = %s:%d ADDR ERROR",nr,addr.toString().toAscii().data(),port,addr2.toString().toAscii().data(),port2); continue; }
    if (port != port2) { qDebug("SCB: %d %s:%d = %s:%d PORT ERROR",nr,addr.toString().toAscii().data(),port,addr2.toString().toAscii().data(),port2); continue; }
    if (size != ret) { qDebug("SCB: %d %s:%d FUN %d=%d SIZE ERROR %d=%d",nr,addr.toString().toAscii().data(),port,fun,mb_scb[q].fun,size,ret); continue; }
    if ((b2[7]&0x80)==0x80) { qDebug("SCB: %d %s:%d FUN %d MBUS ERROR %02x %02x",nr,addr.toString().toAscii().data(),port,fun,b2[7]&0xff,b2[8]&0xff); continue; }
    if (fun != mb_scb[q].fun) { qDebug("SCB: %d %s:%d FUN %d=%d FUN MISMATCH ERROR",nr,addr.toString().toAscii().data(),port,fun,mb_scb[q].fun); continue; }
    qDebug("SCB: %d %s:%d FUN %d=%d RECV %d %04x=%d %d:%d=%d %u=%u TRY:%d OK",nr,addr2.toString().toAscii().data(),port2,fun,mb_scb[q].fun,q,0,0,0,size,ret,b2[5]&0xff,b2[8]&0xff,0);
    ret -= 9;
    switch (fun)
    {
      case 2:
      case 3:
      case 4:
      {
        short *out=(short*)&f[mb_scb[q].group][mb_scb[q].word],*in=(short*)&b2[9]; for (int i=0;i<ret;i+=2,in++,out++) *out=(fun==2)?*in:swap16(*in); //stekot fun 2 error
//      extern int MbusTRX; if (MbusTRX)
        {
          int t; char b[4096],*mem=(char*)&f[mb_scb[q].group][mb_scb[q].word];
          t=sprintf(b,"mbus:"); for (int i=0;i<size;i++) t+=sprintf(b+t," %02x",b2[i]&0xff); qDebug(b);
          t=sprintf(b,"memo:"); for (int i=0;i<ret;i++) t+=sprintf(b+t," %02x",mem[i]&0xff); qDebug(b);
        }
        break;
      }
      case 5:
      case 6:
      {
/*
        if (memcmp(b1,b2,12))
        {
          int t=0; char b[4096];
          t+=sprintf(b+t,"b1"); for (int i=0;i<12;i++) t+=sprintf(b+t," %02x",b1[i]&0xff); t+=sprintf(b+t,"\n");
          t+=sprintf(b+t,"b2"); for (int i=0;i<12;i++) t+=sprintf(b+t," %02x",b2[i]&0xff); t+=sprintf(b+t,"\n");
          t+=sprintf(b+t,"SCB: %d %s:%d FUN %d RESPONSE ERROR",nr,addr.toString().toAscii().data(),port,fun);
          qDebug(b);
          goto mb_repeat;
        }
        qDebug("SCB: %d %s:%d FUN %d RESPONSE OK",nr,addr.toString().toAscii().data(),port,fun);
*/
        break;
      }
      default:
      {
        ret=-1;
      }
    }
    ///up();
  }
  ///up();
  T=(ret<0)?"ERROR":QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
  //qDebug("SCB: %d %s:%d MBUS SEQUENCE %d %s",nr,addr.toString().toAscii().data(),port,ret,(ret<0)?"ERROR":"");
}

int MBUScli::mb(char * mem, char fun, short base, short len, short val)
{
  int tc = /*5*/1;
  ///mb_repeat:
  if (mbus_sequence_exit) return 0; if (--tc<0) return -1;
  if ((fun!=2 && len>125) || (fun==2 && len>2000)) { qDebug("SCB: %d %s:%d FUN %d LEN ERROR %d",nr,addr.toString().toAscii().data(),port,fun,len); return -1; }
  char b1[100],b2[1500];
  *(int*)&b1[0] = swap32(id); id++;
  *(short*)&b1[4] = swap16(6);
  b1[6] = 1;
  b1[7] = fun;
  *(short*)&b1[8] = swap16(base);
  switch (fun)
  {
    case 2:
    case 3:
    case 4: { *(short*)&b1[10] = swap16(len); break; }
    case 5:
    case 6: { *(short*)&b1[10] = val/*swap16(len)*/; break; }
    default: { qDebug("SCB: %d %s:%d FUN UNIMPLEMENTED %d",nr,addr.toString().toAscii().data(),port,fun); return -1; }
  }
  udp->writeDatagram((const char *)&b1,12,addr,port);
/*
  qDebug("SCB: %d %s:%d FUN %d SEND %d %d TRY:%d",nr,addr.toString().toAscii().data(),port,fun,base,len,tc);
  int i; for (i=0;i</ *50*20* /50;i++) { if (udp->hasPendingDatagrams()) break; msleep(20); / *sleep.wait(&mutex,20);* / qApp->processEvents(); }
  /// QTime timeout = QTime::currentTime().addMSecs(500); int i; for (i=0;QTime::currentTime()<timeout;i++) { / *if (udp->hasPendingDatagrams()) break;* / / *sleep.wait(&mutex,20);* / qApp->processEvents(); }
  qDebug("---------------------------> %d",i);
  if (!udp->hasPendingDatagrams()) { qDebug("SCB: %d %s:%d FUN %d RECV ERROR",nr,addr.toString().toAscii().data(),port,fun); goto mb_repeat; }
  int size = udp->pendingDatagramSize();
  int ret = udp->readDatagram(b2,size,&addr2,&port2);
  if ((b2[7]&0x80)==0x80) { qDebug("SCB: %d %s:%d FUN %d MBUS ERROR %02x %02x",nr,addr.toString().toAscii().data(),port,fun,b2[7]&0xff,b2[8]&0xff); goto mb_repeat; }
  if (size != ret) { qDebug("SCB: %d %s:%d FUN %d SIZE ERROR %d=%d",nr,addr.toString().toAscii().data(),port,fun,size,ret); goto mb_repeat; }
  if (fun != b2[7]&0xff) { qDebug("SCB: %d %s:%d FUN %d=%d FUN MISMATCH ERROR",nr,addr.toString().toAscii().data(),port,fun,b2[7]&0xff); goto mb_repeat; }
  qDebug("SCB: %d %s:%d FUN %d=%d RECV %04x=%d %d:%d=%d %u=%u TRY:%d",nr,addr2.toString().toAscii().data(),port2,fun,b2[7]&0xff,base,base,len,size,ret,b2[5]&0xff,b2[8]&0xff,tc);
  ret -= 9;
  switch (fun)
  {
    case 2:
    case 3:
    case 4:
    {
      if (!mem) { qDebug("SCB: %d %s:%d FUN %d OUTPUT ERROR mem=0",nr,addr.toString().toAscii().data(),port,fun); goto mb_repeat; }
      short *out=(short*)mem,*in=(short*)&b2[9]; for (int i=0;i<ret;i+=2,in++,out++) *out=(fun==2)?*in:swap16(*in); //stekot fun 2 error
      extern int MbusTRX; if (MbusTRX)
      {
        int t=0; char b[4096];
        t+=sprintf(b+t,"mbus:"); for (int i=0;i<size;i++) t+=sprintf(b+t," %02x",b2[i]&0xff);
        t+=sprintf(b+t,"\nmem:"); for (int i=0;i<ret;i++) t+=sprintf(b+t," %02x",mem[i]&0xff);
        qDebug(b);
      }
      break;
    }
    case 5:
    case 6:
    {
      if (memcmp(b1,b2,12))
      {
        int t=0; char b[4096];
        t+=sprintf(b+t,"b1"); for (int i=0;i<12;i++) t+=sprintf(b+t," %02x",b1[i]&0xff); t+=sprintf(b+t,"\n");
        t+=sprintf(b+t,"b2"); for (int i=0;i<12;i++) t+=sprintf(b+t," %02x",b2[i]&0xff); t+=sprintf(b+t,"\n");
        t+=sprintf(b+t,"SCB: %d %s:%d FUN %d RESPONSE ERROR",nr,addr.toString().toAscii().data(),port,fun);
        qDebug(b);
        goto mb_repeat;
      }
      qDebug("SCB: %d %s:%d FUN %d RESPONSE OK",nr,addr.toString().toAscii().data(),port,fun);
      break;
    }
  }
  return ret;
*/return 0;
}

#define WE(we) ((we<16)?(*(int*)&f[0][0])&(1<<we):(*(int*)&f[0][1])&(1<<(we&0xf)))
#define WE130(we) (f[0][128/16]&(1<<(we-128)))

void MBUScli::tx()
{
  ///qDebug("=====================================> [%d]",nr);
  int ret = 0;
/*
  if (mb((char*)&f[0][0],2,0,160,0)<0) ret++;
  if (mb((char*)&f[0][160/16],2,160,64,0)<0) ret++;
  if (mb((char*)&f[0][224/16],2,224,16,0)<0) ret++;
  if (mb((char*)&f[1][0],3,0,50,0)<0) ret++;
  if (mb((char*)&f[1][100],3,100,2,0)<0) ret++;
//if (mb((char*)&f[1][1000],3,1000,100,0)<0) ret++;
//if (mb((char*)&f[1][2000],3,2000,100,0)<0) ret++;
//if (mb((char*)&f[1][3000],3,3000,100,0)<0) ret++;
//if (mb((char*)&f[1][4000],3,4000,100,0)<0) ret++;
//if (mb((char*)&f[1][5000],3,5000,100,0)<0) ret++;
//if (mb((char*)&f[1][6000],3,6000,100,0)<0) ret++;
//if (mb((char*)&f[2][0],4,0,100,0)<0) ret++;
  if (mb((char*)&f[2][100],4,100,100,0)<0) ret++;
  if (mb((char*)&f[2][200],4,200,100,0)<0) ret++;
  if (mb((char*)&f[2][380],4,380,6,0)<0) ret++;
//if (mb((char*)&f[2][300],4,300,100,0)<0) ret++;
//if (mb((char*)&f[2][400],4,400,100,0)<0) ret++;
  if (mb((char*)&f[2][500],4,500,100,0)<0) ret++;
  if (mb((char*)&f[2][1000],4,1000,80,0)<0) ret++;
//if (mb((char*)&f[1][50],3,50,6,0)<0) ret++;
//if (mb((char*)&f[2][380],4,380,6,0)<0) ret++;
*/
  //for (int i=0;mb_scb[i].len;i++) if (mb((char*)&f[mb_scb[i].group][mb_scb[i].word],mb_scb[i].fun,mb_scb[i].adr,mb_scb[i].len,mb_scb[i].val)<0) ret++;
  if (!mb_scb[++q].len) q=0;
  if (mb((char*)&f[mb_scb[q].group][mb_scb[q].word],mb_scb[q].fun,mb_scb[q].adr,mb_scb[q].len,mb_scb[q].val)<0) ret++;
/*
  if (ret==0 && nr<4)
  {
    mux[nr-1].s[0]=(!WE(4))?3:(WE(20))?1:0;//(!(WE(4)))?3:(!(WE(20)))?1:0;//alarm kocio³
    mux[nr-1].s[1]=((!WE(18))||(!WE(21))||(WE130(130))||(WE130(131)))?3:0;//(!(WE(18))||!(WE(21))||(WE130(130))||(WE130(131)))?2:0;//max/min ciœnienie wody z kot³a Px12 130 131 18 21
    mux[nr-1].s[2]=((WE130(132))||(WE130(133)))?3:0;//max/min temperatura wody z kot³a Tx19 132 133
    mux[nr-1].s[3]=((!WE(12))||(WE130(134)))?3:0;//(!(WE(12))||(WE130(134))||(WE130(135)))?2:0;//min przep³yw wody przez kocio³ Fx25 134
    mux[nr-1].s[4]=0;//max ciœnienie spalin Px18
    mux[nr-1].s[5]=(!WE(2))?3:(WE(0))?1:0;//pompa Nx81
    mux[nr-1].s[6]=(!WE(5))?3:((WE(9))||(WE(10)))?1:0;//stan pracy palnika
    mux[nr-1].s[7]=(WE(14))?1:0;//kaskada 4
    mux[nr-1].s[8]=(WE(9))?1:(WE(10))?4/ *5* /:0;//rodzaj paliwa
    mux[nr-1].s[9]=(!WE(19))?3:0;//awaryjny poziom wody
    mux[nr-1].a[0]=(double)(f[2][200])/1000;//Px12
    mux[nr-1].a[1]=(double)(f[2][201])/10;//Tx14
    mux[nr-1].a[2]=(double)(f[2][202])/1000;//Px15
    mux[nr-1].a[3]=(double)(f[2][203])/10;//Tx16
    mux[nr-1].a[4]=(double)(f[2][204])/10;//Tx19
    mux[nr-1].a[5]=(double)(f[2][205])/100;//Fx95=Fx25
    mux[nr-1].a[6]=(double)(f[2][206])/10;//Tx26
    mux[nr-1].a[7]=(double)(f[2][207])/10;//Tx31
    mux[nr-1].a[8]=(double)(f[2][205])/100;//Fx25
    mux[nr-1].a[9]=(double)(f[2][100]);//Ux91
    mux[nr-1].a[10]=(double)(*(int*)&f[2][1000])/100;//Ux92
    mux[nr-1].a[11]=(double)(*(int*)&f[2][1002]);//Qx97
    mux[nr-1].a[12]=(double)(*(int*)&f[2][1004]);//Qx99
    mux[nr-1].a[13]=(double)(*(int*)&f[2][1006]);//Qx98
    mux[nr-1].a[14]=(double)(f[2][210]);//Ex85
    mux[nr-1].a[15]=(double)(f[2][211]);//Px30
    mux[nr-1].a[16]=(double)(f[2][500]);//DACP2
    mux[nr-1].a[17]=(double)(f[2][501]);//DACZ2
    mux[nr-1].a[18]=(double)(f[2][102])/10;//TPAL
    mux[nr-1].a[19]=(double)(f[2][208]);//Zx83
    mux[nr-1].a[20]=(double)(f[1][100]);//WZAWx
    qDebug("WE%d: %04x %04x   %d %d %d %d   %d %d %d %d   %d %d %d %d   %d %d %d %d   %d %d %d %d   %d %d %d %d",
    nr,f[0][1]&0xffff,f[0][0]&0xffff,
    WE(0)?1:0,WE(1)?1:0,WE(2)?1:0,WE(3)?1:0,WE(4)?1:0,WE(5)?1:0,WE(6)?1:0,WE(7)?1:0,
    WE(8)?1:0,WE(9)?1:0,WE(10)?1:0,WE(11)?1:0,WE(12)?1:0,WE(13)?1:0,WE(14)?1:0,WE(15)?1:0,
    WE(16)?1:0,WE(17)?1:0,WE(18)?1:0,WE(19)?1:0,WE(20)?1:0,WE(21)?1:0,WE(22)?1:0,WE(23)?1:0);
    qDebug("WE%d: %04x   %d %d %d %d   %d %d %d %d",
    nr,f[0][8]&0xffff,
    WE130(128)?1:0,WE130(129)?1:0,WE130(130)?1:0,WE130(131)?1:0,WE130(132)?1:0,WE130(133)?1:0,WE130(134)?1:0,WE130(135)?1:0);
    qDebug("WE%d: %d %d %d %d %d %d %d %d %d",
    nr,
    mux[nr-1].s[0],mux[nr-1].s[1],mux[nr-1].s[2],mux[nr-1].s[3],mux[nr-1].s[4],mux[nr-1].s[5],mux[nr-1].s[6],mux[nr-1].s[7],mux[nr-1].s[8]);
  }
  T=(ret)?"ERROR":QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
  qDebug("SCB: %d %s:%d MBUS SEQUENCE %d %s",nr,addr.toString().toAscii().data(),port,ret,(ret)?"ERROR":"");
  analiza_zdarzen(nr);
*/
}

void MBUScli::up()
{
  if (nr<0 || nr>3) { qDebug("SCB: %d %s:%d MBUS ERROR MUX nr",nr,addr.toString().toAscii().data(),port); return; }

  struct mux
  {
    char   s[20]; // state
    double a[32]; // analog
  }
  mux[4];

  mux[nr-1].s[0]=(!WE(4))?3:(WE(20))?1:0;//(!(WE(4)))?3:(!(WE(20)))?1:0;//alarm kocio³
  mux[nr-1].s[1]=((!WE(18))||(!WE(21))||(WE130(130))||(WE130(131)))?3:0;//(!(WE(18))||!(WE(21))||(WE130(130))||(WE130(131)))?2:0;//max/min ciœnienie wody z kot³a Px12 130 131 18 21
  mux[nr-1].s[2]=((WE130(132))||(WE130(133)))?3:0;//max/min temperatura wody z kot³a Tx19 132 133
  mux[nr-1].s[3]=((!WE(12))||(WE130(134)))?3:0;//(!(WE(12))||(WE130(134))||(WE130(135)))?2:0;//min przep³yw wody przez kocio³ Fx25 134
  mux[nr-1].s[4]=0;//max ciœnienie spalin Px18
  mux[nr-1].s[5]=(!WE(2))?3:(WE(0))?1:0;//pompa Nx81
  mux[nr-1].s[6]=(!WE(5))?3:((WE(9))||(WE(10)))?1:0;//stan pracy palnika
  mux[nr-1].s[7]=(WE(14))?1:0;//kaskada 4
  mux[nr-1].s[8]=(WE(9))?1:(WE(10))?4/*5*/:0;//rodzaj paliwa
  mux[nr-1].s[9]=(!WE(19))?3:0;//awaryjny poziom wody
  mux[nr-1].a[0]=(double)(f[2][200])/1000;//Px12
  mux[nr-1].a[1]=(double)(f[2][201])/10;//Tx14
  mux[nr-1].a[2]=(double)(f[2][202])/1000;//Px15
  mux[nr-1].a[3]=(double)(f[2][203])/10;//Tx16
  mux[nr-1].a[4]=(double)(f[2][204])/10;//Tx19
  mux[nr-1].a[5]=(double)(f[2][205])/100;//Fx95=Fx25
  mux[nr-1].a[6]=(double)(f[2][206])/10;//Tx26
  mux[nr-1].a[7]=(double)(f[2][207])/10;//Tx31
  mux[nr-1].a[8]=(double)(f[2][205])/100;//Fx25
  mux[nr-1].a[9]=(double)(f[2][100]);//Ux91
  mux[nr-1].a[10]=(double)(*(int*)&f[2][1000])/100;//Ux92
  mux[nr-1].a[11]=(double)(*(int*)&f[2][1002]);//Qx97
  mux[nr-1].a[12]=(double)(*(int*)&f[2][1004]);//Qx99
  mux[nr-1].a[13]=(double)(*(int*)&f[2][1006]);//Qx98
  mux[nr-1].a[14]=(double)(f[2][210]);//Ex85
  mux[nr-1].a[15]=(double)(f[2][211]);//Px30
  mux[nr-1].a[16]=(double)(f[2][500]);//DACP2
  mux[nr-1].a[17]=(double)(f[2][501]);//DACZ2
  mux[nr-1].a[18]=(double)(f[2][102])/10;//TPAL
  mux[nr-1].a[19]=(double)(f[2][208]);//Zx83
  mux[nr-1].a[20]=(double)(f[1][100]);//WZAWx
  qDebug("WE%d: %04x %04x   %d %d %d %d   %d %d %d %d   %d %d %d %d   %d %d %d %d   %d %d %d %d   %d %d %d %d",
  nr,f[0][1]&0xffff,f[0][0]&0xffff,
  WE(0)?1:0,WE(1)?1:0,WE(2)?1:0,WE(3)?1:0,WE(4)?1:0,WE(5)?1:0,WE(6)?1:0,WE(7)?1:0,
  WE(8)?1:0,WE(9)?1:0,WE(10)?1:0,WE(11)?1:0,WE(12)?1:0,WE(13)?1:0,WE(14)?1:0,WE(15)?1:0,
  WE(16)?1:0,WE(17)?1:0,WE(18)?1:0,WE(19)?1:0,WE(20)?1:0,WE(21)?1:0,WE(22)?1:0,WE(23)?1:0);
  qDebug("WE%d: %04x   %d %d %d %d   %d %d %d %d",
  nr,f[0][8]&0xffff,
  WE130(128)?1:0,WE130(129)?1:0,WE130(130)?1:0,WE130(131)?1:0,WE130(132)?1:0,WE130(133)?1:0,WE130(134)?1:0,WE130(135)?1:0);
  qDebug("WE%d: %d %d %d %d %d %d %d %d %d",
  nr,
  mux[nr-1].s[0],mux[nr-1].s[1],mux[nr-1].s[2],mux[nr-1].s[3],mux[nr-1].s[4],mux[nr-1].s[5],mux[nr-1].s[6],mux[nr-1].s[7],mux[nr-1].s[8]);
  ///analiza_zdarzen(nr);
}

MBUSsrv::MBUSsrv(int port)
{
  udp = new QUdpSocket(this); //connect(tcp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(err(QAbstractSocket::SocketError)));
  udp->disconnectFromHost();
  if (!udp->bind(port)) { qDebug("srv: udp socket bind error\n"); return; }
  connect(udp, SIGNAL(readyRead()), this, SLOT(rx()));
  //timer = new QTimer(this); connect(timer, SIGNAL(timeout()), this, SLOT(tx())); timer->start(cfg.t[0]);
for(int i=0;i<(int)sizeof(tmp)/2;i++)tmp[i]=i&0xffff;
  qDebug("srv: udp server start");
}

void MBUSsrv::rx()
{
  while (udp->hasPendingDatagrams())
  {
    QHostAddress host; quint16 port;
    char data[1500]; int size = udp->pendingDatagramSize();
    udp->readDatagram(data,size,&host,&port);
    qDebug("srv: %s:%d %d",host.toString().toAscii().data(),port,size); for (int i=0;i<size;i++) printf(" %02x",data[i]&0xff); printf("\n");
    if (size!=6+swap16(*(short*)&data[4])) { qDebug("srv: error mbus size(%d) @4:0x%04x",size,swap16(*(short*)&data[4])); continue; }
    switch(data[7])
    {
/*
      case 1:
      break;
      case 2:
      break;
      case 3:
      break;
*/
      case 4:
      {
        short adr = swap16(*(short*)&data[8]);
        short len = swap16(*(short*)&data[10]);
        char buf[259];
        *(int*)&buf[0] = *(int*)&data[0];
        *(short*)&buf[4] = swap16((len<<1)+3);
        buf[6] = 1;
        buf[7] = 4;
        buf[8] = len<<1;
        short * dst = (short*)&buf[9];
        short * src = &tmp[adr];
        for (int i=0;i<len;i++,src++,dst++) *dst = swap16(*src);
        udp->writeDatagram((const char *)&buf,(len<<1)+9,host,port);
        qDebug("srv: fun4 0x%04x(%d) %x(%d)",adr,adr,len,len);
        break;
      }
/*
      case 5:
      break;
      case 15:
      break;
      case 16:
      break;
*/
      default:
      {
        char buf[9];
        *(int*)&buf[0] = *(int*)&data[0];
        *(short*)&buf[4] = swap16(3);
        buf[6] = 1;
        buf[7] = 0x84;
        buf[8] = 2;
        udp->writeDatagram((const char *)&buf,9,host,port);
        qDebug("srv: error fun %d",data[7]);
      }
    }
  }
}

MBUSview::MBUSview(QWidget *parent) : QWidget(parent)
{
  H = new QHBoxLayout;
  for (int h=0;h<4;h++)
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

void MBUSview::up()
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
