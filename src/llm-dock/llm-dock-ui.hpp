#ifndef LLMDOCKWIDGETUI_HPP
#define LLMDOCKWIDGETUI_HPP

#include <QDockWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class BrainDock;
}
QT_END_NAMESPACE

class LLMDockWidgetUI : public QDockWidget {
	Q_OBJECT
public:
	explicit LLMDockWidgetUI(QWidget *parent);
	~LLMDockWidgetUI();

public slots:
	void generate();
	void clear();
    void stop();
	void update_text(const QString &text, bool partial_generation);

signals:
	void update_text_signal(const QString &text, bool partial_generation);

private:
	Ui::BrainDock *ui;
    bool stop_flag = false;
};

#endif // LLMDOCKWIDGETUI_HPP
