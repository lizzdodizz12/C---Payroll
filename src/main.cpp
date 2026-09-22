#include <QApplication>
#include <QDialog>
#include <QFont>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include "Database.h"
#include "MainWindow.h"
#include "User.h"

namespace {
bool showLoginDialog(QString& username, QString& password) {
    QDialog dialog(nullptr);
    dialog.setWindowTitle("Payroll Management System");
    dialog.setFixedSize(420, 260);
    dialog.setModal(true);
    dialog.setStyleSheet(R"(
        QDialog { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0f172a, stop:1 #1e3a8a); }
        QLabel { color: #e2e8f0; }
        QLineEdit { background: rgba(255,255,255,0.08); border: 1px solid rgba(148,163,184,0.7); border-radius: 10px; padding: 10px; color: white; }
        QPushButton { background: #2563eb; color: white; border: none; border-radius: 10px; padding: 10px 18px; font-weight: 700; }
        QPushButton:hover { background: #1d4ed8; }
    )");

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel("Sign in to continue");
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: white;");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* form = new QFormLayout;
    auto* usernameInput = new QLineEdit;
    usernameInput->setPlaceholderText("Username");
    auto* passwordInput = new QLineEdit;
    passwordInput->setPlaceholderText("Password");
    passwordInput->setEchoMode(QLineEdit::Password);
    form->addRow("Username:", usernameInput);
    form->addRow("Password:", passwordInput);
    root->addLayout(form);

    auto* buttons = new QHBoxLayout;
    auto* cancelButton = new QPushButton("Cancel");
    auto* loginButton = new QPushButton("Login");
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(loginButton);
    root->addLayout(buttons);

    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(loginButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(usernameInput, &QLineEdit::returnPressed, &dialog, &QDialog::accept);
    QObject::connect(passwordInput, &QLineEdit::returnPressed, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    username = usernameInput->text();
    password = passwordInput->text();
    return true;
}
}

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setStyle("Fusion");
    application.setFont(QFont("Segoe UI", 9));

    Database database("database/payroll.db");
    if (!database.connect() || !database.initializeDatabase()) {
        QMessageBox::critical(nullptr, "Payroll Management System", "Unable to connect to the database.");
        return 1;
    }
    if (!User::ensureDefaultAdministrator(database)) {
        QMessageBox::critical(nullptr, "Payroll Management System", "Unable to initialize user accounts.");
        return 1;
    }

    User currentUser;
    for (int attempt = 0; attempt < 3 && currentUser.getID() == 0; ++attempt) {
        QString username;
        QString password;
        if (!showLoginDialog(username, password)) {
            return 0;
        }

        if (!User::login(database, username.toStdString(), password.toStdString(), currentUser)) {
            QMessageBox::warning(nullptr, "Login", "Invalid username or password.");
        }
    }
    if (currentUser.getID() == 0) {
        QMessageBox::critical(nullptr, "Login", "Authentication failed.");
        return 1;
    }

    MainWindow window(database, currentUser);
    window.show();
    const int result = application.exec();
    database.disconnect();
    return result;
}
