#include "message.h"
#include "ui_message.h"

message::message(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::message)
{
    ui->setupUi(this);
    ui->retranslateUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

void message::updateAuthor(QString author)
{
    ui->author_label->setText(author);
}

void message::updateSubject(QString subject)
{
    ui->subject_label->setText(subject);
}

void message::updateDate(QString date)
{
    ui->dates_label->setText(date);
}

void message::updateMessage(QString body)
{
    ui->message_body->setText(body);
}

message::~message()
{
    delete ui;
}
