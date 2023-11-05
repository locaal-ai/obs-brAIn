#include <QtWidgets>

#include "llama-inference.h"
#include "llm-config-data.h"
#include "plugin-support.h"

#include "LLMSettingsDialog.hpp"
#include "ui/ui_settingsdialog.h"

#include <obs-module.h>

LLMSettingsDialog::LLMSettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);
	// load settings from config
	ui->sysPrompt->setPlainText(QString::fromStdString(global_llm_config.system_prompt));
	ui->maxTokens->setText(QString::number(global_llm_config.max_output_tokens));
	ui->temperature->setText(QString::number(global_llm_config.temperature));
	ui->apiKey->setText(QString::fromStdString(global_llm_config.cloud_api_key));
	ui->apiModel->setText(QString::fromStdString(global_llm_config.cloud_model_name));
	ui->localLlmPath->setText(QString::fromStdString(global_llm_config.local_model_path));
	ui->dockLLM->setCurrentIndex(global_llm_config.local ? 0 : 1);

	// File dialog
	connect(this->ui->localLlmPathButton, &QPushButton::clicked, this, [=]() {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
								tr("Model Files (*.gguf)"));
		if (fileName != "") {
			this->ui->localLlmPath->setText(fileName);
		}
	});

	// connect to the dialog Save action to save the settings
	this->connect(this->ui->buttonBox, &QDialogButtonBox::accepted, this, [=]() {
		// get settings from UI into config struct
		global_llm_config.local = this->ui->dockLLM->currentIndex() == 0;
		global_llm_config.local_model_path = this->ui->localLlmPath->text().toStdString();
		global_llm_config.cloud_api_key = this->ui->apiKey->text().toStdString();
		global_llm_config.cloud_model_name = this->ui->apiModel->text().toStdString();
		global_llm_config.system_prompt = this->ui->sysPrompt->toPlainText().toStdString();
		global_llm_config.end_sequence = this->ui->endSeq->text().toStdString();
		global_llm_config.max_output_tokens = this->ui->maxTokens->text().toUShort();
		global_llm_config.temperature = this->ui->temperature->text().toFloat();

		// serialize to json and save to the OBS module settings
		if (saveConfig() == OBS_BRAIN_CONFIG_SUCCESS) {
			obs_log(LOG_INFO, "Saved LLM settings");
		} else {
			obs_log(LOG_ERROR, "Failed to save LLM settings");
		}

		// close the dialog
		this->close();
	});
}

LLMSettingsDialog::~LLMSettingsDialog() {}
