#ifndef DATERANGEDIALOG_H
#define DATERANGEDIALOG_H

#include <QDialog>

namespace Ui {
class DateRangeDialog;
}

class DateRangeDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit DateRangeDialog(QWidget *parent = 0);
    ~DateRangeDialog();
    
private:
    Ui::DateRangeDialog *ui;
};

#endif // DATERANGEDIALOG_H
