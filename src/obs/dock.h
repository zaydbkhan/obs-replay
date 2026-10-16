#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QComboBox>

// Dock / UI
/**
 * OBS dock UI panel for controlling replay and editor playback.
 */

class ReplayDock : public QWidget {
	Q_OBJECT

public:
	ReplayDock(QWidget *parent = nullptr);
	void addReplayToList(const std::string& displayName);
    std::string getSelectedTargetSource() const;

public slots:
	// Slot that triggers when the Refresh button is clicked
	void refreshSources();

private:
	QComboBox *targetSelector;
	QListWidget *replayList;

private slots:
	void onExportClicked();
	// Lookup replay_name in your ReplayManager and trigger FFmpeg export
};
