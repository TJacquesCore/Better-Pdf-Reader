#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Definir le nom de l'application
    QCoreApplication::setApplicationName("Better Pdf Viewer");
    setWindowTitle("Better PDF Viewer");

    //location du dossier de configuration
    ConfigPath=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(ConfigPath);

    QString exePath = QCoreApplication::applicationDirPath() + "/darkmod.exe";
    QString dest = ConfigPath + "/darkmod.exe";

    if (!QFile::exists(dest)) {
        qDebug() << "Chemin source :" << exePath;
        if (!QFile::exists(exePath)) {
            qDebug() << "Erreur : le fichier darkmod.exe n'est pas trouvé !";
        } else {
            if (QFile::copy(exePath, dest))
                qDebug() << "Copie réussie !";
            else
                qDebug() << "Erreur lors de la copie ! Vérifie les permissions.";
        }
    }

    qDebug() << "Dossier de configuration prêt :" << ConfigPath;

    document = new QPdfDocument(this);

    QGridLayout* layout = new QGridLayout(ui->mainFrame);

    bookmarkModel= new QPdfBookmarkModel(this);
    proxyModel = new QSortFilterProxyModel(this);

    // Initialisation
    searchModel = new QPdfSearchModel(this);
    view = new QPdfView(this);
    view->setDocument(document);

    layout->addWidget(view);
    ui->ExtendButton->setIcon(QIcon(":/icon/extend.png"));

    //definir le scroll continu et l'auto strech
    view->setPageMode(QPdfView::PageMode::MultiPage);
    view->setZoomMode(QPdfView::ZoomMode::FitToWidth);

    zoomFactor=1.0;
    view->setZoomFactor(zoomFactor);

    //toujour pour configurer up et down pour defiler entre les pages
    up= new QShortcut(QKeySequence(Qt::Key_Up),this);
    down=new QShortcut(QKeySequence(Qt::Key_Down),this);
    crtl_p= new QShortcut(QKeySequence("Ctrl+="),this);
    crtl_m= new QShortcut(QKeySequence("Ctrl+-"),this);

    //pour le slider factor
    ui->ZoomSlide->setMaximum(400);
    ui->ZoomSlide->setMinimum(25);

    //connect pour darkmode
    connect(ui->DarkModeRatio,&QRadioButton::toggled,this,&MainWindow::ToogleDarkMod);

    //connect pour extendButton
    connect(ui->ExtendButton, &QPushButton::clicked,this,&MainWindow::SetExtend);

    //connect pour save la page a chaque changement de page
    connect(view->pageNavigator(), &QPdfPageNavigator::currentPageChanged,this,&MainWindow::saveCurrentPage);

    //connect pour le slideFactor
    connect(ui->ZoomSlide, &QSlider::valueChanged, this, [this](int value) {
        zoomFactor = value / 100.0;
        view->setZoomMode(QPdfView::ZoomMode::Custom);
        view->setZoomFactor(zoomFactor);
    });

    //connect pour la recherche
    connect(ui->treeView, &QTreeView::activated, this, [this](const QModelIndex &index) {
        QModelIndex sourceIndex = proxyModel->mapToSource(index);
        int page = bookmarkModel->data(sourceIndex, static_cast<int>(QPdfBookmarkModel::Role::Page)).toInt();
        if (page >= 0)
            view->pageNavigator()->jump(page, QPointF(), 0);
    });

    // Recherche/filtre
    connect(ui->searchEdit, &QLineEdit::textChanged, proxyModel, &QSortFilterProxyModel::setFilterFixedString);

    //connect pour les shortcut
    connect(up,&QShortcut::activated,this,&MainWindow::backward);
    connect(down,&QShortcut::activated,this,&MainWindow::forward);
    connect(crtl_m,&QShortcut::activated,this,&MainWindow::zoomOut);
    connect(crtl_p,&QShortcut::activated,this,&MainWindow::zoomIn);

    //connect pour le bouton ouvrir
    connect(ui->openDocButton,&QPushButton::clicked,this,&MainWindow::ouvrir);

    //connect pour update le label
    connect(view->pageNavigator(),&QPdfPageNavigator::currentPageChanged, this, &MainWindow::updateLabel);

    //connect pour cacher le pannel de gauche
    connect(ui->HideButton,&QPushButton::clicked,this,&MainWindow::ToogleViewBar);

    //connect pour les ZoomFactor
    connect(ui->ZoomInButton,&QPushButton::clicked,this,&MainWindow::zoomIn);
    connect(ui->ZoomOutButton,&QPushButton::clicked,this,&MainWindow::zoomOut);

    //filtre pour les event pour prevenir le changement de page au resize
    view->installEventFilter(this);

    // Initialiser originalDocument
    originalDocument = "";
}

