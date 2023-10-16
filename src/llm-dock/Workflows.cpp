#include <QtWidgets>

#include "Workflows.hpp"
#include "ui/ui_workflows.h"
#include "ui/ui_workflow.h"
#include "plugin-support.h"
#include "llm-config-data.h"

#include <nlohmann/json.hpp>
#include <obs-module.h>

class Workflow : public QWidget {
public:
	Workflow(QWidget *parent = nullptr) : QWidget(parent), ui(new Ui::Workflow)
	{
		ui->setupUi(this);
	}
	~Workflow() { delete ui; }

	Ui::Workflow *ui;
};

Workflows::Workflows(QWidget *parent) : QDialog(parent), ui(new Ui::Workflows)
{
	ui->setupUi(this);

	// get the list of workflows from the OBS module settings and add them to the list
	for (size_t i = 0; i < global_llm_config.workflows.size(); i++) {
		nlohmann::json workflowJson = nlohmann::json::parse(global_llm_config.workflows[i]);
		Workflow *workflow = new Workflow(this);
		ui->workflowsLayout->addWidget(workflow);
		workflow->ui->workflowGroupBox->setTitle(
			"Workflow " + QString::number(ui->workflowsLayout->count()));
		workflow->ui->prompt->setPlainText(QString::fromStdString(workflowJson["prompt"]));
		workflow->ui->source->setCurrentText(
			QString::fromStdString(workflowJson["source"]));
		workflow->ui->sourceFile->setText(
			QString::fromStdString(workflowJson["sourceFile"]));
		workflow->ui->target->setCurrentText(
			QString::fromStdString(workflowJson["target"]));
		workflow->ui->targetFile->setText(
			QString::fromStdString(workflowJson["targetFile"]));
		workflow->ui->localCloud->setCurrentText(
			QString::fromStdString(workflowJson["localOrCloud"]));
		workflow->ui->streaming->setChecked(workflowJson["streaming"]);
		workflow->ui->trigger->setCurrentText(
			QString::fromStdString(workflowJson["trigger_onChange_or_periodic"]));
		workflow->ui->timeMs->setText(QString::number((int)workflowJson["triggerMs"]));
	}

	connect(ui->add, &QPushButton::clicked, this, [=]() {
		// Add a new workflow to the list
		Workflow *workflow = new Workflow(this);
		ui->workflowsLayout->addWidget(workflow);
		workflow->ui->workflowGroupBox->setTitle(
			"Workflow " + QString::number(ui->workflowsLayout->count()));
		this->adjustSize();
	});

	connect(ui->save, &QPushButton::clicked, this, [=]() {
		// Serialize all workflows to json and save to the OBS module settings
		global_llm_config.workflows.clear();
		for (int i = 0; i < ui->workflowsLayout->count(); i++) {
			Workflow *workflow = (Workflow *)ui->workflowsLayout->itemAt(i)->widget();
			nlohmann::json workflowJson;
			workflowJson["prompt"] = workflow->ui->prompt->toPlainText().toStdString();
			workflowJson["source"] =
				workflow->ui->source->itemText(workflow->ui->source->currentIndex())
					.toStdString();
			workflowJson["sourceFile"] = workflow->ui->sourceFile->text().toStdString();
			workflowJson["target"] =
				workflow->ui->target->itemText(workflow->ui->target->currentIndex())
					.toStdString();
			workflowJson["targetFile"] = workflow->ui->targetFile->text().toStdString();
			workflowJson["localOrCloud"] =
				workflow->ui->localCloud
					->itemText(workflow->ui->localCloud->currentIndex())
					.toStdString();
			workflowJson["streaming"] = workflow->ui->streaming->isChecked();
			workflowJson["trigger_onChange_or_periodic"] =
				workflow->ui->trigger
					->itemText(workflow->ui->trigger->currentIndex())
					.toStdString();
			workflowJson["triggerMs"] = workflow->ui->timeMs->text().toUInt();
			global_llm_config.workflows.push_back(workflowJson.dump());
		}
		if (saveConfig() == OBS_BRAIN_CONFIG_SUCCESS) {
			obs_log(LOG_INFO, "Saved LLM settings");
		} else {
			obs_log(LOG_ERROR, "Failed to save LLM settings");
		}
		// close the dialog
		this->close();
	});
}

Workflows::~Workflows()
{
	delete ui;
}
