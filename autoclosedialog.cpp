#include "autoclosedialog.h"
#include "ui_autoclosedialog.h"

AutoCloseDialog::AutoCloseDialog(QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::AutoCloseDialog)
{
    ui->setupUi(this);
    ui->retranslateUi(this);
    QTimer::singleShot(5000, this, SLOT(reject()));
}

AutoCloseDialog::~AutoCloseDialog()
{
    delete ui;
}

void AutoCloseDialog::on_autoClosePushButton_clicked()
{
    accept();
}