//methode pour mode sombre
void MainWindow::ToogleDarkMod()
{
    bool ischecked = ui->DarkModeRatio->isChecked();

    if (CurrentDocument.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Aucun document n'est chargé.");
        ui->DarkModeRatio->setChecked(false);
        return;
    }

    QFileInfo info(CurrentDocument);
    QString darkPath = ConfigPath + "/dark/" + info.fileName();

    if (ischecked)
    {
        // Mode sombre
        // Sauvegarder le document original si c'est la première activation
        if (originalDocument.isEmpty()) {
            originalDocument = CurrentDocument;
        }

        // Si la version sombre existe déjà → on la charge
        if (QFile::exists(darkPath))
        {
            qDebug() << "Version sombre déjà existante, chargement de :" << darkPath;
            reload(darkPath);
            return;
        }

        // Désactiver le bouton pendant la génération
        ui->DarkModeRatio->setEnabled(false);

        // BOÎTE DE DIALOGUE SIMPLIFIÉE
        QMessageBox *loadingBox = new QMessageBox(this);
        loadingBox->setWindowTitle("Génération en cours");
        loadingBox->setText("Génération du PDF en mode sombre...\n\nCette étape peut prendre un certain temps la première fois, dépendamment du nombre de pages.");
        loadingBox->setStandardButtons(QMessageBox::NoButton);
        loadingBox->setModal(true);
        loadingBox->show();

        // --- PROCESS ASYNCHRONE ---
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(ConfigPath);

        QString exePath = ConfigPath + "/darkmod.exe";
        QStringList args{ info.absoluteFilePath() };

        qDebug() << "→ Lancement du process darkmod...";
        qDebug() << "Exécutable :" << exePath;
        qDebug() << "Argument :" << info.absoluteFilePath();
        qDebug() << "Répertoire de travail :" << process->workingDirectory();
        qDebug() << "PDF dark attendu à :" << darkPath;

        // Capture des sorties pour debug
        connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
            qDebug() << "Darkmod stdout:" << process->readAllStandardOutput();
        });

        connect(process, &QProcess::readyReadStandardError, this, [=]() {
            qDebug() << "Darkmod stderr:" << process->readAllStandardError();
        });

        // Quand le process est terminé :
        connect(process, &QProcess::finished, this,
                [=](int exitCode, QProcess::ExitStatus exitStatus)
                {
                    // Fermer la boîte de dialogue
                    loadingBox->accept();
                    loadingBox->deleteLater();

                    // Réactiver le bouton
                    ui->DarkModeRatio->setEnabled(true);

                    qDebug() << "Process terminé avec code:" << exitCode;

                    if (exitCode == 0 && QFile::exists(darkPath))
                    {
                        qDebug() << "✓ Génération dark terminée, chargement...";
                        reload(darkPath);
                    }
                    else
                    {
                        qDebug() << "✗ Erreur lors de la génération dark";
                        qDebug() << "PDF dark existe :" << QFile::exists(darkPath);

                        QMessageBox::warning(this, "Erreur",
                                             "Impossible de générer la version sombre.\n"
                                             "Code de sortie: " + QString::number(exitCode));
                        ui->DarkModeRatio->setChecked(false);
                    }
                    process->deleteLater();
                });

        process->start(exePath, args);
    }
    else
    {
        // Mode clair - Recharger le document ORIGINAL
        if (!originalDocument.isEmpty() && QFile::exists(originalDocument)) {
            qDebug() << "Retour au mode clair, chargement de :" << originalDocument;
            reload(originalDocument);
        } else {
            qDebug() << "Document original non trouvé, chargement du document courant :" << CurrentDocument;
            reload(CurrentDocument);
        }
    }
}

//methodes pour zoomer
void MainWindow::zoomIn()
{
    if (zoomFactor > 4.0) zoomFactor = 4.0; // max
    zoomFactor*=1.1;
    view->setZoomMode(QPdfView::ZoomMode::Custom);
    ui->ZoomSlide->setValue(zoomFactor * 100);
    view->setZoomFactor(zoomFactor);
}

void MainWindow::zoomOut()
{
    if (zoomFactor < 0.25) zoomFactor = 0.25;
    zoomFactor/=1.1;
    view->setZoomMode(QPdfView::ZoomMode::Custom);
    ui->ZoomSlide->setValue(zoomFactor * 100);
    view->setZoomFactor(zoomFactor);
}

//toogle extend
void MainWindow::SetExtend()
{
    view->setZoomMode(QPdfView::ZoomMode::FitToWidth);
}

//evenement pour eviter le changement de page
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == view && event->type() == QEvent::Resize) {
        // Sauvegarde la page actuelle
        int currentPage = view->pageNavigator()->currentPage();

        // Restaure après redimensionnement
        QTimer::singleShot(0, this, [this, currentPage]() {
            view->pageNavigator()->jump(currentPage, QPointF(), 0);
        });
    }

    return QMainWindow::eventFilter(obj, event);
}

//definir fonction pour garder la page aux changement
void MainWindow::keepSamePage()
{
    view->pageNavigator()->currentPage();
}

