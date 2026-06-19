#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QStackedWidget;

namespace pyraqt::core {
class ConfigManager;
class I18nManager;
class PythonRuntimeManager;
class ThemeManager;
class UpdateManager;
class WorkspaceManager;
} // namespace pyraqt::core

namespace pyraqt::ui {

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(
        core::ConfigManager &configManager,
        core::ThemeManager &themeManager,
        core::I18nManager &i18nManager,
        core::PythonRuntimeManager &pythonRuntimeManager,
        core::UpdateManager &updateManager,
        core::WorkspaceManager &workspaceManager,
        QWidget *parent = nullptr);

private:
    void createUi();
    void populateValues();
    void applyFilter(const QString &text);
    QWidget *createGeneralPage();
    QWidget *createPythonPage();
    QWidget *createUpdatesPage();

    core::ConfigManager &m_configManager;
    core::ThemeManager &m_themeManager;
    core::I18nManager &m_i18nManager;
    core::PythonRuntimeManager &m_pythonRuntimeManager;
    core::UpdateManager &m_updateManager;
    core::WorkspaceManager &m_workspaceManager;

    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_categoryList = nullptr;
    QStackedWidget *m_pages = nullptr;
    QComboBox *m_themeCombo = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QLineEdit *m_browserRootEdit = nullptr;
    QCheckBox *m_restoreSessionCheck = nullptr;
    QLineEdit *m_interpreterEdit = nullptr;
    QSpinBox *m_timeoutSpin = nullptr;
    QCheckBox *m_autoUpdateCheck = nullptr;
    QComboBox *m_updateChannelCombo = nullptr;
};

} // namespace pyraqt::ui
