#include "daterangedialog.h"
#include "ui_daterangedialog.h"

DateRangeDialog::DateRangeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DateRangeDialog)
{
    ui->setupUi(this);
}

DateRangeDialog::~DateRangeDialog()
{
    delete ui;
}
