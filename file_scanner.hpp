#ifndef FILE_SCANNER_HPP
#define FILE_SCANNER_HPP

#include <QObject>
#include <map>
#include <QVector>
#include <QString>
#include <optional>
#include <fstream>

const std::string LOG_FILE_NAME = "dupfinder.log";
const qint64 BLOCK_SIZE = 8192;


class FileScanner : public QObject
{
    Q_OBJECT
public:
    FileScanner(QString path);
    ~FileScanner();

public slots:
    void scan_directory();

signals:
    void set_progress_bar(int progress);
    void finished();
    void set_status(QString message);
    void send_file(QByteArray hash, QString path, bool default_delete = true);

private:
    std::ofstream log_file;

    QString path;
    std::map<qint64, QVector<QString>> files_by_size;
    std::map<QByteArray, QVector<QString>> files_by_hash;
};

#endif // FILE_SCANNER_HPP
