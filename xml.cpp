#include <QApplication>
#include <QtGui>
#include <QDomDocument>

void p(QDomNode node,int level)
{
  for(int i=0;i<level;i++)printf("   ");
    printf("%s: %s '%s' (%d)",__FUNCTION__,
      node.nodeName().toAscii().data(),
      node.toElement().attribute("name").toAscii().data(),
      node.childNodes().count());
  for(int i=0;i<node.toElement().attributes().count();i++)
    printf(" | %s='%s'",
      node.toElement().attributes().item(i).nodeName().toAscii().data(),
      node.toElement().attributes().item(i).nodeValue().toAscii().data());
  printf("\n");
  for(int i=0;i<node.childNodes().count();i++)
    p(node.childNodes().item(i),level+1);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    printf("%s\n",__FUNCTION__);
    //MainWindow window;
    QFile file("xml.xml"); if (!file.open(QIODevice::ReadOnly)) { printf("error open xml\n"); return -1; }
    QDomDocument document; if (!document.setContent(&file)) { printf("error set xml\n"); file.close(); return -1; }
    file.close();
    //DomModel newModel(document, this);
    //view->setModel(newModel);
    //delete model;
    //model = newModel;
    p(document.firstChild(),0);
    return app.exec();
}
