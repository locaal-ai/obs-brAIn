#include <QtWidgets>

#include <obs-frontend-api.h>

#include <regex>

#include "plugin-support.h"
#include "llm-dock-ui.hpp"
#include "llm-dock.h"
// #include "../model-utils/model-downloader.h"
#include "llama-inference.h"
#include "LLMSettingsDialog.hpp"

QDockWidget *createLLMDockWidget(QMainWindow *parent, void *llm_ctx);

void register_llm_dock(void)
{
	// Find the model file
	// std::string model_file_path = find_model_file("models/ggml-gpt2-117M.bin");
	// std::string model_file_path = "/Users/roy_shilkrot/Downloads/open-llama-3b-q4_0.gguf";
	std::string model_file_path =
		"/Users/roy_shilkrot/Downloads/mistral-7b-instruct-v0.1.Q4_K_M.gguf";

	if (model_file_path.empty()) {
		// If the model file is not found, start the model downloader UI dialog
		// download_model_with_ui_dialog("ggml-gpt2-117M.bin", [](bool success) {
		//     if (success) {
		//         // If the download is successful, register the GPT dock
		//         obs_frontend_add_dock(createGPTDockWidget(obs_frontend_get_main_window()));
		//     }
		// });
		obs_log(LOG_ERROR, "LLM Model not found.");
	} else {
		struct llama_context *ctx_llama = llama_init_context(model_file_path);

		// If the model is loaded successfully, register the GPT dock
		if (ctx_llama == nullptr) {
			obs_log(LOG_ERROR, "Failed to load LLM model from %s.",
				model_file_path.c_str());
			return;
		}

		// register the GPT dock
		obs_frontend_add_dock(createLLMDockWidget(
			(QMainWindow *)obs_frontend_get_main_window(), ctx_llama));
	}
}

QDockWidget *createLLMDockWidget(QMainWindow *parent, void *llm_ctx)
{
	QDockWidget *dock = new QDockWidget(parent);
	dock->setObjectName("LLMDockWidget");
	dock->setWindowTitle("LLM Dock");
	// dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	dock->setWidget(new LLMDockWidgetUI(dock, llm_ctx));
	parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
	return dock;
}

LLMDockWidgetUI::LLMDockWidgetUI(QWidget *parent, void *llm_ctx) : QWidget(parent)
{
	this->llm_ctx = llm_ctx;

	this->layout = new QVBoxLayout(this);
	this->layout->setContentsMargins(0, 0, 0, 0);

	this->text_edit = new QTextEdit(this);
	this->text_edit->setReadOnly(true);
	// apply wrapping to break lines
	this->text_edit->setLineWrapMode(QTextEdit::WidgetWidth);
	this->text_edit->setStyleSheet("QTextEdit { background-color: #000000; color: #ffffff; }");
	this->layout->addWidget(this->text_edit);

	this->input_text_edit = new QTextEdit(this);
	this->input_text_edit->setLineWrapMode(QTextEdit::WidgetWidth);
	this->input_text_edit->setStyleSheet(
		"QTextEdit { background-color: #000000; color: #ffffff; }");
	this->layout->addWidget(this->input_text_edit);

	this->button_layout = new QHBoxLayout(this);
	this->button_layout->setContentsMargins(0, 0, 0, 0);
	this->layout->addLayout(this->button_layout);

	this->generate_button = new QPushButton("Generate", this);
	this->button_layout->addWidget(this->generate_button);

	this->clear_button = new QPushButton("Clear", this);
	this->button_layout->addWidget(this->clear_button);

	// add a button to open the settings dialog
	this->settings_button = new QPushButton("Settings", this);
	this->button_layout->addWidget(this->settings_button);

	// connect the settings button to open the settings dialog
	this->connect(this->settings_button, &QPushButton::clicked, this, [=]() {
		// open the settings dialog
		LLMSettingsDialog *settings_dialog = new LLMSettingsDialog(this);
		settings_dialog->show();
	});

	this->connect(this->generate_button, &QPushButton::clicked, this,
		      &LLMDockWidgetUI::generate);
	this->connect(this->clear_button, &QPushButton::clicked, this, &LLMDockWidgetUI::clear);
	this->connect(this, &LLMDockWidgetUI::update_text_signal, this,
		      &LLMDockWidgetUI::update_text);
}

LLMDockWidgetUI::~LLMDockWidgetUI() {}

void LLMDockWidgetUI::generate()
{
	QString input_text = this->input_text_edit->toPlainText();
	if (input_text.isEmpty()) {
		return;
	}

	this->text_edit->insertHtml(QString("<p style=\"color:#ffffff;\">%1</p><br/>").arg(input_text));
	this->text_edit->moveCursor(QTextCursor::End);
	this->input_text_edit->clear();

	// call LLM inference on a separate thread using a lambda function
	std::thread t([input_text, this]() {
		std::string generated_text = llama_inference(
			input_text.toStdString(), (struct llama_context *)this->llm_ctx,
			[this](const std::string &partial_generation) {
				emit update_text_signal(QString::fromStdString(partial_generation), true);
			});
		emit update_text_signal(QString("<br/>"), true);
		// generated_text = std::regex_replace(
		// 	generated_text, std::regex("[^a-zA-Z0-9\\.,\\?!\\s\\:\\'\\-]"), "");
		// generated_text = generated_text.substr(0, generated_text.find_first_of('\n'));

		// update the text edit with the generated text, call the update_text signal
		// emit update_text_signal(QString::fromStdString(generated_text), false);
	});
	t.detach();
}

void LLMDockWidgetUI::clear()
{
	this->text_edit->clear();
}

void LLMDockWidgetUI::update_text(const QString &text, bool partial_generation)
{
	if (partial_generation) {
		if (text.isEmpty()) {
			return;
		}
		// if text is just a new line, append a <br> tag
		if (text == "\n") {
			this->text_edit->insertHtml("<br>");
			return;
		}
		// replace spaces with non-breaking spaces
		QString text_with_non_breaking_spaces = text;
		text_with_non_breaking_spaces.replace(" ", "&nbsp;");

		// append text in a different color
		this->text_edit->insertHtml(
			QString("<span style=\"color:#00ff00;\">%1</span>").arg(text_with_non_breaking_spaces));
	} else {
		this->text_edit->insertHtml(QString("<p style=\"color:#ffffff;\">%1</p>").arg(text));
	}
	// always scroll to the bottom
	this->text_edit->moveCursor(QTextCursor::End);
}
