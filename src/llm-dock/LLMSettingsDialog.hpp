#ifndef LLMSETTINGSDIALOG_HPP
#define LLMSETTINGSDIALOG_HPP

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class SettingsDialog;
}
QT_END_NAMESPACE

/**
  * @brief The LLMSettingsDialog class
  * This class is used to create a settings dialog for the LLM dock.
  * The settings dialog is opened by clicking the settings button in the LLM dock.
  */
class LLMSettingsDialog : public QDialog {
	Q_OBJECT
public:
	explicit LLMSettingsDialog(QWidget *parent);
	~LLMSettingsDialog();

private:
	Ui::SettingsDialog *ui;
};

#endif // LLMSETTINGSDIALOG_HPP
