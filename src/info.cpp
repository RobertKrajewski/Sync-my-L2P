#include "info.h"
#include "ui_info.h"

Info::Info(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Info)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
    ui->retranslateUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    QImage image(":/icons/Sync-my-L2P.png");
    ui->picture->setPixmap(QPixmap::fromImage(image));
    ui->picture->adjustSize();
}
Info::~Info()
{
    delete ui;
}
