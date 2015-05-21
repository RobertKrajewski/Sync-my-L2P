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
    if (author.isEmpty())
        ui->author_label->setText(tr("Nicht verfügbar"));
    else
        ui->author_label->setText(author);
}

void message::updateSubject(QString subject)
{
    if (subject.isEmpty())
        ui->subject_label->setText(tr("Nachricht offline nicht verfügbar!"));
    else
        ui->subject_label->setText(subject);
}

void message::updateDate(QString date)
{
    ui->dates_label->setText(date);
}

void message::updateMessage(QString body)
{
    if (body.isEmpty())
        ui->message_body->setText(tr("Bitte verbinde dich mit dem L²P, um die Nachricht zu lesen!"));
    else
        ui->message_body->setText(body);
}

message::~message()
{
    delete ui;
}
