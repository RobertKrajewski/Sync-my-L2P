#ifndef MESSAGE_H
#define MESSAGE_H

#include <QDialog>

namespace Ui {
class message;
}

class message : public QDialog
{
    Q_OBJECT

public:
    explicit message(QWidget *parent = 0);
    ~message();

private:
    Ui::message *ui;
};

#endif // MESSAGE_H
