#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include<QtPdf/QPdfDocument>
#include<QtPdfWidgets/QPdfView>
#include<QtPdf/QPdfPageNavigator>
#include<QShortcut>
#include<QFileDialog>
#include<QPdfBookmarkModel>
#include <QPdfSearchModel>
#include<QSortFilterProxyModel>
#include<QEvent>
#include<QTimer>
#include<QStandardPaths>
#include<QDir>
#include<QCoreApplication>
#include<QFile>




QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();





public slots:
    //Pour Zoomer
    void zoomIn();
    void zoomOut();

    //methode pour changer de page
    void forward();
    void backward();
    //ouvrir un document depuis l'interface
    void ouvrir();


    //refresh
    void reload(QString locationfile);

    //mettre a jour le label
    void updateLabel();

    //cacher barre de gauche
    void ToogleViewBar();

    //pour garder la meme page quand il y a un evenement

    void keepSamePage();

    //Savegarder la page
    void saveCurrentPage();

    //charger les donnees
    void loadData();

    //toogle extend
    void SetExtend();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;


private:
    Ui::MainWindow *ui;

    //Variable pour dossier de configuration

    QString ConfigPath;

    //la vue pdf
    QPdfView* view;

    //Le document actuelle
    QString CurrentDocument;




    //le document a charger
    QPdfDocument* document;

    //pour la recherche
    QPdfSearchModel *searchModel;
    QSortFilterProxyModel *proxyModel;

    //pour le sommaire
    QPdfBookmarkModel* bookmarkModel;

    //shortcut pour touch up et down pour changer de page
    QShortcut* up;
    QShortcut* down;
    QShortcut* crtl_p;
    QShortcut* crtl_m;

    //Zoom factor
    qreal zoomFactor;






};
#endif // MAINWINDOW_H
