#ifndef AUTOCLOSEDIALOG_H
#define AUTOCLOSEDIALOG_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class AutoCloseDialog;
}

class AutoCloseDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AutoCloseDialog(QWidget *parent = nullptr);
    ~AutoCloseDialog();
    
private:
    Ui::AutoCloseDialog *ui;

private slots:
    void on_autoClosePushButton_clicked();
};

#endif // AUTOCLOSEDIALOG_H
