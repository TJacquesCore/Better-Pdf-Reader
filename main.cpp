#include "mainwindow.h"
#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    if(argc>1)
    {
        QString pdfPath=QString::fromLocal8Bit(argv[1]);//le chemin de l'argument
        QFileInfo info(pdfPath);
        if(info.exists()&& info.suffix().toLower()=="pdf")
        {
            // ⚠️ CORRECTION : Utiliser singleShot pour retarder le chargement
            QTimer::singleShot(100, [&w, pdfPath]() {
                w.reload(pdfPath);//charger le pdf
            });
        }
        else
        {
            qDebug()<< "le ficher n'existe pas ou n'est pas un pdf";
        }
    }

    a.setWindowIcon(QIcon(":/icon/pdf-file.png"));

    return a.exec();
}
