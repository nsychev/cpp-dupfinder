#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTreeWidgetItem>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void select_directory();
    void scan_directory();
    void cancel_scan();

    void delete_files();

    void update_progress_bar(int progress);
    void update_status(QString message);
    void scan_finished();

    void get_file(QByteArray hash, QString path, bool default_delete = true);

private:
    Ui::MainWindow *ui;
    QString path;
    QThread *job;

    QMap<QByteArray, QTreeWidgetItem*> nodes;

    QString to_human(qint64 size);
};

#endif // MAINWINDOW_H
