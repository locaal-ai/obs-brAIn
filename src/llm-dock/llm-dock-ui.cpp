#include <QtWidgets>

#include <obs-frontend-api.h>

#include <regex>

#include "plugin-support.h"
#include "llm-dock-ui.hpp"
#include "llm-dock.h"
#include "llama-inference.h"
#include "LLMSettingsDialog.hpp"
#include "llm-config-data.h"
#include "ui/ui_dockwidget.h"
#include "Workflows.hpp"

QDockWidget *createLLMDockWidget(QMainWindow *parent);

void register_llm_dock(void)
{
	// load plugin settings from config
	if (loadConfig() == OBS_BRAIN_CONFIG_SUCCESS) {
		obs_log(LOG_INFO, "Loaded LLM config from config file");
	} else {
		obs_log(LOG_INFO, "Failed to load LLM config from config file");
	}

	if (global_llm_config.local) {
		obs_log(LOG_INFO, "Using local LLM model: %s",
			global_llm_config.local_model_path.c_str());
		// initialize the local LLM model
		if (global_llm_config.local_model_path.empty()) {
			obs_log(LOG_ERROR, "LLM Model not found.");
		} else {
			global_llm_context.ctx_llama =
				llama_init_context(global_llm_config.local_model_path);

			// If the model is loaded successfully, register the GPT dock
			if (global_llm_context.ctx_llama == nullptr) {
				obs_log(LOG_ERROR, "Failed to load LLM model from %s.",
					global_llm_config.local_model_path.c_str());
				global_llm_context.error_message =
					"Failed to load local LLM model.";
				return;
			}
		}
	} else {
		obs_log(LOG_INFO, "Using cloud LLM model: %s",
			global_llm_config.cloud_model_name.c_str());
	}

	// register the GPT dock
	obs_frontend_add_dock(createLLMDockWidget((QMainWindow *)obs_frontend_get_main_window()));
}

QDockWidget *createLLMDockWidget(QMainWindow *parent)
{
	QDockWidget *dock = new LLMDockWidgetUI(parent);
	dock->setObjectName("LLMDockWidget");
	dock->setWindowTitle("LLM Dock");
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
	return dock;
}

LLMDockWidgetUI::LLMDockWidgetUI(QWidget *parent) : QDockWidget(parent), ui(new Ui::BrainDock)
{
	ui->setupUi(this);

	// connect the settings button to open the settings dialog
	this->connect(this->ui->settings, &QPushButton::clicked, this, [=]() {
		// open the settings dialog
		LLMSettingsDialog *settings_dialog = new LLMSettingsDialog(this);
		settings_dialog->show();
	});

	this->connect(this->ui->generate, &QPushButton::clicked, this, &LLMDockWidgetUI::generate);
	this->connect(this->ui->clear, &QPushButton::clicked, this, &LLMDockWidgetUI::clear);
	this->connect(this, &LLMDockWidgetUI::update_text_signal, this,
		      &LLMDockWidgetUI::update_text);
	// connect workflows
	this->connect(this->ui->workflows, &QPushButton::clicked, this, [=]() {
		Workflows *workflows_dialog = new Workflows(this);
		workflows_dialog->show();
	});
}

LLMDockWidgetUI::~LLMDockWidgetUI() {}

void LLMDockWidgetUI::generate()
{
	QString input_text = this->ui->prompt->toPlainText();
	if (input_text.isEmpty()) {
		return;
	}

	this->ui->generated->insertHtml(
		QString("<p style=\"color:#ffffff;\">%1</p><br/>").arg(input_text));
	this->ui->generated->moveCursor(QTextCursor::End);
	this->ui->prompt->clear();
	// also clear any styles
	this->ui->prompt->setStyleSheet("QTextEdit { background-color: #000000; color: #ffffff; }");

	// call LLM inference on a separate thread using a lambda function
	std::thread t([input_text, this]() {
		std::string generated_text = llama_inference(
			input_text.toStdString(), global_llm_context.ctx_llama,
			[this](const std::string &partial_generation) {
				emit update_text_signal(QString::fromStdString(partial_generation),
							true);
			});
		emit update_text_signal(QString("<br/>"), true);
	});
	t.detach();
}

void LLMDockWidgetUI::clear()
{
	this->ui->prompt->clear();
	this->ui->generated->clear();
}

void LLMDockWidgetUI::update_text(const QString &text, bool partial_generation)
{
	if (partial_generation) {
		if (text.isEmpty()) {
			return;
		}
		// if text is just a new line, append a <br> tag
		if (text == "\n") {
			this->ui->generated->insertHtml("<br>");
			return;
		}
		// replace spaces with non-breaking spaces
		QString text_with_non_breaking_spaces = text;
		text_with_non_breaking_spaces.replace(" ", "&nbsp;");

		// append text in a different color
		this->ui->generated->insertHtml(QString("<span style=\"color:#00ff00;\">%1</span>")
							.arg(text_with_non_breaking_spaces));
	} else {
		this->ui->generated->insertHtml(
			QString("<p style=\"color:#ffffff;\">%1</p>").arg(text));
	}
	// always scroll to the bottom
	this->ui->generated->moveCursor(QTextCursor::End);
}
