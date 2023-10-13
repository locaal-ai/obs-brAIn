#include "LLMSettingsDialog.hpp"
#include "llama-inference.h"

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
  // - end of text token

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
	// set value from request_data
	// filePathLineEdit->setText(QString::fromStdString(request_data->url));
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
  cloud_llm_form_layout->addRow("OpenAI API Key", openai_api_key_input);

  // add openai engine input
  QLineEdit *openai_engine_input = new QLineEdit(this);
  openai_engine_input->setPlaceholderText("OpenAI Engine");
  cloud_llm_form_layout->addRow("OpenAI Engine", openai_engine_input);
  // default engine gpt-3.5-turbo
  openai_engine_input->setText("gpt-3.5-turbo");

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
  general_llm_form_layout->addRow("LLM", llm_selector);

  // add system prompt input with a multi-line text edit
  QTextEdit *system_prompt_input = new QTextEdit(this);
  system_prompt_input->setPlaceholderText("System Prompt");
  general_llm_form_layout->addRow("System Prompt", system_prompt_input);
  // set the system prompt default
  system_prompt_input->setText(QString::fromStdString(LLAMA_DEFAULT_SYSTEM_PROMPT));

  // add max number of tokens input
  QLineEdit *max_tokens_input = new QLineEdit(this);
  max_tokens_input->setPlaceholderText("Max Tokens");
  general_llm_form_layout->addRow("Max Tokens", max_tokens_input);
  // set the max tokens default
  max_tokens_input->setText("64");

  // add temperature input
  QLineEdit *temperature_input = new QLineEdit(this);
  temperature_input->setPlaceholderText("Temperature");
  general_llm_form_layout->addRow("Temperature", temperature_input);
  // set the temperature default
  temperature_input->setText("0.9");
}

LLMSettingsDialog::~LLMSettingsDialog()
{
}
