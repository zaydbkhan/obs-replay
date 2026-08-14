#include "replay-dock.h"

#include "replay-control.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QEvent>
#include <QGridLayout>
#include <QHideEvent>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace {

constexpr auto REPLAY_DOCK_ID = "ReplaySourceControlsDock";
constexpr int TIMELINE_STEPS = 1000;

QString moduleText(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

struct ReplaySourceEntry {
	QString name;
	QString uuid;
};

class ReplayDock final : public QWidget {
public:
	explicit ReplayDock(QWidget *parent = nullptr) : QWidget(parent)
	{
		auto *mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(6, 6, 6, 6);
		mainLayout->setSpacing(6);

		auto *sourceLayout = new QHBoxLayout;
		sourceLayout->addWidget(new QLabel(moduleText("DockReplaySource"), this));
		sourceSelector = new QComboBox(this);
		sourceSelector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		sourceLayout->addWidget(sourceSelector, 1);
		propertiesButton = new QPushButton(moduleText("DockProperties"), this);
		sourceLayout->addWidget(propertiesButton);
		mainLayout->addLayout(sourceLayout);

		statusLabel = new QLabel(moduleText("DockNoReplaySource"), this);
		statusLabel->setAlignment(Qt::AlignCenter);
		mainLayout->addWidget(statusLabel);

		timeline = new QSlider(Qt::Horizontal, this);
		timeline->setRange(0, TIMELINE_STEPS);
		mainLayout->addWidget(timeline);

		timeLabel = new QLabel(formatTime(0) + " / " + formatTime(0), this);
		timeLabel->setAlignment(Qt::AlignCenter);
		mainLayout->addWidget(timeLabel);

		auto *captureLayout = new QHBoxLayout;
		makeActionButton(captureLayout, "LoadReplay", REPLAY_CONTROL_LOAD);
		makeActionButton(captureLayout, "SaveReplay", REPLAY_CONTROL_SAVE);
		makeActionButton(captureLayout, "Remove", REPLAY_CONTROL_REMOVE);
		makeActionButton(captureLayout, "Clear", REPLAY_CONTROL_CLEAR);
		mainLayout->addLayout(captureLayout);

		auto *transportLayout = new QHBoxLayout;
		makeActionButton(transportLayout, "First", REPLAY_CONTROL_FIRST);
		makeActionButton(transportLayout, "Previous", REPLAY_CONTROL_PREVIOUS);
		playPauseButton = makeActionButton(transportLayout, "DockPlay", REPLAY_CONTROL_PLAY_PAUSE);
		makeActionButton(transportLayout, "Next", REPLAY_CONTROL_NEXT);
		makeActionButton(transportLayout, "Last", REPLAY_CONTROL_LAST);
		mainLayout->addLayout(transportLayout);

		auto *speedLayout = new QHBoxLayout;
		makeActionButton(speedLayout, "Slower", REPLAY_CONTROL_SLOWER);
		makeActionButton(speedLayout, "HalfSpeed", REPLAY_CONTROL_HALF_SPEED);
		makeActionButton(speedLayout, "NormalSpeed", REPLAY_CONTROL_NORMAL_SPEED);
		makeActionButton(speedLayout, "DoubleSpeed", REPLAY_CONTROL_DOUBLE_SPEED);
		makeActionButton(speedLayout, "Faster", REPLAY_CONTROL_FASTER);
		mainLayout->addLayout(speedLayout);

		auto *frameLayout = new QHBoxLayout;
		makeActionButton(frameLayout, "PrevNFrames", REPLAY_CONTROL_PREVIOUS_N_FRAMES);
		makeActionButton(frameLayout, "PrevFrame", REPLAY_CONTROL_PREVIOUS_FRAME);
		makeActionButton(frameLayout, "NextFrame", REPLAY_CONTROL_NEXT_FRAME);
		makeActionButton(frameLayout, "NextNFrames", REPLAY_CONTROL_NEXT_N_FRAMES);
		mainLayout->addLayout(frameLayout);

		auto *editLayout = new QHBoxLayout;
		makeActionButton(editLayout, "TrimFront", REPLAY_CONTROL_TRIM_FRONT);
		makeActionButton(editLayout, "TrimReset", REPLAY_CONTROL_TRIM_RESET);
		makeActionButton(editLayout, "TrimEnd", REPLAY_CONTROL_TRIM_END);
		mainLayout->addLayout(editLayout);

		auto *modeLayout = new QHBoxLayout;
		directionButton = new QPushButton(moduleText("Backward"), this);
		modeLayout->addWidget(directionButton);
		actionControls.push_back(directionButton);
		connect(directionButton, &QPushButton::clicked, this,
			[this] { executeAction(lastBackward ? REPLAY_CONTROL_FORWARD : REPLAY_CONTROL_BACKWARD); });
		captureButton = new QPushButton(moduleText("Disable"), this);
		modeLayout->addWidget(captureButton);
		actionControls.push_back(captureButton);
		connect(captureButton, &QPushButton::clicked, this,
			[this] { executeAction(lastCaptureEnabled ? REPLAY_CONTROL_DISABLE : REPLAY_CONTROL_ENABLE); });
		makeActionButton(modeLayout, "Restart", REPLAY_CONTROL_RESTART);
		makeActionButton(modeLayout, "DockStop", REPLAY_CONTROL_STOP);
		mainLayout->addLayout(modeLayout);

		connect(sourceSelector, &QComboBox::currentIndexChanged, this, [this] { updateSnapshot(); });
		connect(propertiesButton, &QPushButton::clicked, this, [this] { openProperties(); });
		connect(timeline, &QSlider::sliderMoved, this, [this](int value) {
			const int64_t position = lastDurationMs * value / TIMELINE_STEPS;
			timeLabel->setText(formatTime(position) + " / " + formatTime(lastDurationMs));
		});
		connect(timeline, &QSlider::sliderReleased, this, [this] { seekToSlider(); });

		sourceTimer = new QTimer(this);
		sourceTimer->setInterval(1000);
		connect(sourceTimer, &QTimer::timeout, this, [this] { refreshSources(); });

		stateTimer = new QTimer(this);
		stateTimer->setInterval(100);
		connect(stateTimer, &QTimer::timeout, this, [this] { updateSnapshot(); });

		refreshSources();
		startTimers();
	}

protected:
	bool event(QEvent *event) override
	{
		const auto dockCloseEvent = static_cast<QEvent::Type>(QEvent::User + QEvent::Close);
		if (event->type() == dockCloseEvent)
			stopTimers();
		return QWidget::event(event);
	}

	void showEvent(QShowEvent *event) override
	{
		QWidget::showEvent(event);
		refreshSources();
		startTimers();
	}

	void hideEvent(QHideEvent *event) override
	{
		stopTimers();
		QWidget::hideEvent(event);
	}

private:
	static bool enumerateReplaySources(void *data, obs_source_t *source)
	{
		if (!replay_control_is_source(source))
			return true;

		const char *name = obs_source_get_name(source);
		const char *uuid = obs_source_get_uuid(source);
		if (!name || !uuid)
			return true;

		auto *entries = static_cast<std::vector<ReplaySourceEntry> *>(data);
		entries->push_back({QString::fromUtf8(name), QString::fromUtf8(uuid)});
		return true;
	}

	QPushButton *makeActionButton(QBoxLayout *layout, const char *textKey, enum replay_control_action action)
	{
		auto *button = new QPushButton(moduleText(textKey), this);
		layout->addWidget(button);
		actionControls.push_back(button);
		connect(button, &QPushButton::clicked, this, [this, action] { executeAction(action); });
		return button;
	}

	QString selectedUuid() const { return sourceSelector->currentData().toString(); }

	obs_source_t *currentSource() const
	{
		const QByteArray uuid = selectedUuid().toUtf8();
		return uuid.isEmpty() ? nullptr : obs_get_source_by_uuid(uuid.constData());
	}

	void refreshSources()
	{
		std::vector<ReplaySourceEntry> entries;
		obs_enum_sources(enumerateReplaySources, &entries);
		std::sort(entries.begin(), entries.end(),
			  [](const ReplaySourceEntry &left, const ReplaySourceEntry &right) {
				  return QString::localeAwareCompare(left.name, right.name) < 0;
			  });

		bool unchanged = sourceSelector->count() == static_cast<int>(entries.size());
		for (int index = 0; unchanged && index < sourceSelector->count(); ++index) {
			unchanged = sourceSelector->itemText(index) == entries[static_cast<size_t>(index)].name &&
				    sourceSelector->itemData(index).toString() ==
					    entries[static_cast<size_t>(index)].uuid;
		}
		if (unchanged)
			return;

		const QString previousUuid = selectedUuid();
		const QSignalBlocker blocker(sourceSelector);
		sourceSelector->clear();
		for (const ReplaySourceEntry &entry : entries)
			sourceSelector->addItem(entry.name, entry.uuid);

		const int previousIndex = sourceSelector->findData(previousUuid);
		if (previousIndex >= 0)
			sourceSelector->setCurrentIndex(previousIndex);
		else if (!entries.empty())
			sourceSelector->setCurrentIndex(0);

		updateSnapshot();
	}

	void executeAction(enum replay_control_action action)
	{
		obs_source_t *source = currentSource();
		if (!source)
			return;

		replay_control_execute(source, action);
		obs_source_release(source);
		updateSnapshot();
	}

	void openProperties()
	{
		obs_source_t *source = currentSource();
		if (!source)
			return;

		obs_frontend_open_source_properties(source);
		obs_source_release(source);
	}

	void seekToSlider()
	{
		obs_source_t *source = currentSource();
		if (!source)
			return;

		const int64_t position = lastDurationMs * timeline->value() / TIMELINE_STEPS;
		replay_control_set_time(source, position);
		obs_source_release(source);
		updateSnapshot();
	}

	void updateSnapshot()
	{
		obs_source_t *source = currentSource();
		if (!source) {
			setControlsEnabled(false);
			lastDurationMs = 0;
			timeline->setValue(0);
			timeLabel->setText(formatTime(0) + " / " + formatTime(0));
			statusLabel->setText(moduleText("DockNoReplaySource"));
			return;
		}

		struct replay_control_snapshot snapshot;
		const bool available = replay_control_get_snapshot(source, &snapshot);
		obs_source_release(source);
		if (!available) {
			setControlsEnabled(false);
			return;
		}

		setControlsEnabled(true);
		lastDurationMs = std::max<int64_t>(0, snapshot.duration_ms);
		const int64_t positionMs = std::clamp<int64_t>(snapshot.position_ms, 0, lastDurationMs);
		if (!timeline->isSliderDown()) {
			const int position =
				lastDurationMs > 0 ? static_cast<int>(positionMs * TIMELINE_STEPS / lastDurationMs) : 0;
			timeline->setValue(position);
			timeLabel->setText(formatTime(positionMs) + " / " + formatTime(lastDurationMs));
		}

		const QString state = stateText(snapshot.state);
		QString status = moduleText("DockStatusFormat")
					 .arg(snapshot.replay_index)
					 .arg(snapshot.replay_count)
					 .arg(snapshot.speed_percent, 0, 'f', 0)
					 .arg(state);
		if (snapshot.saving)
			status += " · " + moduleText("DockSaving");
		statusLabel->setText(status);

		playPauseButton->setText(moduleText(snapshot.state == OBS_MEDIA_STATE_PLAYING ? "Pause" : "DockPlay"));
		lastBackward = snapshot.backward;
		lastCaptureEnabled = snapshot.capture_enabled;
		captureButton->setText(moduleText(snapshot.capture_enabled ? "Disable" : "Enable"));
		directionButton->setText(moduleText(snapshot.backward ? "Forward" : "Backward"));
	}

	void setControlsEnabled(bool enabled)
	{
		propertiesButton->setEnabled(enabled);
		timeline->setEnabled(enabled);
		for (QWidget *control : actionControls)
			control->setEnabled(enabled);
	}

	void startTimers()
	{
		sourceTimer->start();
		stateTimer->start();
	}

	void stopTimers()
	{
		sourceTimer->stop();
		stateTimer->stop();
	}

	static QString formatTime(int64_t milliseconds)
	{
		const int64_t totalSeconds = std::max<int64_t>(0, milliseconds) / 1000;
		const int64_t hours = totalSeconds / 3600;
		const int64_t minutes = totalSeconds / 60 % 60;
		const int64_t seconds = totalSeconds % 60;
		if (hours > 0)
			return QStringLiteral("%1:%2:%3")
				.arg(hours)
				.arg(minutes, 2, 10, QChar('0'))
				.arg(seconds, 2, 10, QChar('0'));
		return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
	}

	static QString stateText(enum obs_media_state state)
	{
		switch (state) {
		case OBS_MEDIA_STATE_PLAYING:
			return moduleText("DockPlaying");
		case OBS_MEDIA_STATE_OPENING:
		case OBS_MEDIA_STATE_BUFFERING:
			return moduleText("DockLoading");
		case OBS_MEDIA_STATE_PAUSED:
			return moduleText("DockPaused");
		case OBS_MEDIA_STATE_ENDED:
			return moduleText("DockEnded");
		case OBS_MEDIA_STATE_ERROR:
			return moduleText("DockError");
		case OBS_MEDIA_STATE_NONE:
		case OBS_MEDIA_STATE_STOPPED:
		default:
			return moduleText("DockStopped");
		}
	}

	QComboBox *sourceSelector = nullptr;
	QLabel *statusLabel = nullptr;
	QLabel *timeLabel = nullptr;
	QSlider *timeline = nullptr;
	QPushButton *propertiesButton = nullptr;
	QPushButton *playPauseButton = nullptr;
	QPushButton *captureButton = nullptr;
	QPushButton *directionButton = nullptr;
	QTimer *sourceTimer = nullptr;
	QTimer *stateTimer = nullptr;
	std::vector<QWidget *> actionControls;
	int64_t lastDurationMs = 0;
	bool lastBackward = false;
	bool lastCaptureEnabled = true;
};

QPointer<ReplayDock> replayDock;

} // namespace

bool replay_dock_create(void)
{
	if (replayDock)
		return true;

	auto *dock = new ReplayDock;
	if (!obs_frontend_add_dock_by_id(REPLAY_DOCK_ID, obs_module_text("ReplayControlsDock"), dock)) {
		delete dock;
		return false;
	}

	replayDock = dock;
	return true;
}

void replay_dock_destroy(void)
{
	if (!replayDock)
		return;

	ReplayDock *dock = replayDock.data();
	obs_frontend_remove_dock(REPLAY_DOCK_ID);
	if (replayDock)
		delete dock;
	replayDock.clear();
}
