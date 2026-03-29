#include "mainwindow.h"
#include <QApplication>
#include <QWindow>               // Pour windowHandle()
#include <qt_windows.h>          // Pour HWND et les APIs Windows
#include <dwmapi.h>              // Pour DwmSetWindowAttribute
#include <QDebug>                // Pour les messages de debug (optionnel mais utile)



// Définitions si elles ne sont pas déjà dans ton SDK Windows
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

enum DWM_SYSTEMBACKDROP_TYPE
{
    DWMSBT_AUTO            = 0,
    DWMSBT_NONE            = 1,
    DWMSBT_MAINWINDOW      = 2,   // ← Mica standard (celui qu'on veut le plus souvent)
    DWMSBT_TRANSIENTWINDOW = 3,
    DWMSBT_TABBEDWINDOW    = 4
};


// Fonction helper pour activer Mica
void enableMica(QWindow* window)
{
    if (!window) return;

    HWND hwnd = (HWND)window->winId();

    // Étendre le Mica sur toute la zone client (important !)
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // Appliquer le type Mica
    DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_MAINWINDOW;

    HRESULT hr = DwmSetWindowAttribute(
        hwnd,
        DWMWA_SYSTEMBACKDROP_TYPE,
        &backdropType,
        sizeof(backdropType)
        );

    if (FAILED(hr))
    {
        qDebug() << "Échec de l'activation Mica - HRESULT :" << hr;
    }
    else
    {
        qDebug() << "Mica activé avec succès";
    }
}



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Q_INIT_RESOURCE(stylelight);   // nom de ton fichier stylelight.qrc sans .qrc
    Q_INIT_RESOURCE(styledark);    // nom de ton fichier styledark.qrc sans .qrc



    MainWindow w;
    w.show();

    // Appliquer Mica APRÈS le show() → le HWND doit exister
    enableMica(w.windowHandle());

    if(argc>1)
    {
        QString pdfPath=QString::fromLocal8Bit(argv[1]);//le chemin de l'argument
        QFileInfo info(pdfPath);
        if(info.exists()&& info.suffix().toLower()=="pdf")
        {
            // ️ CORRECTION : Utiliser singleShot pour retarder le chargement
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
