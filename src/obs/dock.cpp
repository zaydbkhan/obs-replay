#include <obs.h>
#include "dock.h"
#include "preview-source.h"

#include <QLabel>
#include <QVBoxLayout>

/**
 * OBS dock UI panel for controlling replay and editor playback.
 */

void update_ingest_target(const std::string &);

static bool EnumSourcesCallback(void *data, obs_source_t *source)
{
	auto *dropdown = static_cast<QComboBox *>(data);

	// Check what kind of source this is
	uint32_t flags = obs_source_get_output_flags(source);

	// We only want to list Video and Audio sources (ignore Scene folders, etc.)
	if ((flags & OBS_SOURCE_VIDEO) || (flags & OBS_SOURCE_AUDIO)) {
		const char *name = obs_source_get_name(source);
		dropdown->addItem(QString::fromUtf8(name));
	}

	return true; // Return true to tell OBS to keep iterating
}

ReplayDock::ReplayDock(QWidget *parent) : QWidget(parent)
{

	// 2. Apply the layout directly to 'this' instead of a container
	QVBoxLayout *layout = new QVBoxLayout(this);

	// --- 1. Target Selector (Dropdown) ---
	layout->addWidget(new QLabel("Target Source for Replays:"));
	targetSelector = new QComboBox();
	layout->addWidget(targetSelector);

	// --- 2. Refresh Button ---
	QPushButton *refreshBtn = new QPushButton("Refresh Source List");
	layout->addWidget(refreshBtn);

	connect(refreshBtn, &QPushButton::clicked, this, &ReplayDock::refreshSources);

	// --- 3. Recorded Replays List ---
	layout->addSpacing(15);
	layout->addWidget(new QLabel("Recorded Replays:"));
	replayList = new QListWidget();
	layout->addWidget(replayList);

	connect(replayList, &QListWidget::itemDoubleClicked, this, [](QListWidgetItem *item) {
		std::string replay_name = item->text().toStdString();
		play_replay_in_current_scene(replay_name);
	});

	connect(targetSelector, &QComboBox::currentTextChanged, this, [](const QString &text) {
		std::string new_source = text.toStdString();
		update_ingest_target(new_source);
	});

	refreshSources();
}

void ReplayDock::addReplayToList(const std::string &displayName)
{
	replayList->addItem(QString::fromStdString(displayName));
}

void ReplayDock::refreshSources()
{
	targetSelector->clear(); // Empty the current dropdown

	// Ask OBS to loop through all sources and run our callback,
	// passing our combobox as the data payload.
	obs_enum_sources(EnumSourcesCallback, targetSelector);
}

std::string ReplayDock::getSelectedTargetSource() const
{
	// Return the text of whatever the user currently has selected
	return targetSelector->currentText().toStdString();
}

void ReplayDock::onExportClicked()
{
	QListWidgetItem *selected = replayList->currentItem();
	if (!selected)
		return;

	std::string replay_name = selected->text().toStdString();
	// Lookup replay_name in your ReplayManager and trigger FFmpeg export
}
