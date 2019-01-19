#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "file_scanner.hpp"

#include <QAction>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->progressBar->setValue(0);
    ui->fileList->header()->resizeSection(0, 400);

    connect(ui->dirSelect, &QAbstractButton::clicked, this, &MainWindow::select_directory);
    connect(ui->searchStart, &QAbstractButton::clicked, this, &MainWindow::scan_directory);
    connect(ui->searchCancel, &QAbstractButton::clicked, this, &MainWindow::cancel_scan);

    connect(ui->deleteStart, &QAbstractButton::clicked, this, &MainWindow::delete_files);
}

MainWindow::~MainWindow()
{
    if (job)
        job->requestInterruption();
    delete ui;
}

void MainWindow::select_directory() {
    path = QFileDialog::getExistingDirectory(
        this,
        "Select directory to find duplicates"
    );

    ui->dirPath->setText(path);

    ui->searchStart->setEnabled(path.size() > 0);
}

void MainWindow::scan_directory() {
    nodes.clear();
    ui->fileList->clear();
    ui->searchStart->setEnabled(false);
    ui->searchCancel->setEnabled(true);
    ui->taskLabel->setText("Scanning the directory...");
    ui->progressBar->setValue(0);

    FileScanner *file_scanner = new FileScanner(path);
    job = new QThread;

    connect(job, &QThread::started, file_scanner, &FileScanner::scan_directory);

    connect(file_scanner, &FileScanner::finished, this, &MainWindow::scan_finished);
    connect(file_scanner, &FileScanner::set_progress_bar, this, &MainWindow::update_progress_bar);
    connect(file_scanner, &FileScanner::set_status, this, &MainWindow::update_status);
    connect(file_scanner, &FileScanner::send_file, this, &MainWindow::get_file);

    connect(job, &QThread::finished, file_scanner, &FileScanner::deleteLater);
    connect(job, &QThread::finished, job, &QThread::deleteLater);

    file_scanner->moveToThread(job);
    job->start();
}

void MainWindow::update_progress_bar(int progress) {
    ui->progressBar->setValue(progress);
}

void MainWindow::update_status(QString message) {
    ui->taskLabel->setText(message);
}

void MainWindow::scan_finished() {
    ui->searchStart->setEnabled(true);
    ui->searchCancel->setEnabled(false);
    ui->deleteStart->setEnabled(true);
}

void MainWindow::get_file(QByteArray hash, QString path, bool default_delete) {
    if (nodes.find(hash) == nodes.end()) {
        QTreeWidgetItem *node = new QTreeWidgetItem(
            ui->fileList,
            QStringList(QString(hash.toHex().toUpper()).left(6))
        );
        nodes[hash] = node;
        ui->fileList->expandItem(node);
    }

    QTreeWidgetItem *item = new QTreeWidgetItem(
        nodes[hash],
        QStringList({path, to_human(QFile(path).size())})
    );

    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, default_delete ? Qt::Checked : Qt::Unchecked);
}

void MainWindow::cancel_scan() {
    if (job)
        job->requestInterruption();
}

void MainWindow::delete_files() {
    ui->searchStart->setEnabled(false);
    ui->deleteStart->setEnabled(false);
    ui->taskLabel->setText("Deleting files...");
    ui->progressBar->setValue(0);

    QTreeWidgetItemIterator it(ui->fileList);

    while (*it) {
        if (!(*it)->isHidden() && (*it)->checkState(0) == Qt::Checked) {
            QFile file((*it)->text(0));
            if (!file.remove()) {
                (*it)->setDisabled(true);
                (*it)->setCheckState(0, Qt::Unchecked);
                QMessageBox::warning(
                    this,
                    "Error",
                    QString("Can't delete file ") + (*it)->text(0)
                );
            } else {
                (*it)->setHidden(true);
            }
        }
        it++;
    }

    ui->searchStart->setEnabled(true);
    ui->deleteStart->setEnabled(true);
    ui->taskLabel->setText("Files are deleted.");
    ui->progressBar->setValue(100);
}

QString MainWindow::to_human(qint64 size) {
    QString result;
    if (size < 1024) {
        QTextStream(&result) << size << " B";
    } else if (size < 1048576) {
        QTextStream(&result) << size / 1024 << " K";
    } else if (size < 1073741824) {
        QTextStream(&result) << size / 1048576 << " M";
    } else {
        QTextStream(&result) << size / 1073741824 << " G";
    }

    return result;
}

