#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>

#include "engine.h"

static void append_results(QTextEdit* results, const std::vector<std::pair<std::string,int>>& hits) {
    if (hits.empty()) {
        results->append("No results.");
        return;
    }
    for (const auto& [file, count] : hits) {
        results->append(QString::number(count) + "  " + QString::fromStdString(file));
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    auto *window = new QMainWindow;
    window->setWindowTitle("File Search Engine");
    window->resize(900, 560);

    auto *central = new QWidget;
    auto *mainLayout = new QVBoxLayout;

    auto *row1 = new QHBoxLayout;
    auto *folderLabel = new QLabel("Folder:");
    auto *folderPath = new QLineEdit;
    folderPath->setPlaceholderText("Choose a folder...");
    auto *chooseBtn = new QPushButton("Choose...");
    auto *indexBtn = new QPushButton("Index");
    row1->addWidget(folderLabel);
    row1->addWidget(folderPath, 1);
    row1->addWidget(chooseBtn);
    row1->addWidget(indexBtn);
    mainLayout->addLayout(row1);

    auto *row2 = new QHBoxLayout;
    auto *queryLabel = new QLabel("Query:");
    auto *queryInput = new QLineEdit;
    queryInput->setPlaceholderText("Type a single word");
    auto *threadsLabel = new QLabel("Threads:");
    auto *threadsSpin = new QSpinBox;
    threadsSpin->setRange(1, 64);
    threadsSpin->setValue(4);
    auto *searchBtn = new QPushButton("Search");
    auto *benchBtn = new QPushButton("Benchmark");
    row2->addWidget(queryLabel);
    row2->addWidget(queryInput, 1);
    row2->addWidget(threadsLabel);
    row2->addWidget(threadsSpin);
    row2->addWidget(searchBtn);
    row2->addWidget(benchBtn);
    mainLayout->addLayout(row2);

    auto *results = new QTextEdit;
    results->setReadOnly(true);
    results->setPlaceholderText("Results will show here...");
    mainLayout->addWidget(results, 1);

    central->setLayout(mainLayout);
    window->setCentralWidget(central);
    window->show();

    Index index;
    bool indexed = false;
    bool indexing_now = false;

    QObject::connect(chooseBtn, &QPushButton::clicked, [&]() {
        QString dir = QFileDialog::getExistingDirectory(
            window, "Choose folder to index", folderPath->text()
        );
        if (!dir.isEmpty()) {
            folderPath->setText(dir);
            results->append("Selected folder: " + dir);
            indexed = false;
        }
    });

    QObject::connect(indexBtn, &QPushButton::clicked, [&]() {
        if (indexing_now) {
            results->append("Indexing already running...");
            return;
        }

        QString dir = folderPath->text().trimmed();
        if (dir.isEmpty()) {
            results->append("Please choose a folder first.");
            return;
        }
        int threads = threadsSpin->value();

        indexing_now = true;
        indexBtn->setEnabled(false);
        searchBtn->setEnabled(false);
        benchBtn->setEnabled(false);

        results->append("Indexing... (running in background)");

        // Run in background thread
        QtConcurrent::run([&, dir, threads]() {
            Index new_index;
            long long ms = build_index(dir.toStdString(), threads, new_index);

            // Back to UI thread
            QMetaObject::invokeMethod(results, [&, dir, threads, ms, new_index = std::move(new_index)]() mutable {
                index = std::move(new_index);
                indexed = true;
                indexing_now = false;

                results->append("Indexed with " + QString::number(threads) + " threads in " + QString::number(ms) + " ms.");
                indexBtn->setEnabled(true);
                searchBtn->setEnabled(true);
                benchBtn->setEnabled(true);
            });
        });
    });

    QObject::connect(searchBtn, &QPushButton::clicked, [&]() {
        if (!indexed) {
            results->append("Not indexed yet. Click 'Index' first.");
            return;
        }
        QString q = queryInput->text().trimmed();
        if (q.isEmpty()) {
            results->append("Type a query word first.");
            return;
        }

        results->append("---- Search: " + q + " ----");
        auto hits = search_word(index, q.toStdString(), 20);
        append_results(results, hits);
    });

    QObject::connect(benchBtn, &QPushButton::clicked, [&]() {
    if (indexing_now) {
        results->append("Indexing already running...");
        return;
    }

    QString dir = folderPath->text().trimmed();
    if (dir.isEmpty()) {
        results->append("Please choose a folder first.");
        return;
    }

    indexing_now = true;
    indexBtn->setEnabled(false);
    searchBtn->setEnabled(false);
    benchBtn->setEnabled(false);

    results->append("---- Benchmark (Indexing, background) ----");
    results->append("Threads\tTime(ms)");

    QtConcurrent::run([&, dir]() {
        std::vector<int> thread_counts = {1, 2, 4, 8};

        for (int t : thread_counts) {
            Index tmp;
            long long ms = build_index(dir.toStdString(), t, tmp);

            QMetaObject::invokeMethod(results, [&, t, ms]() {
                results->append(QString::number(t) + "\t" + QString::number(ms));
            });
        }

        QMetaObject::invokeMethod(results, [&]() {
            indexing_now = false;
            indexBtn->setEnabled(true);
            searchBtn->setEnabled(true);
            benchBtn->setEnabled(true);
        });
    });
});


    return app.exec();
}
