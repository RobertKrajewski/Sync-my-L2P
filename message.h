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

public slots:
    void updateSubject(QString subject);
    void updateDate(QString date);
    void updateMessage(QString body);
    void updateAuthor(QString author);


};

#endif // MESSAGE_H
