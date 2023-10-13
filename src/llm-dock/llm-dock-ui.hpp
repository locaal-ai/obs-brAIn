#include <QtWidgets>

class LLMDockWidgetUI : public QWidget {
	Q_OBJECT
public:
	explicit LLMDockWidgetUI(QWidget *parent, void *llm_ctx);
	~LLMDockWidgetUI();

public slots:
	void generate();
	void clear();
	void update_text(const QString &text, bool partial_generation);

signals:
	void update_text_signal(const QString &text, bool partial_generation);

private:
	void *llm_ctx;
	QVBoxLayout *layout;
	QTextEdit *text_edit;
	QTextEdit *input_text_edit;
	QHBoxLayout *button_layout;
	QPushButton *generate_button;
	QPushButton *clear_button;
	QPushButton *settings_button;
};
