#include <QApplication>
#include <QFont>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include "Database.h"
#include "MainWindow.h"
#include "User.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setStyle("Windows");
    application.setFont(QFont("Tahoma", 9));

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
        bool accepted = false;
        const QString username = QInputDialog::getText(nullptr, "Login", "Username:", QLineEdit::Normal, "", &accepted);
        if (!accepted) return 0;
        const QString password = QInputDialog::getText(nullptr, "Login", "Password:", QLineEdit::Password, "", &accepted);
        if (!accepted) return 0;
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
