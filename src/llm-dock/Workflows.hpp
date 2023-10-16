#ifndef WORKFLOWS_H
#define WORKFLOWS_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class Workflows;
}
QT_END_NAMESPACE

class Workflows : public QDialog {
	Q_OBJECT

public:
	Workflows(QWidget *parent = nullptr);
	~Workflows();

private:
	Ui::Workflows *ui;
};
#endif // WORKFLOWS_H
