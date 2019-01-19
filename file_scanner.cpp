#include "file_scanner.hpp"
#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QThread>

FileScanner::FileScanner(QString path):
    log_file(LOG_FILE_NAME, std::ostream::out | std::ostream::app),
    path(path)
{
}

FileScanner::~FileScanner() {
    log_file.close();
}

void FileScanner::scan_directory() {
    QDir dir(path);
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    qint64 total = 0;

    while (it.hasNext()) {
        it.next();

        QFileInfo file_info = it.fileInfo();

        files_by_size[file_info.size()].push_back(file_info.filePath());
        total += file_info.size();
    }

    qint64 processed = 0;

    for (auto const &[size, file_paths]: files_by_size) {
        if (file_paths.size() == 1) {
            continue;
        }

        files_by_hash.clear();
        for (auto const &path: file_paths) {
            QThread::msleep(500);
            if (QThread::currentThread()->isInterruptionRequested()) {
                emit set_status("Process was interrupted");
                emit finished();
                return;
            }

            QFile file(path);

            processed += file.size();

            emit set_progress_bar(int(processed * 100 / total));

            if (file.open(QIODevice::ReadOnly)) {
                // faster than sha-512, but still good to rely on
                QCryptographicHash hash(QCryptographicHash::Sha384);

                while (!file.atEnd()) {
                    hash.addData(file.read(BLOCK_SIZE));
                }

                QByteArray hash_value = hash.result();

                files_by_hash[hash_value].push_back(path);

                if (files_by_hash[hash_value].size() == 2)
                    emit send_file(hash_value, files_by_hash[hash_value].front(), false);

                if (files_by_hash[hash_value].size() >= 2)
                    emit send_file(hash_value, path);
            } else {
                log_file << "Can not open the file " << path.toStdString() << std::endl;
            }
        }
    }

    emit set_progress_bar(100);
    emit set_status("Scan finished. Select files to delete.");
    emit finished();

};
