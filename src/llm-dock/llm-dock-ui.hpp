#include <QtWidgets>

class LLMDockWidgetUI : public QWidget {
	Q_OBJECT
public:
	explicit LLMDockWidgetUI(QWidget *parent);
	~LLMDockWidgetUI();

public slots:
	void generate();
	void clear();
	void update_text(const QString &text, bool partial_generation);

signals:
	void update_text_signal(const QString &text, bool partial_generation);

private:
	QVBoxLayout *layout;
	QTextEdit *text_edit;
	QTextEdit *input_text_edit;
	QHBoxLayout *button_layout;
	QPushButton *generate_button;
	QPushButton *clear_button;
	QPushButton *settings_button;
	QLabel *error_message_label;
};
