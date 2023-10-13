#include <QtWidgets>

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

public slots:

private:
};
