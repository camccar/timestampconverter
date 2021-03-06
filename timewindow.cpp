#include "timewindow.h"
#include "ui_timewindow.h"
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#include <ctime>
#include <QDateTime>
#include <QListWidgetItem>
#include <QKeyEvent>
#include "about.h"
#include <chrono>
TimeWindow::TimeWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TimeWindow)
{
    startTimer(1000);
    ui->setupUi(this);
    setWindowTitle("Unix Time Stamp Converter");
    connect(ui->ConvertButton,SIGNAL(released()),this,SLOT(CalculateTime()));

    connect(ui->timeformat,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(SetSecondText()));

    connect(ui->listWidget , SIGNAL(itemClicked(QListWidgetItem*)),
                this, SLOT(RetrieveHistoryItem(QListWidgetItem*)));

    connect(ui->toolButton,SIGNAL(released()),this,SLOT(OpenAbout()));
    connect(ui->DisplayCurrentTime,SIGNAL(released()),this,SLOT(setCurrentTime()));

     ui->listWidget->installEventFilter(this);

    //ui->listWidget->event()
     t = new Database();
     auto historyvalues = t->getHistory();
     for(auto it = historyvalues.begin(); it < historyvalues.end(); ++it){
         QListWidgetItem* item = new QListWidgetItem();
         item->setData(Qt::UserRole,(*it).second);
         item->setText((*it).first);

         ui->listWidget->insertItem(0,item);
     }
     auto secondtype = t->getSecondType();
     int index = ui->timeformat->findText(secondtype);
     if(index != -1){
         ui->timeformat->setCurrentIndex(index);
     }
     wdg = new About();

    ui->currentstamp->setText(QTime::currentTime().toString("hh:mm:ss"));

}

TimeWindow::~TimeWindow()
{
    delete ui;
    delete wdg;
    delete t;
}

void TimeWindow::timerEvent(QTimerEvent * event){

    std::chrono::time_point<std::chrono::system_clock> timestamp =
        std::chrono::system_clock::now();

        //QString::number(QDateTime::currentSecsSinceEpoch()

      auto timeStampString = QString();

      if(ui->timeformat->currentText() == "Nanoseconds"){
          const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>
            (timestamp.time_since_epoch()).count();
          timeStampString = QString::number(ns);
      }
       else if(ui->timeformat->currentText() == "Milliseconds"){
          const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>
            (timestamp.time_since_epoch()).count();
           timeStampString = QString::number(ms);
       }else{
          const auto ss = std::chrono::duration_cast<std::chrono::seconds>
            (timestamp.time_since_epoch()).count();
           timeStampString = QString::number(ss);
       }


    QTime UtcTime  = QDateTime::currentDateTime ().toTimeSpec(Qt::UTC).time();
    ui->currentstamp->setText("Local Time " +QTime::currentTime().toString("hh:mm:ss") + " "+" UTC "+UtcTime.toString() + " Timestamp " +timeStampString);

}

void TimeWindow::OpenAbout(){
   wdg->show();
}
void TimeWindow::setCurrentTime(){

    std::chrono::time_point<std::chrono::system_clock> timestamp =
        std::chrono::system_clock::now();

      if(ui->timeformat->currentText() == "Nanoseconds"){
          const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>
            (timestamp.time_since_epoch()).count();
          ui->TimeStampInput->setText(QString::number(ns));
      }
       else if(ui->timeformat->currentText() == "Milliseconds"){
          const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>
            (timestamp.time_since_epoch()).count();
           ui->TimeStampInput->setText(QString::number(ms));
       }else{
          const auto ss = std::chrono::duration_cast<std::chrono::seconds>
            (timestamp.time_since_epoch()).count();
           ui->TimeStampInput->setText(QString::number(ss));
       }

    CalculateTime();
}

bool TimeWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->listWidget && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Delete ||keyEvent->key() == Qt::Key_Backspace ) {
            // Special tab handling
            auto items = ui->listWidget->selectedItems();
            foreach(QListWidgetItem * item, items)
            {
                auto id = item->data(Qt::UserRole).toString();
                t->deleteHistoryItem(id);
                delete ui->listWidget->takeItem(ui->listWidget->row(item));
            }
            return true;
        } else if(keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter ){
            auto items = ui->listWidget->selectedItems();
            foreach(QListWidgetItem * item, items)
            {
                auto id = item->data(Qt::UserRole).toString();
                auto historyrecord = t->getHistoryItem(id);
                ui->TimeStampInput->setText(historyrecord.first);
                int index = ui->timeformat->findText(historyrecord.second);
                if(index != -1){
                    ui->timeformat->setCurrentIndex(index);
                }

                CalculateTime(true);

            }

            return true;
        }else{
            return false;
        }
    }
    return false;
}


void TimeWindow::SetSecondText(){
    t->setSecondType(ui->timeformat->currentText());
}
void TimeWindow::RetrieveHistoryItem(QListWidgetItem* input){
    QString id = input->data(Qt::UserRole).toString();
    std::pair<QString,QString> historyitem = t->getHistoryItem(id);
    ui->TimeStampInput->setText(historyitem.first);
    int index = ui->timeformat->findText(historyitem.second);
    if(index != -1){
        ui->timeformat->setCurrentIndex(index);
    }
    CalculateTime(true);

}


void TimeWindow::CalculateTime(bool inside) {
    QString butval = ui->TimeStampInput->toPlainText();
    butval = butval.trimmed();
    ui->TimeStampInput->setPlainText(butval);
    if(butval == ""){
        ui->ErrorOutput->setText("Error, Invalid time stamp");
        return;
    }

    QRegExp reg("[0-9.]*");
    if(reg.exactMatch(butval)){
       ui->ErrorOutput->setText("");

       long long timestamp = butval.toLong();
       long long sec;
       if(ui->timeformat->currentText() == "Nanoseconds"){
           sec = timestamp /  1000000000ULL;
       }
        else if(ui->timeformat->currentText() == "Milliseconds"){
            sec = ((timestamp + 500) / 1000);
        }else{
            sec = timestamp;
        }
        QDateTime qtimestamp = QDateTime::fromTime_t(sec);

        QString historyvalue = ui->timeformat->currentText()+" " +butval+" UTC:"+qtimestamp.toUTC().toString()+ "    Local:"+qtimestamp.toString();

        if(!inside){

           QString id = t->insertRow(butval,ui->timeformat->currentText());
           QListWidgetItem* hv = new QListWidgetItem();
           hv->setData(Qt::UserRole,id);
           hv->setText(historyvalue);
           ui->listWidget->insertItem(0,hv);
        }
        ui->utcOutput->setText(qtimestamp.toUTC().toString());
        ui->CentralOutput->setText(qtimestamp.toString());
    }else{
        ui->ErrorOutput->setText("Error, Invalid time stamp");
    }

}

