#include "AboutWindow.h"

AboutWindow::AboutWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::AboutWindow) {
    ui->setupUi(this);
    setWindowTitle("About");

    // Создаем центральный виджет
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    // Вертикальный лэйаут
    QVBoxLayout* verticalLayout = new QVBoxLayout(central);

    // Заголовок
    QLabel* titleLabel = new QLabel("<h2>О программе</h2>", central);
    verticalLayout->addWidget(titleLabel);

    // Описание программы
    QLabel* descriptionLabel = new QLabel(
        "<b>WFS04</b><br>"
        "Версия 1.0.0<br><br>"
        "Программа для работы с файловой системой WFS.<br>"
        "Позволяет на основании анализа структуры файловой системы<br>"
        "извлекать имеющиеся видеофрагменты.<br><br>"
        "Разработано с использованием Qt Framework.<br>"
        "© 2025 <a href=\"https://github.com/maxgpt03\" style=\"color:#2979FF; text-decoration:none;\">maxgpt03</a><br><br>"
        "<a href=\"https://github.com/maxgpt03/wfs04\" style=\"color:#2979FF; text-decoration:none;\">"
        "GitHub проект программы</a>",
        central);
    descriptionLabel->setTextFormat(Qt::RichText);
    descriptionLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    descriptionLabel->setOpenExternalLinks(true);
    descriptionLabel->setWordWrap(true);

    verticalLayout->addWidget(descriptionLabel);

    // Кнопка закрытия
    QPushButton* closeButton = new QPushButton("Закрыть", central);

    // Лейаут для кнопки
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();         // Добавление отступа перед кнопкой
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();         // Добавление отступа после кнопки

    verticalLayout->addLayout(buttonLayout);

    // Подключаем кнопку к слоту close окна
    connect(closeButton, &QPushButton::clicked, this, &QMainWindow::close);

    // Загрузка стиля
    QFile file(":/styles/AboutWindow.qss");
    if (file.open(QFile::ReadOnly)) {
        QString style = QLatin1String(file.readAll());
        this->setStyleSheet(style);
    }
}

AboutWindow::~AboutWindow()
{
    delete ui;
}