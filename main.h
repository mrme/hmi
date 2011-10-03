#ifndef MAIN_H
#define MAIN_H

#define dbg(s,x...) qDebug("%s %s:%d: "s,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toAscii().data(),__FILE__,__LINE__,x)
#define err(s,x...) qDebug("%s %s:%d: ERROR "s,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toAscii().data(),__FILE__,__LINE__,x)

#define BAZA_A_MAX 1000
#define BAZA_B_MAX 1000
struct baza { int a[BAZA_A_MAX]; char b[BAZA_B_MAX]; }; extern struct baza baza;

#endif