//definir fonction pour hide/show la bar
void MainWindow::ToogleViewBar()
{
    bool isVisible= ui->sidebarFrame->isVisible();

    if(isVisible)
    {
        ui->sidebarFrame->setVisible(false);
        ui->HideButton->setText("Afficher");
    }
    else
    {
        ui->sidebarFrame->setVisible(true);
        ui->HideButton->setText("Masquer");
    }
}

//definition de ouvrir
void MainWindow::ouvrir()
{
    QString newDocument = QFileDialog::getOpenFileName(this,"Selectionner un fichier pdf",QDir::homePath(),"(*.pdf)");

    if (!newDocument.isEmpty() && newDocument != CurrentDocument) {
        qDebug() << "Chargement du nouveau document :" << newDocument;

        // IMPORTANT: Toujours définir le document original
        originalDocument = newDocument;
        CurrentDocument = newDocument;

        reload(newDocument);
    } else if (newDocument == CurrentDocument) {
        qDebug() << "Rechargement du même document :" << CurrentDocument;
        reload(CurrentDocument);
    }

    // Forcer le mode clair à l'ouverture
    ui->DarkModeRatio->setChecked(false);
}

//Definition fonction load
void MainWindow::loadData()
{
    if (CurrentDocument.isEmpty()) {
        qDebug() << "Aucun document courant";
        return;
    }

    QFileInfo info(CurrentDocument);
    QString filePassager = ConfigPath + "/" + info.fileName();

    QFile file(filePassager);
    if (!file.exists()) {
        qDebug() << "Fichier de configuration introuvable :" << filePassager;
        // Charger la première page par défaut
        view->pageNavigator()->jump(0, QPointF(), 0);
        return;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        int lastPage = 0;

        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.startsWith("at page : "))
                lastPage = line.section("at page : ", 1, 1).toInt();
        }
        file.close();

        // S'assurer que la page est valide
        if (lastPage >= document->pageCount()) {
            lastPage = 0;
        }

        // Puis sauter à la page
        view->pageNavigator()->jump(lastPage, QPointF(), 0);
    }
    else
    {
        qDebug() << "Impossible d'ouvrir le fichier :" << filePassager;
    }
}

//Definirtion Fonction Save Page
void MainWindow::saveCurrentPage()
{
    QTimer::singleShot(2000,this,[this]()
                       {
                           QFileInfo info(CurrentDocument);
                           qDebug()<<"file name is :"<<info.fileName();
                           QString filePassager= ConfigPath+"/"+info.fileName();
                           qDebug()<< "file passager : "<<filePassager;
                           QFile file (filePassager);

                           if(file.open(QIODevice::WriteOnly | QIODevice::Text))
                           {
                               QTextStream out(&file);
                               out << "at page : "<<view->pageNavigator()->currentPage()<<Qt::endl;
                               file.close();
                           }
                           else
                           {
                               qDebug()<<"impossible d'ouvrir le ficher";
                           }
                       });
}

//definition de reload
void MainWindow::reload(QString locationfile)
{
    if(!locationfile.isEmpty())
    {
        // Ne pas modifier originalDocument lors d'un reload
        // Sauf si c'est un nouveau document
        QFileInfo newInfo(locationfile);
        QFileInfo currentInfo(CurrentDocument);

        // Si c'est un document différent, mettre à jour originalDocument
        if (newInfo.fileName() != currentInfo.fileName()) {
            originalDocument = locationfile;
        }

        CurrentDocument = locationfile;

        document->close();
        document->load(locationfile);

        // Récupérer la table des matières
        bookmarkModel->setDocument(document);
        ui->treeView->setModel(bookmarkModel);
        proxyModel->setSourceModel(bookmarkModel);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setRecursiveFilteringEnabled(true);
        ui->treeView->setModel(proxyModel);

        searchModel->setDocument(document);

        // Charger les données SAUVÉES avant de mettre à jour l'interface
        loadData();

        QFileInfo info(CurrentDocument);
        //mettre le nom en haut
        qDebug()<<"le nom de l'app ";
        this->setWindowTitle("Better Pdf Viewer - " + info.fileName());

        // Mettre à jour le label après le chargement
        updateLabel();
    }
    else
    {
        qDebug()<<"erreur fonction reload variable location est vide";
    }
}

void MainWindow::forward()
{
    int nextpage=view->pageNavigator()->currentPage()+1;
    if(nextpage<document->pageCount())
        view->pageNavigator()->jump(nextpage,QPointF(),0);

    updateLabel();
}

void MainWindow::backward()
{
    int previous=view->pageNavigator()->currentPage()-1;
    if(previous >= 0)
        view->pageNavigator()->jump(previous,QPointF(),0);

    updateLabel();
}

//definition mettre a jour label
void MainWindow::updateLabel()
{
    int current= view->pageNavigator()->currentPage()+1;
    int total= document->pageCount();
    ui->pageInfoLabel->setText(QString("Page %1/%2").arg(current).arg(total));
}

MainWindow::~MainWindow()
{
    delete ui;
}
