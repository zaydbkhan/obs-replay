#include "replay-dock.h"

#include "replay-control.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QAbstractItemView>
#include <QBoxLayout>
#include <QComboBox>
#include <QDockWidget>
#include <QEvent>
#include <QHeaderView>
#include <QHideEvent>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QString>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include <algorithm>
#include <cstdint>
#include <functional>
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
		setObjectName(QStringLiteral("replayDock"));
		setMinimumSize(640, 300);
		setStyleSheet(QStringLiteral(R"(
			QWidget#replayDock {
				font-size: 12px;
			}
			QWidget#replayDock QPushButton {
				background-color: #3b4050;
				border: 1px solid #3b4050;
				border-radius: 4px;
				color: #ffffff;
				min-height: 30px;
				padding: 0 12px;
			}
			QWidget#replayDock QPushButton:hover {
				background-color: #4a5062;
			}
			QWidget#replayDock QPushButton:pressed {
				background-color: #2f3441;
			}
			QWidget#replayDock QPushButton:disabled {
				color: #8d929e;
				background-color: #303541;
				border-color: #303541;
			}
			QWidget#replayDock QPushButton#liveButton {
				background-color: #c8322a;
				border-color: #c8322a;
				font-weight: 600;
			}
			QWidget#replayDock QPushButton#liveButton:hover {
				background-color: #dd4037;
			}
			QWidget#replayDock QPushButton#markInButton {
				border-bottom: 2px solid #22a767;
			}
			QWidget#replayDock QPushButton#markOutButton {
				border-bottom: 2px solid #d93d4a;
			}
			QWidget#replayDock QComboBox {
				min-height: 30px;
				padding-left: 8px;
			}
			QWidget#replayDock QTableWidget {
				background-color: #232731;
				alternate-background-color: #282d38;
				border: 1px solid #242a35;
				border-radius: 4px;
				color: #ffffff;
				gridline-color: #343a46;
				selection-background-color: #315a91;
			}
			QWidget#replayDock QHeaderView::section {
				background-color: #17457d;
				border: 0;
				border-right: 1px solid #2f6ba9;
				color: #ffffff;
				padding: 7px 3px;
				text-align: left;
			}
			QWidget#replayDock QSlider::groove:horizontal {
				background: #3c4351;
				height: 4px;
			}
			QWidget#replayDock QSlider::sub-page:horizontal {
				background: #6f7c91;
			}
			QWidget#replayDock QSlider::handle:horizontal {
				background: #ffffff;
				border-radius: 5px;
				height: 10px;
				margin: -3px 0;
				width: 20px;
			}
		)"));

		auto *mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(10, 8, 10, 8);
		mainLayout->setSpacing(7);

		auto *sourceLayout = new QHBoxLayout;
		sourceLayout->setSpacing(6);
		sourceLayout->addWidget(new QLabel(moduleText("DockReplaySource"), this));
		sourceSelector = new QComboBox(this);
		sourceSelector->setMinimumWidth(220);
		sourceSelector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		sourceLayout->addWidget(sourceSelector, 1);
		statusLabel = new QLabel(moduleText("DockNoReplaySourceShort"), this);
		statusLabel->setMinimumWidth(145);
		sourceLayout->addWidget(statusLabel);

		captureButton = makeButton(sourceLayout, moduleText("DockRec"), [this] {
			executeAction(lastCaptureEnabled ? REPLAY_CONTROL_DISABLE : REPLAY_CONTROL_ENABLE);
		});
		captureButton->setToolTip(moduleText("DockRecTooltip"));
		liveButton = makeActionButton(sourceLayout, moduleText("DockLive"), REPLAY_CONTROL_LOAD);
		liveButton->setObjectName(QStringLiteral("liveButton"));
		liveButton->setToolTip(moduleText("DockLiveTooltip"));
		mainLayout->addLayout(sourceLayout);

		replayTable = new QTableWidget(0, 6, this);
		replayTable->setHorizontalHeaderLabels({moduleText("DockTableId"), moduleText("DockTableName"),
						      moduleText("DockTableIn"), moduleText("DockTableOut"),
						      moduleText("DockTableDuration"), moduleText("DockTableSpeed")});
		for (int column = 0; column < replayTable->columnCount(); column++)
			replayTable->horizontalHeaderItem(column)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		replayTable->verticalHeader()->hide();
		replayTable->horizontalHeader()->setMinimumSectionSize(42);
		replayTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
		replayTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		for (int column = 2; column < replayTable->columnCount(); column++)
			replayTable->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
		replayTable->setAlternatingRowColors(true);
		replayTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		replayTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		replayTable->setSelectionMode(QAbstractItemView::SingleSelection);
		replayTable->setShowGrid(false);
		replayTable->setMinimumHeight(124);
		mainLayout->addWidget(replayTable, 1);

		auto *markLayout = new QHBoxLayout;
		markLayout->setSpacing(5);
		markLayout->addWidget(new QLabel(moduleText("DockMark"), this));
		auto *markIn = makeActionButton(markLayout, moduleText("DockMarkIn"), REPLAY_CONTROL_TRIM_FRONT);
		markIn->setObjectName(QStringLiteral("markInButton"));
		auto *markOut = makeActionButton(markLayout, moduleText("DockMarkOut"), REPLAY_CONTROL_TRIM_END);
		markOut->setObjectName(QStringLiteral("markOutButton"));
		makeSeekButton(markLayout, QStringLiteral("-5"), -5000);
		makeSeekButton(markLayout, QStringLiteral("-10"), -10000);
		makeSeekButton(markLayout, QStringLiteral("-20"), -20000);
		markLayout->addStretch(1);
		makeActionButton(markLayout, moduleText("DockMoveUp"), REPLAY_CONTROL_PREVIOUS);
		makeActionButton(markLayout, moduleText("DockMoveDown"), REPLAY_CONTROL_NEXT);
		makeActionButton(markLayout, moduleText("DockDelete"), REPLAY_CONTROL_REMOVE);
		mainLayout->addLayout(markLayout);

		auto *transportLayout = new QHBoxLayout;
		transportLayout->setSpacing(5);
		makeActionButton(transportLayout, moduleText("DockPreviousFrame"), REPLAY_CONTROL_PREVIOUS_FRAME);
		playPauseButton = makeActionButton(transportLayout, moduleText("DockPlay"), REPLAY_CONTROL_PLAY_PAUSE);
		makeActionButton(transportLayout, moduleText("DockNextFrame"), REPLAY_CONTROL_NEXT_FRAME);
		makeActionButton(transportLayout, moduleText("DockStop"), REPLAY_CONTROL_STOP);
		transportLayout->addSpacing(5);
		auto *playEvents =
			makeActionButton(transportLayout, moduleText("DockPlayEvents"), REPLAY_CONTROL_RESTART);
		playEvents->setMinimumWidth(92);
		makeSeekButton(transportLayout, QStringLiteral("5↶"), -5000);
		makeSeekButton(transportLayout, QStringLiteral("0↶"), -1000);
		makeSeekButton(transportLayout, QStringLiteral("5↷"), 5000);
		makeSeekButton(transportLayout, QStringLiteral("30↷"), 30000);
		makeSeekButton(transportLayout, QStringLiteral("60↷"), 60000);
		mainLayout->addLayout(transportLayout);

		auto *timeLayout = new QHBoxLayout;
		timeLabel = new QLabel(formatTime(0) + QStringLiteral(" / ") + formatTime(0), this);
		timeLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));
		timeLayout->addWidget(timeLabel);
		timeLayout->addStretch(1);
		speedLabel = new QLabel(QStringLiteral("100%"), this);
		timeLayout->addWidget(speedLabel);
		mainLayout->addLayout(timeLayout);

		timeline = new QSlider(Qt::Horizontal, this);
		timeline->setRange(0, TIMELINE_STEPS);
		timeline->setTickPosition(QSlider::TicksBelow);
		timeline->setTickInterval(50);
		mainLayout->addWidget(timeline);

		connect(sourceSelector, &QComboBox::currentIndexChanged, this, [this] { updateSnapshot(); });
		connect(replayTable, &QTableWidget::cellClicked, this, [this](int row, int) { selectReplay(row); });
		connect(timeline, &QSlider::sliderMoved, this, [this](int value) {
			const int64_t position = lastDurationMs * value / TIMELINE_STEPS;
			timeLabel->setText(formatTime(position) + QStringLiteral(" / ") + formatTime(lastDurationMs));
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

	QPushButton *makeButton(QBoxLayout *layout, const QString &text, const std::function<void()> &callback)
	{
		auto *button = new QPushButton(text, this);
		layout->addWidget(button);
		actionControls.push_back(button);
		connect(button, &QPushButton::clicked, this, callback);
		return button;
	}

	QPushButton *makeActionButton(QBoxLayout *layout, const QString &text, enum replay_control_action action)
	{
		return makeButton(layout, text, [this, action] { executeAction(action); });
	}

	QPushButton *makeSeekButton(QBoxLayout *layout, const QString &text, int64_t offsetMs)
	{
		auto *button = makeButton(layout, text, [this, offsetMs] { seekRelative(offsetMs); });
		button->setMinimumWidth(48);
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

		bool unchanged = false;
		if (entries.empty()) {
			unchanged = sourceSelector->count() == 1 && sourceSelector->itemData(0).toString().isEmpty();
		} else if (sourceSelector->count() == static_cast<int>(entries.size())) {
			unchanged = true;
			for (int index = 0; unchanged && index < sourceSelector->count(); ++index) {
				unchanged = sourceSelector->itemText(index) == entries[static_cast<size_t>(index)].name &&
					    sourceSelector->itemData(index).toString() ==
						    entries[static_cast<size_t>(index)].uuid;
			}
		}
		if (unchanged)
			return;

		const QString previousUuid = selectedUuid();
		const QSignalBlocker blocker(sourceSelector);
		sourceSelector->clear();
		if (entries.empty()) {
			sourceSelector->addItem(moduleText("DockNoReplaySourceShort"), QString());
		} else {
			for (const ReplaySourceEntry &entry : entries)
				sourceSelector->addItem(entry.name, entry.uuid);

			const int previousIndex = sourceSelector->findData(previousUuid);
			sourceSelector->setCurrentIndex(previousIndex >= 0 ? previousIndex : 0);
		}

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

	void selectReplay(int row)
	{
		obs_source_t *source = currentSource();
		if (!source)
			return;

		replay_control_select_item(source, row);
		obs_source_release(source);
		updateSnapshot();
	}

	void seekRelative(int64_t offsetMs)
	{
		obs_source_t *source = currentSource();
		if (!source)
			return;

		const int64_t position = obs_source_media_get_time(source);
		replay_control_set_time(source, position + offsetMs);
		obs_source_release(source);
		updateSnapshot();
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

	void refreshReplayTable(obs_source_t *source, const struct replay_control_snapshot &snapshot)
	{
		const size_t count = replay_control_get_items(source, nullptr, 0);
		std::vector<struct replay_control_item> items(count);
		if (count)
			replay_control_get_items(source, items.data(), items.size());

		const QSignalBlocker blocker(replayTable);
		replayTable->setRowCount(static_cast<int>(items.size()));
		for (int row = 0; row < static_cast<int>(items.size()); row++) {
			const struct replay_control_item &item = items[static_cast<size_t>(row)];
			setTableText(row, 0, QString::number(item.id), Qt::AlignCenter);
			setTableText(row, 1, moduleText("DockReplayRowName").arg(item.id), Qt::AlignLeft | Qt::AlignVCenter);
			setTableText(row, 2, formatTableTime(item.in_ms), Qt::AlignCenter);
			setTableText(row, 3, formatTableTime(item.out_ms), Qt::AlignCenter);
			setTableText(row, 4, formatTableTime(item.duration_ms), Qt::AlignCenter);
			setTableText(row, 5, QStringLiteral("%1%").arg(item.speed_percent, 0, 'f', 0), Qt::AlignCenter);
			replayTable->setRowHeight(row, 24);
		}

		if (snapshot.replay_index > 0 && snapshot.replay_index <= replayTable->rowCount())
			replayTable->selectRow(snapshot.replay_index - 1);
		else
			replayTable->clearSelection();
	}

	void setTableText(int row, int column, const QString &text, Qt::Alignment alignment)
	{
		QTableWidgetItem *item = replayTable->item(row, column);
		if (!item) {
			item = new QTableWidgetItem;
			replayTable->setItem(row, column, item);
		}
		item->setText(text);
		item->setTextAlignment(alignment);
	}

	void updateSnapshot()
	{
		obs_source_t *source = currentSource();
		if (!source) {
			setControlsEnabled(false);
			lastDurationMs = 0;
			replayTable->setRowCount(0);
			timeline->setValue(0);
			timeLabel->setText(formatTime(0) + QStringLiteral(" / ") + formatTime(0));
			speedLabel->setText(QStringLiteral("100%"));
			statusLabel->setText(moduleText("DockNoReplaySourceShort"));
			return;
		}

		struct replay_control_snapshot snapshot;
		const bool available = replay_control_get_snapshot(source, &snapshot);
		if (!available) {
			obs_source_release(source);
			setControlsEnabled(false);
			return;
		}

		setControlsEnabled(true);
		refreshReplayTable(source, snapshot);
		obs_source_release(source);

		lastDurationMs = std::max<int64_t>(0, snapshot.duration_ms);
		const int64_t positionMs = std::clamp<int64_t>(snapshot.position_ms, 0, lastDurationMs);
		if (!timeline->isSliderDown()) {
			const int position =
				lastDurationMs > 0 ? static_cast<int>(positionMs * TIMELINE_STEPS / lastDurationMs) : 0;
			timeline->setValue(position);
			timeLabel->setText(formatTime(positionMs) + QStringLiteral(" / ") + formatTime(lastDurationMs));
		}

		statusLabel->setText(stateText(snapshot.state));
		const float signedSpeed = snapshot.backward ? -snapshot.speed_percent : snapshot.speed_percent;
		speedLabel->setText(QStringLiteral("%1%").arg(signedSpeed, 0, 'f', 0));
		playPauseButton->setText(moduleText(snapshot.state == OBS_MEDIA_STATE_PLAYING ? "Pause" : "DockPlay"));
		lastCaptureEnabled = snapshot.capture_enabled;
		captureButton->setProperty("captureActive", snapshot.capture_enabled);
		captureButton->style()->unpolish(captureButton);
		captureButton->style()->polish(captureButton);
	}

	void setControlsEnabled(bool enabled)
	{
		timeline->setEnabled(enabled);
		replayTable->setEnabled(enabled);
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
		const int64_t total = std::max<int64_t>(0, milliseconds);
		const int64_t hours = total / 3600000;
		const int64_t minutes = total / 60000 % 60;
		const int64_t seconds = total / 1000 % 60;
		const int64_t millis = total % 1000;
		return QStringLiteral("%1:%2:%3.%4")
			.arg(hours, 2, 10, QChar('0'))
			.arg(minutes, 2, 10, QChar('0'))
			.arg(seconds, 2, 10, QChar('0'))
			.arg(millis, 3, 10, QChar('0'));
	}

	static QString formatTableTime(int64_t milliseconds)
	{
		const int64_t total = std::max<int64_t>(0, milliseconds);
		const int64_t minutes = total / 60000;
		const int64_t seconds = total / 1000 % 60;
		const int64_t millis = total % 1000;
		return QStringLiteral("%1:%2.%3")
			.arg(minutes, 2, 10, QChar('0'))
			.arg(seconds, 2, 10, QChar('0'))
			.arg(millis, 3, 10, QChar('0'));
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
	QLabel *speedLabel = nullptr;
	QTableWidget *replayTable = nullptr;
	QSlider *timeline = nullptr;
	QPushButton *playPauseButton = nullptr;
	QPushButton *captureButton = nullptr;
	QPushButton *liveButton = nullptr;
	QTimer *sourceTimer = nullptr;
	QTimer *stateTimer = nullptr;
	std::vector<QWidget *> actionControls;
	int64_t lastDurationMs = 0;
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
	QTimer::singleShot(1000, dock, [dock] {
			dock->show();
			QWidget *container = dock->parentWidget();
			while (container && !qobject_cast<QDockWidget *>(container))
				container = container->parentWidget();
			if (auto *dockWidget = qobject_cast<QDockWidget *>(container)) {
				auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
				if (mainWindow) {
					mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
					dockWidget->setFloating(false);
				}
				dockWidget->show();
				dockWidget->raise();
			}
		});
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
