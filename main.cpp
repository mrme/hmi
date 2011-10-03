#include <QApplication>

#include "main.h"
#include "xml.h"
#include "mb.h"

struct baza baza;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QString xml("xml.xml");
    for (int i=1;i<argc;i++)
    if (!QString(argv[i]).compare("-xml")) xml=QString(argv[++i]);
    else if (!QString(argv[i]).compare("-srv")) { new MBS(QString(argv[++i]).toInt()); return app.exec(); }

    XML(xml);

    return app.exec();
}
