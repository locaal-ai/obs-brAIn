#include "LLMSettingsDialog.hpp"
#include "llama-inference.h"
#include "llm-config-data.h"
#include "plugin-support.h"

#include <obs-module.h>

LLMSettingsDialog::LLMSettingsDialog(QWidget *parent) : QDialog(parent)
{
	// Create a dialog with settings for LLMs usage in OBS
	// The settings should allow to select local or cloud LLM (OpenAI API)
	// The local LLM should allow selecting a .gguf model file for llama.cpp
	// The cloud LLM should allow entering an API key for OpenAI API
	// The settings should allow to edit the following:
	// - system prompt for LLM
	// - max number of tokens to generate
	// - temperature

	setWindowTitle("LLM Settings");
	setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
	setModal(true);
	// set a minimum width for the dialog
	setMinimumWidth(500);

	// Create a layout for the dialog
	QGridLayout *layout = new QGridLayout(this);

	// Create a tab widget for the dialog
	QTabWidget *tab_widget = new QTabWidget(this);

	// Create a tab for local LLM settings
	QWidget *local_llm_tab = new QWidget(this);
	QGridLayout *local_llm_tab_layout = new QGridLayout(local_llm_tab);
	local_llm_tab->setLayout(local_llm_tab_layout);

	// Create a tab for cloud LLM settings
	QWidget *cloud_llm_tab = new QWidget(this);
	QGridLayout *cloud_llm_tab_layout = new QGridLayout(cloud_llm_tab);
	cloud_llm_tab->setLayout(cloud_llm_tab_layout);

	// Create a tab for general LLM settings
	QWidget *general_llm_tab = new QWidget(this);
	QGridLayout *general_llm_tab_layout = new QGridLayout(general_llm_tab);
	general_llm_tab->setLayout(general_llm_tab_layout);

	// Add the tabs to the tab widget
	tab_widget->addTab(general_llm_tab, "General");
	tab_widget->addTab(local_llm_tab, "Local LLM");
	tab_widget->addTab(cloud_llm_tab, "Cloud LLM");

	// Add the tab widget to the layout
	layout->addWidget(tab_widget);

	this->setLayout(layout);
	this->setWindowTitle("LLM Settings");
	this->resize(600, 400);

	// Use a form layout for the local LLM tab
	QFormLayout *local_llm_form_layout = new QFormLayout(local_llm_tab);
	local_llm_form_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

	// Model file path
	QHBoxLayout *fileInputLayout = new QHBoxLayout;
	local_llm_form_layout->addRow("Model File", fileInputLayout);

	QLineEdit *filePathLineEdit = new QLineEdit;
	filePathLineEdit->setPlaceholderText("Model File");
	// set value from config
	filePathLineEdit->setText(QString::fromStdString(global_llm_config.local_model_path));
	fileInputLayout->addWidget(filePathLineEdit);
	// add file selector button if file is selected
	QPushButton *fileButton = new QPushButton("...");
	fileInputLayout->addWidget(fileButton);

	// File dialog
	connect(fileButton, &QPushButton::clicked, this, [=]() {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
								tr("Model Files (*.gguf)"));
		if (fileName != "") {
			filePathLineEdit->setText(fileName);
		}
	});

	// Add the form layout to the local LLM tab layout
	local_llm_tab_layout->addLayout(local_llm_form_layout, 0, 0, 1, 1);

	/** CLOUD */
	// Use a form layout for the cloud LLM tab
	QFormLayout *cloud_llm_form_layout = new QFormLayout(cloud_llm_tab);
	// set growing policy for the form layout
	cloud_llm_form_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

	// Add the form layout to the cloud LLM tab layout
	cloud_llm_tab_layout->addLayout(cloud_llm_form_layout, 0, 0, 1, 1);

	// add openai api key input
	QLineEdit *openai_api_key_input = new QLineEdit(this);
	openai_api_key_input->setPlaceholderText("sk-...");
	openai_api_key_input->setText(QString::fromStdString(global_llm_config.cloud_api_key));
	cloud_llm_form_layout->addRow("OpenAI API Key", openai_api_key_input);

	// add openai engine input
	QLineEdit *openai_engine_input = new QLineEdit(this);
	openai_engine_input->setPlaceholderText("OpenAI Engine");
	openai_engine_input->setText(QString::fromStdString(global_llm_config.cloud_model_name));
	cloud_llm_form_layout->addRow("OpenAI Engine", openai_engine_input);

	/** GENERAL */
	// Use a form layout for the general LLM tab
	QFormLayout *general_llm_form_layout = new QFormLayout(general_llm_tab);
	// auto stretch the form layout
	general_llm_form_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

	// Add the form layout to the general LLM tab layout
	general_llm_tab_layout->addLayout(general_llm_form_layout, 0, 0, 1, 1);

	// add a selector between local and clod llm
	QComboBox *llm_selector = new QComboBox(this);
	llm_selector->addItem("Local LLM");
	llm_selector->addItem("Cloud LLM");
	// set the default
	llm_selector->setCurrentIndex(global_llm_config.local ? 0 : 1);
	general_llm_form_layout->addRow("LLM", llm_selector);

	// add system prompt input with a multi-line text edit
	QTextEdit *system_prompt_input = new QTextEdit(this);
	system_prompt_input->setPlaceholderText("System Prompt");
	general_llm_form_layout->addRow("System Prompt", system_prompt_input);
	// set the system prompt default
	system_prompt_input->setText(QString::fromStdString(global_llm_config.system_prompt));

	// add max number of tokens input
	QLineEdit *max_tokens_input = new QLineEdit(this);
	max_tokens_input->setPlaceholderText("Max Tokens");
	general_llm_form_layout->addRow("Max Tokens", max_tokens_input);
	// set the max tokens default
	max_tokens_input->setText(QString::number(global_llm_config.max_output_tokens));

	// add temperature input
	QLineEdit *temperature_input = new QLineEdit(this);
	temperature_input->setPlaceholderText("Temperature");
	general_llm_form_layout->addRow("Temperature", temperature_input);
	// set the temperature default
	temperature_input->setText(QString::number(global_llm_config.temperature));

	// add a save button to save all the settings, add to the grid layout of the dialog
	QPushButton *save_button = new QPushButton("Save and Close", this);
	layout->addWidget(save_button);

	// connect the save button to save the settings
	this->connect(save_button, &QPushButton::clicked, this, [=]() {
		// get settings from UI into config struct
		global_llm_config.local = llm_selector->currentIndex() == 0;
		global_llm_config.local_model_path = filePathLineEdit->text().toStdString();
		global_llm_config.cloud_api_key = openai_api_key_input->text().toStdString();
		global_llm_config.cloud_model_name = openai_engine_input->text().toStdString();
		global_llm_config.system_prompt = system_prompt_input->toPlainText().toStdString();
		global_llm_config.max_output_tokens = max_tokens_input->text().toUShort();
		global_llm_config.temperature = temperature_input->text().toFloat();

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
