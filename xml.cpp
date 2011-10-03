#include "main.h"
#include "xml.h"
#include "mb.h"

/*
void p(QDomNode node,int level)
{
  for (int i=0;i<level;i++) printf("-");
  printf("%s: %s '%s' (%d)",__FUNCTION__,node.nodeName().toAscii().data(),node.toElement().attribute("name").toAscii().data(),node.childNodes().count());
  for (int i=0;i<node.toElement().attributes().count();i++) printf(" | %s='%s'",node.toElement().attributes().item(i).nodeName().toAscii().data(),node.toElement().attributes().item(i).nodeValue().toAscii().data());
  printf("\n");
  for (int i=0;i<node.childNodes().count();i++) p(node.childNodes().item(i),level+1);
}

void o(QDomNode node)
{
  for(int i=0;i<node.childNodes().count();i++)
  if (node.childNodes().item(i).nodeName()=="object")
  printf("%s\n",node.childNodes().item(i).toElement().attribute("name").toAscii().data());
}

void s(QDomNode node)
{
  for(int i=0;i<node.childNodes().count();i++)
  if(node.childNodes().item(i).nodeName()=="objects")
  //for(int j=0;j<node.childNodes().item(i).childNodes().count();j++)
  //p(node.childNodes().item(i).childNodes().item(j),5);
  o(node.childNodes().item(i));
}

void modbuses(QDomNode node)
{
  for (int i=0;i<node.childNodes().count();i++)
  {
    if (node.childNodes().item(i).nodeName() != "modbus") { printf("error %s (%d)",node.childNodes().item(i).nodeName().toAscii().data(),node.childNodes().item(i).childNodes().count()); continue; }
    //new modbus(node.childNodes().item(i));
    printf("modbus %s %s\n",node.childNodes().item(i).toElement().attribute("ip").toAscii().data(),node.childNodes().item(i).toElement().attribute("port").toAscii().data());
  }
}
*/

void XML(QString xml)
{
  dbg("%s",xml.toAscii().data());
  QFile file(xml); if (!file.open(QIODevice::ReadOnly)) { err("xml open %s",xml.toAscii().data()); return; }
  QDomDocument doc; if (!doc.setContent(&file)) { err("xml set %s",xml.toAscii().data()); return; }
  file.close();
  QDomNode node = doc.firstChild();
  if (node.nodeName()!="xml") { err("xml root name <%s>\n",node.nodeName().toAscii().data()); return; }
  for (int i=0;i<node.childNodes().count();i++)
  {
    QDomNode node1 = node.childNodes().item(i);
    if (node1.nodeName() == "net")
    {
      for (int j=0;j<node1.childNodes().count();j++)
      {
        QDomNode node2 = node1.childNodes().item(j);
        if (node2.nodeName() == "modbus")
        {
          //dbg("%s",node2.nodeName().toAscii().data());
          new MBC(node2);
        }
        else
        {
          err("xml net unknown %s(%d)",
            node2.nodeName().toAscii().data(),
            node2.childNodes().count());
          continue;
        }
      }
    }
    else
    {
      err("xml unknown %s '%s' (%d)",
        node1.nodeName().toAscii().data(),
        node1.toElement().attribute("name").toAscii().data(),
        node1.childNodes().count());
      return;
    }
  }
}
