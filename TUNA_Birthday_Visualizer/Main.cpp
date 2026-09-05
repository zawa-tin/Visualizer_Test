# include <Siv3D.hpp> // Siv3D v0.6.16

// Siv3D v0.6.16 向けの実装を、OpenSiv3D v0.6.6 Web SDK で互換ビルドする。
namespace AppConfig
{
	constexpr int32 CellSize = 50;
	constexpr int32 GridOffsetX = 20;
	constexpr int32 GridOffsetY = 20;
	constexpr int32 SidePanelMargin = 20;
	constexpr int32 SidePanelWidth = 400;
	constexpr int32 WindowWidth = 1200;
	constexpr int32 WindowHeight = 900;
}

enum class MoveDirection
{
	Left,
	Down,
	Right,
	Up,
};

Optional<MoveDirection> ToMoveDirection(const char32 command)
{
	switch (command)
	{
	case U'L':
		return MoveDirection::Left;
	case U'D':
		return MoveDirection::Down;
	case U'R':
		return MoveDirection::Right;
	case U'U':
		return MoveDirection::Up;
	default:
		return none;
	}
}

Point DirectionDelta(const MoveDirection direction)
{
	switch (direction)
	{
	case MoveDirection::Left:
		return{ -1, 0 };
	case MoveDirection::Down:
		return{ 0, 1 };
	case MoveDirection::Right:
		return{ 1, 0 };
	case MoveDirection::Up:
		return{ 0, -1 };
	default:
		return{ 0, 0 };
	}
}

MoveDirection TurnRight(const MoveDirection direction)
{
	switch (direction)
	{
	case MoveDirection::Left:
		return MoveDirection::Up;
	case MoveDirection::Down:
		return MoveDirection::Left;
	case MoveDirection::Right:
		return MoveDirection::Down;
	case MoveDirection::Up:
		return MoveDirection::Right;
	default:
		return direction;
	}
}

MoveDirection TurnLeft(const MoveDirection direction)
{
	switch (direction)
	{
	case MoveDirection::Left:
		return MoveDirection::Down;
	case MoveDirection::Down:
		return MoveDirection::Right;
	case MoveDirection::Right:
		return MoveDirection::Up;
	case MoveDirection::Up:
		return MoveDirection::Left;
	default:
		return direction;
	}
}

class GameMap
{
private:
	const Array<String> m_rows = TextReader{ U"Grid.txt" }.readLines();

public:
	[[nodiscard]] int32 height() const
	{
		return static_cast<int32>(m_rows.size());
	}

	[[nodiscard]] int32 width() const
	{
		return m_rows.empty() ? 0 : static_cast<int32>(m_rows.front().size());
	}

	[[nodiscard]] const Array<String>& rows() const
	{
		return m_rows;
	}

	[[nodiscard]] bool isFloor(const Point& position) const
	{
		return InRange(position.y, 0, height() - 1)
			&& InRange(position.x, 0, width() - 1)
			&& (m_rows[position.y][position.x] == U'.');
	}

	[[nodiscard]] Vec2 cellCenter(const Point& position) const
	{
		return{
			AppConfig::GridOffsetX + AppConfig::CellSize * (position.x + 0.5),
			AppConfig::GridOffsetY + AppConfig::CellSize * (position.y + 0.5),
		};
	}

	[[nodiscard]] double right() const
	{
		return AppConfig::GridOffsetX + width() * AppConfig::CellSize;
	}

	void draw() const
	{
		for (int32 y = 0; y < height(); ++y)
		{
			for (int32 x = 0; x < width(); ++x)
			{
				Rect{
					AppConfig::GridOffsetX + x * AppConfig::CellSize,
					AppConfig::GridOffsetY + y * AppConfig::CellSize,
					AppConfig::CellSize,
				}
					.draw(m_rows[y][x] == U'.' ? Palette::White : Palette::Black)
					.drawFrame(5, Palette::Black);
			}
		}
	}
};

class Player
{
private:
	Texture m_graphic{ U"Player.png" };
	Point m_position{ 0, 0 };

public:
	[[nodiscard]] const Point& position() const
	{
		return m_position;
	}

	void setPosition(const Point& position)
	{
		m_position = position;
	}

	void tryMove(const MoveDirection direction, const GameMap& map)
	{
		const Point next = m_position + DirectionDelta(direction);
		if (map.isFloor(next))
		{
			m_position = next;
		}
	}

	void draw(const GameMap& map) const
	{
		drawAt(map.cellCenter(m_position));
	}

	void drawAt(const Vec2& position) const
	{
		m_graphic.scaled(0.1).drawAt(position);
	}
};

class Tuna
{
private:
	int32 m_id;
	Point m_position;
	Texture m_graphic;

	[[nodiscard]] Optional<Point> findEscapeDestination(
		const MoveDirection direction,
		const Point& playerPosition,
		const GameMap& map) const
	{
		Point destination = m_position + DirectionDelta(direction);
		const Point rightDelta = DirectionDelta(TurnRight(direction));
		const Point leftDelta = DirectionDelta(TurnLeft(direction));

		while (map.isFloor(destination))
		{
			const bool isFarEnough = (destination.manhattanDistanceFrom(playerPosition) >= 2);
			const bool canTurn = map.isFloor(destination + rightDelta)
				|| map.isFloor(destination + leftDelta);

			if (isFarEnough && canTurn)
			{
				return destination;
			}

			destination += DirectionDelta(direction);
		}

		return none;
	}

public:
	Tuna(const int32 id, const Point& position, const Texture& graphic)
		: m_id{ id }
		, m_position{ position }
		, m_graphic{ graphic } {}

	[[nodiscard]] int32 id() const
	{
		return m_id;
	}

	[[nodiscard]] const Point& position() const
	{
		return m_position;
	}

	void fleeFrom(const Point& playerPosition, const GameMap& map)
	{
		if ((m_position.x != playerPosition.x && m_position.y != playerPosition.y)
			|| (m_position.manhattanDistanceFrom(playerPosition) > 2)
			|| (m_position == playerPosition))
		{
			return;
		}

		MoveDirection front;
		if (m_position.x < playerPosition.x)
		{
			front = MoveDirection::Left;
		}
		else if (m_position.x > playerPosition.x)
		{
			front = MoveDirection::Right;
		}
		else if (m_position.y < playerPosition.y)
		{
			front = MoveDirection::Up;
		}
		else
		{
			front = MoveDirection::Down;
		}

		for (const MoveDirection direction : { front, TurnRight(front), TurnLeft(front) })
		{
			if (const Optional<Point> destination = findEscapeDestination(direction, playerPosition, map))
			{
				m_position = *destination;
				return;
			}
		}
	}

	void draw(const GameMap& map) const
	{
		drawAt(map.cellCenter(m_position));
	}

	void drawAt(const Vec2& position) const
	{
		m_graphic.scaled(0.1).drawAt(position);
	}
};

struct SimulationDiff
{
	Point playerPosition;
	Array<int32> spawnedTunaIds;
	Array<int32> caughtTunaIds;
};

class Simulation
{
private:
	static constexpr int32 TunaCount = 10;
	static constexpr double AnimationDurationSeconds = 0.1;

	struct Snapshot
	{
		Point playerPosition;
		Array<Tuna> tunas;
	};

	struct TunaState
	{
		int32 id;
		Point position;
	};

	struct RollbackState
	{
		Point playerPosition;
		Array<TunaState> tunas;
		DefaultRNG::State_t randomState;
		int32 nextTunaId;
		int32 moveCount;
		int32 caughtTunaCount;
	};

	GameMap m_map;
	Player m_player;
	Texture m_tunaGraphic{ U"Tuna.png" };
	DefaultRNG m_random;
	Array<Tuna> m_tunas;
	int32 m_nextTunaId = 1;
	int32 m_moveCount = 0;
	int32 m_caughtTunaCount = 0;
	Array<Snapshot> m_animationSnapshots;
	Stopwatch m_animationStopwatch;
	Array<RollbackState> m_rollbackStates;

	[[nodiscard]] Point randomTunaPosition()
	{
		Array<Point> candidates;
		for (int32 y = 0; y < m_map.height(); ++y)
		{
			for (int32 x = 0; x < m_map.width(); ++x)
			{
				const Point position{ x, y };
				if (m_map.isFloor(position) && position != m_player.position())
				{
					candidates.push_back(position);
				}
			}
		}

		return candidates.choice(m_random);
	}

	void replenishTunas(Array<int32>* spawnedTunaIds = nullptr)
	{
		while (static_cast<int32>(m_tunas.size()) < TunaCount)
		{
			const int32 id = m_nextTunaId++;
			m_tunas.emplace_back(id, randomTunaPosition(), m_tunaGraphic);
			if (spawnedTunaIds)
			{
				spawnedTunaIds->push_back(id);
			}
		}
	}

	void advance(const MoveDirection direction, SimulationDiff& diff)
	{
		++m_moveCount;
		m_player.tryMove(direction, m_map);

		m_tunas.remove_if([this, &diff](const Tuna& tuna)
		{
			if (tuna.position() == m_player.position())
			{
				++m_caughtTunaCount;
				diff.caughtTunaIds.push_back(tuna.id());
				return true;
			}
			return false;
		});

		for (Tuna& tuna : m_tunas)
		{
			tuna.fleeFrom(m_player.position(), m_map);
		}

		replenishTunas(&diff.spawnedTunaIds);
	}

	[[nodiscard]] Snapshot makeSnapshot() const
	{
		return{ m_player.position(), m_tunas };
	}

	[[nodiscard]] RollbackState makeRollbackState() const
	{
		RollbackState state{
			m_player.position(),
			{},
			m_random.serialize(),
			m_nextTunaId,
			m_moveCount,
			m_caughtTunaCount,
		};
		state.tunas.reserve(m_tunas.size());
		for (const Tuna& tuna : m_tunas)
		{
			state.tunas.push_back({ tuna.id(), tuna.position() });
		}
		return state;
	}

	[[nodiscard]] static const Tuna* FindTunaById(const Array<Tuna>& tunas, const int32 id)
	{
		for (const Tuna& tuna : tunas)
		{
			if (tuna.id() == id)
			{
				return &tuna;
			}
		}

		return nullptr;
	}

	void drawAnimation() const
	{
		const size_t segmentCount = (m_animationSnapshots.size() - 1);
		const double animationProgress = Clamp(
			m_animationStopwatch.sF() / AnimationDurationSeconds, 0.0, 1.0);
		const double snapshotProgress = (animationProgress * segmentCount);
		const size_t segmentIndex = Min(
			static_cast<size_t>(snapshotProgress), segmentCount - 1);
		const double segmentProgress = (snapshotProgress - segmentIndex);

		const Snapshot& from = m_animationSnapshots[segmentIndex];
		const Snapshot& to = m_animationSnapshots[segmentIndex + 1];

		for (const Tuna& tuna : from.tunas)
		{
			const Vec2 fromPosition = m_map.cellCenter(tuna.position());
			if (const Tuna* destination = FindTunaById(to.tunas, tuna.id()))
			{
				const Vec2 toPosition = m_map.cellCenter(destination->position());
				tuna.drawAt(fromPosition + (toPosition - fromPosition) * segmentProgress);
			}
			else
			{
				// 捕獲されるマグロは、プレイヤーが移動先へ到着するまで表示する。
				tuna.drawAt(fromPosition);
			}
		}

		const Vec2 playerFrom = m_map.cellCenter(from.playerPosition);
		const Vec2 playerTo = m_map.cellCenter(to.playerPosition);
		m_player.drawAt(playerFrom + (playerTo - playerFrom) * segmentProgress);
	}

public:
	Simulation()
	{
		replenishTunas();
	}

	void reset()
	{
		m_player.setPosition({ 0, 0 });
		m_tunas.clear();
		m_nextTunaId = 1;
		m_moveCount = 0;
		m_caughtTunaCount = 0;
		m_animationSnapshots.clear();
		m_animationStopwatch.reset();
		m_rollbackStates.clear();
		replenishTunas();
	}

	[[nodiscard]] const GameMap& map() const
	{
		return m_map;
	}

	[[nodiscard]] const Player& player() const
	{
		return m_player;
	}

	[[nodiscard]] const Array<Tuna>& tunas() const
	{
		return m_tunas;
	}

	[[nodiscard]] int32 moveCount() const
	{
		return m_moveCount;
	}

	[[nodiscard]] int32 caughtTunaCount() const
	{
		return m_caughtTunaCount;
	}

	[[nodiscard]] bool isAnimating() const
	{
		return (m_animationSnapshots.size() >= 2)
			&& (m_animationStopwatch.sF() < AnimationDurationSeconds);
	}

	bool rollback()
	{
		if (m_rollbackStates.empty())
		{
			return false;
		}

		const RollbackState& rollbackState = m_rollbackStates.back();
		m_player.setPosition(rollbackState.playerPosition);
		m_tunas.clear();
		m_tunas.reserve(rollbackState.tunas.size());
		for (const TunaState& tuna : rollbackState.tunas)
		{
			m_tunas.emplace_back(tuna.id, tuna.position, m_tunaGraphic);
		}

		m_random.deserialize(rollbackState.randomState);
		m_nextTunaId = rollbackState.nextTunaId;
		m_moveCount = rollbackState.moveCount;
		m_caughtTunaCount = rollbackState.caughtTunaCount;
		m_animationSnapshots.clear();
		m_animationStopwatch.reset();
		m_rollbackStates.pop_back();
		return true;
	}

	SimulationDiff step(const MoveDirection direction)
	{
		return run(Array<MoveDirection>{ direction });
	}

	SimulationDiff run(const Array<MoveDirection>& command)
	{
		SimulationDiff diff{ m_player.position() };
		if (command.empty())
		{
			return diff;
		}

		m_rollbackStates.push_back(makeRollbackState());
		m_animationSnapshots.clear();
		m_animationSnapshots.reserve(command.size() + 1);
		m_animationSnapshots.push_back(makeSnapshot());

		for (const MoveDirection direction : command)
		{
			advance(direction, diff);
			m_animationSnapshots.push_back(makeSnapshot());
		}

		diff.playerPosition = m_player.position();
		m_animationStopwatch.restart();
		return diff;
	}

	void draw() const
	{
		m_map.draw();
		if (isAnimating())
		{
			drawAnimation();
			return;
		}

		for (const Tuna& tuna : m_tunas)
		{
			tuna.draw(m_map);
		}
		m_player.draw(m_map);
	}
};

Optional<Array<MoveDirection>> ParseCommand(const String& command)
{
	Array<MoveDirection> result;
	result.reserve(command.size());

	for (const char32 character : command)
	{
		const Optional<MoveDirection> direction = ToMoveDirection(character);
		if (not direction)
		{
			return none;
		}
		result.push_back(*direction);
	}

	return result;
}

class CommandInputer
{
private:
	TextEditState m_text;
	Vec2 m_position;

public:
	explicit CommandInputer(const double sidePanelX)
		: m_position{ sidePanelX, AppConfig::GridOffsetY } {}

	void reset()
	{
		m_text.clear();
	}

	[[nodiscard]] bool isActive() const
	{
		return m_text.active;
	}

	Optional<String> update(const bool enabled)
	{
		SimpleGUI::TextBox(
			m_text,
			m_position,
			300,
			unspecified,
			enabled);
		const bool buttonPressed = SimpleGUI::Button(
			U"実行",
			Vec2{ m_position.x + 320, m_position.y + 2 },
			unspecified,
			enabled);
		if (buttonPressed || (enabled && m_text.enterKey))
		{
			String command = m_text.text;
			m_text.clear();
			return command;
		}
		return none;
	}
};

class InputLogger
{
private:
	String m_text;
	Array<size_t> m_rollbackTextSizes;
	Font m_font{ FontMethod::MSDF, 20 };
	RectF m_box;
	Vec2 m_copyButtonPosition;

public:
	explicit InputLogger(const double sidePanelX)
		: m_box{ sidePanelX, AppConfig::GridOffsetY + 60, AppConfig::SidePanelWidth, 220 }
		, m_copyButtonPosition{ sidePanelX, AppConfig::GridOffsetY + 290 } {}

	void addNewLine()
	{
		if (not m_text.empty() && m_text.back() != U'\n')
		{
			m_text += U'\n';
		}
	}

	void addMove(const char32 command)
	{
		if (ToMoveDirection(command))
		{
			m_text += command;
		}
	}

	void addCommand(const String& command)
	{
		addNewLine();
		m_text += command;
	}

	void saveRollbackState()
	{
		m_rollbackTextSizes.push_back(m_text.size());
	}

	void rollback()
	{
		if (not m_rollbackTextSizes.empty())
		{
			m_text.resize(m_rollbackTextSizes.back());
			m_rollbackTextSizes.pop_back();
		}
	}

	void reset()
	{
		m_text.clear();
		m_rollbackTextSizes.clear();
	}

	void draw() const
	{
		m_box.draw(Palette::White).drawFrame(0, 2, Palette::Black);
		m_font(m_text).draw(m_box, Palette::Black);
	}

	void updateCopyButton() const
	{
		if (SimpleGUI::Button(U"クリップボードにコピー", m_copyButtonPosition))
		{
			Clipboard::SetText(m_text);
		}
	}
};

class DiffLogger
{
private:
	static constexpr int32 MoveLimit = 10000;

	String m_visibleText;
	Array<String> m_rollbackTexts;
	Font m_font{ FontMethod::MSDF, 12 };
	RectF m_box;
	Vec2 m_copyButtonPosition;

	static void AppendIdLine(String& text, const Array<int32>& ids)
	{
		for (size_t i = 0; i < ids.size(); ++i)
		{
			if (i != 0)
			{
				text += U' ';
			}
			text += Format(ids[i]);
		}
		text += U'\n';
	}

	static void AppendTunas(String& text, const Array<Tuna>& tunas)
	{
		text += Format(tunas.size(), U'\n');
		for (const Tuna& tuna : tunas)
		{
			text += Format(
				tuna.id(), U' ',
				tuna.position().x, U' ',
				tuna.position().y, U'\n');
		}
	}

public:
	DiffLogger(const double sidePanelX, const Simulation& simulation)
		: m_box{ sidePanelX, AppConfig::GridOffsetY + 340, AppConfig::SidePanelWidth, 480 }
		, m_copyButtonPosition{ sidePanelX, AppConfig::GridOffsetY + 830 }
	{
		reset(simulation);
	}

	void reset(const Simulation& simulation)
	{
		m_visibleText.clear();
		m_rollbackTexts.clear();

		const GameMap& map = simulation.map();
		const Point& playerPosition = simulation.player().position();
		m_visibleText += Format(
			MoveLimit, U' ',
			map.height(), U' ',
			map.width(), U' ',
			playerPosition.x, U' ',
			playerPosition.y, U'\n');

		for (const String& row : map.rows())
		{
			m_visibleText += row;
			m_visibleText += U'\n';
		}

		AppendTunas(m_visibleText, simulation.tunas());
	}

	void addDiff(const SimulationDiff& diff, const Array<Tuna>& tunas)
	{
		String latestDiff;
		latestDiff += Format(diff.playerPosition.x, U' ', diff.playerPosition.y, U'\n');
		latestDiff += Format(diff.spawnedTunaIds.size(), U'\n');
		AppendIdLine(latestDiff, diff.spawnedTunaIds);
		latestDiff += Format(diff.caughtTunaIds.size(), U'\n');
		AppendIdLine(latestDiff, diff.caughtTunaIds);
		AppendTunas(latestDiff, tunas);

		m_visibleText = latestDiff;
	}

	void saveRollbackState()
	{
		m_rollbackTexts.push_back(m_visibleText);
	}

	void rollback()
	{
		if (not m_rollbackTexts.empty())
		{
			m_visibleText = m_rollbackTexts.back();
			m_rollbackTexts.pop_back();
		}
	}

	void draw() const
	{
		m_box.draw(Palette::White).drawFrame(0, 2, Palette::Black);
		m_font(m_visibleText).draw(m_box.stretched(-4), Palette::Black);
	}

	void updateCopyButton() const
	{
		if (SimpleGUI::Button(U"差分ログをクリップボードにコピー", m_copyButtonPosition))
		{
			Clipboard::SetText(m_visibleText);
		}
	}
};

class StatisticsDisplay
{
private:
	Texture m_caughtIcon{ U"🐟"_emoji };
	Texture m_moveIcon{ U"👣"_emoji };
	Texture m_resetIcon{ U"🔄"_emoji };
	Texture m_helpIcon{ U"❓"_emoji };
	Font m_font{ FontMethod::MSDF, 22 };
	Font m_tooltipFont{ FontMethod::MSDF, 18 };
	RectF m_panel{ 20, AppConfig::WindowHeight - 90, 460, 60 };

	[[nodiscard]] RectF resetButton() const
	{
		return{ m_panel.x + 358, m_panel.y + 8, 44, 44 };
	}

	[[nodiscard]] RectF helpButton() const
	{
		return{ m_panel.x + 408, m_panel.y + 8, 44, 44 };
	}

	void drawIconButton(
		const RectF& button,
		const Texture& icon,
		const StringView tooltipText) const
	{
		if (button.mouseOver())
		{
			button.rounded(8).draw(ColorF{ 1.0, 0.25 });
			Cursor::RequestStyle(CursorStyle::Hand);

			const RectF tooltip{
				button.center().x - 46,
				button.y - 38,
				92,
				30,
			};
			tooltip.rounded(6).draw(ColorF{ 0.05, 0.95 });
			m_tooltipFont(tooltipText).drawAt(tooltip.center(), Palette::White);
		}
		icon.resized(32).drawAt(button.center());
	}

public:
	[[nodiscard]] bool isResetRequested() const
	{
		return resetButton().mouseOver() && MouseL.down();
	}

	[[nodiscard]] bool isHelpRequested() const
	{
		return helpButton().mouseOver() && MouseL.down();
	}

	void draw(const int32 caughtTunaCount, const int32 moveCount) const
	{
		m_panel.rounded(8)
			.draw(ColorF{ 0.1, 0.1, 0.1, 0.85 })
			.drawFrame(2, Palette::White);

		m_caughtIcon.resized(32).draw(m_panel.x + 12, m_panel.y + 14);
		m_font(Format(U"捕獲 ", caughtTunaCount))
			.draw(m_panel.x + 52, m_panel.y + 16, Palette::White);

		m_moveIcon.resized(32).draw(m_panel.x + 190, m_panel.y + 14);
		m_font(Format(U"移動 ", moveCount))
			.draw(m_panel.x + 230, m_panel.y + 16, Palette::White);

		drawIconButton(resetButton(), m_resetIcon, U"リセット");
		drawIconButton(helpButton(), m_helpIcon, U"ヘルプ");
	}
};

class HelpDialog
{
private:
	static constexpr size_t PageCount = 3;

	bool m_open = false;
	size_t m_page = 0;
	Font m_titleFont{ FontMethod::MSDF, 32, Typeface::Bold };
	Font m_headingFont{ FontMethod::MSDF, 24, Typeface::Bold };
	Font m_bodyFont{ FontMethod::MSDF, 21 };
	Font m_pageFont{ FontMethod::MSDF, 18 };
	RectF m_panel{ 150, 100, 900, 650 };

	void drawTabs() const
	{
		const Array<String> labels{ U"操作", U"ログ", U"ロールバック" };
		const Array<double> widths{ 130, 130, 210 };
		double x = m_panel.x + 40;
		for (size_t i = 0; i < PageCount; ++i)
		{
			const RectF tab{ x, m_panel.y + 78, widths[i], 42 };
			tab.rounded(8, 8, 0, 0).draw(
				i == m_page ? ColorF{ 0.20, 0.55, 0.90 } : ColorF{ 0.75 });
			m_pageFont(labels[i]).drawAt(tab.center(),
				i == m_page ? ColorF{ 1.0 } : ColorF{ 0.25 });
			x += widths[i] + 8;
		}
	}

	void drawPage() const
	{
		const RectF content{ m_panel.x + 40, m_panel.y + 120, m_panel.w - 80, 420 };
		content.draw(ColorF{ 0.96 }).drawFrame(0, 2, ColorF{ 0.70 });

		String heading;
		String body;
		switch (m_page)
		{
		case 0:
			heading = U"マグロを追いかける";
			body =
				U"● 矢印キーで一歩ずつ移動できます。\n"
				U"    ← 左　　↓ 下　　→ 右　　↑ 上\n\n"
				U"● 画面右上の入力欄で、複数歩をまとめて指定できます。\n"
				U"    L = 左　D = 下　R = 右　U = 上\n"
				U"    例:  RRDDLU  と入力して「実行」を押します。";
			break;
		case 1:
			heading = U"二つのログ";
			body =
				U"● Input ログ\n"
				U"    実行した移動コマンドを記録します。\n\n"
				U"● 差分ログ\n"
				U"    プレイヤーの位置、出現・捕獲した魚、\n"
				U"    現在の魚の位置を表示します。\n\n"
				U"● それぞれのボタンからログをクリップボードへ\n"
				U"    コピーできます。";
			break;
		default:
			heading = U"操作をロールバックする";
			body =
				U"● Ctrl + Z で直前の操作を1回分戻せます。\n\n"
				U"● 入力欄から複数歩をまとめて実行した場合は、\n"
				U"    その移動全体が1回分の操作として戻ります。\n\n";
			break;
		}

		m_headingFont(heading).draw(content.x + 24, content.y + 22, ColorF{ 0.15, 0.35, 0.60 });
		m_bodyFont(body).draw(RectF{
			content.x + 24,
			content.y + 76,
			content.w - 48,
			content.h - 96,
		}, ColorF{ 0.15 });
	}

public:
	[[nodiscard]] bool isOpen() const
	{
		return m_open;
	}

	void open()
	{
		m_open = true;
		m_page = 0;
	}

	void updateAndDraw()
	{
		if (not m_open)
		{
			return;
		}

		Rect{ Scene::Size() }.draw(ColorF{ 0.0, 0.58 });
		m_panel.rounded(14)
			.draw(ColorF{ 0.92 })
			.drawFrame(0, 3, ColorF{ 0.20, 0.45, 0.70 });
		m_titleFont(U"ヘルプ").draw(m_panel.x + 40, m_panel.y + 24, ColorF{ 0.15 });

		drawTabs();
		drawPage();

		const bool canGoBack = (0 < m_page);
		const bool canGoForward = ((m_page + 1) < PageCount);
		if (SimpleGUI::Button(
			U"← 前へ",
			Vec2{ m_panel.x + 40, m_panel.bottomY() - 66 },
			120,
			canGoBack))
		{
			--m_page;
		}
		if (SimpleGUI::Button(
			U"次へ →",
			Vec2{ m_panel.rightX() - 160, m_panel.bottomY() - 66 },
			120,
			canGoForward))
		{
			++m_page;
		}

		m_pageFont(Format(m_page + 1, U" / ", PageCount))
			.drawAt(m_panel.center().x, m_panel.bottomY() - 46, ColorF{ 0.30 });

		if (SimpleGUI::Button(
			U"×",
			Vec2{ m_panel.rightX() - 62, m_panel.y + 18 },
			42)
			|| KeyEscape.down())
		{
			m_open = false;
		}
	}
};

Optional<MoveDirection> ReadKeyboardMove()
{
	if (KeyLeft.pressed())
	{
		return MoveDirection::Left;
	}
	if (KeyDown.pressed())
	{
		return MoveDirection::Down;
	}
	if (KeyRight.pressed())
	{
		return MoveDirection::Right;
	}
	if (KeyUp.pressed())
	{
		return MoveDirection::Up;
	}
	return none;
}

bool IsRollbackRequested()
{
	return KeyControl.pressed() && KeyZ.down();
}

char32 ToCommandCharacter(const MoveDirection direction)
{
	switch (direction)
	{
	case MoveDirection::Left:
		return U'L';
	case MoveDirection::Down:
		return U'D';
	case MoveDirection::Right:
		return U'R';
	case MoveDirection::Up:
		return U'U';
	default:
		return U'\0';
	}
}

void ProcessKeyboardInput(Simulation& simulation, InputLogger& logger, DiffLogger& diffLogger)
{
	if (const Optional<MoveDirection> direction = ReadKeyboardMove())
	{
		logger.saveRollbackState();
		diffLogger.saveRollbackState();
		const SimulationDiff diff = simulation.step(*direction);
		logger.addMove(ToCommandCharacter(*direction));
		diffLogger.addDiff(diff, simulation.tunas());
	}
	else if (KeyEnter.down())
	{
		logger.addNewLine();
	}
}

bool ProcessSubmittedCommand(
	const String& command,
	Simulation& simulation,
	InputLogger& logger,
	DiffLogger& diffLogger)
{
	const Optional<Array<MoveDirection>> directions = ParseCommand(command);
	if (not directions)
	{
		return false;
	}

	if (not directions->empty())
	{
		logger.saveRollbackState();
		diffLogger.saveRollbackState();
	}
	const SimulationDiff diff = simulation.run(*directions);
	logger.addCommand(command);
	if (not directions->empty())
	{
		diffLogger.addDiff(diff, simulation.tunas());
	}
	return true;
}

void ProcessRollback(Simulation& simulation, InputLogger& logger, DiffLogger& diffLogger)
{
	if (simulation.rollback())
	{
		logger.rollback();
		diffLogger.rollback();
	}
}

void ProcessReset(
	Simulation& simulation,
	CommandInputer& inputer,
	InputLogger& logger,
	DiffLogger& diffLogger)
{
	simulation.reset();
	inputer.reset();
	logger.reset();
	diffLogger.reset(simulation);
}

void Main()
{
	Window::Resize(AppConfig::WindowWidth, AppConfig::WindowHeight);
	Scene::SetBackground(Palette::Darkgray);

	Simulation simulation;
	const double sidePanelX = simulation.map().right() + AppConfig::SidePanelMargin;
	CommandInputer inputer{ sidePanelX };
	InputLogger logger{ sidePanelX };
	DiffLogger diffLogger{ sidePanelX, simulation };
	StatisticsDisplay statisticsDisplay;
	HelpDialog helpDialog;
	Timer invalidCommandMessage{ 0s, StartImmediately::Yes };

	while (System::Update())
	{
		ClearPrint();
		const bool acceptsInput = (not simulation.isAnimating());
		if ((not helpDialog.isOpen()) && statisticsDisplay.isHelpRequested())
		{
			helpDialog.open();
		}
		const bool helpIsOpen = helpDialog.isOpen();

		if (not helpIsOpen && statisticsDisplay.isResetRequested())
		{
			ProcessReset(simulation, inputer, logger, diffLogger);
			invalidCommandMessage.setRemaining(0s);
		}
		else if (not helpIsOpen
			&& acceptsInput
			&& (not inputer.isActive())
			&& IsRollbackRequested())
		{
			inputer.update(false);
			ProcessRollback(simulation, logger, diffLogger);
		}
		else if (not helpIsOpen)
		{
			if (const Optional<String> command = inputer.update(acceptsInput))
			{
				if (not ProcessSubmittedCommand(*command, simulation, logger, diffLogger))
				{
					invalidCommandMessage.setRemaining(1.0s);
				}
			}
			else if (acceptsInput && (not inputer.isActive()))
			{
				ProcessKeyboardInput(simulation, logger, diffLogger);
			}
		}

		if (not invalidCommandMessage.reachedZero())
		{
			Print << U"LRDU以外の文字が含まれます";
		}

		simulation.draw();
		statisticsDisplay.draw(simulation.caughtTunaCount(), simulation.moveCount());
		if (not helpIsOpen)
		{
			logger.updateCopyButton();
			diffLogger.updateCopyButton();
		}
		logger.draw();
		diffLogger.draw();
		helpDialog.updateAndDraw();
	}
}

//
// - Debug ビルド: プログラムの最適化を減らす代わりに、エラーやクラッシュ時に詳細な情報を得られます。
//
// - Release ビルド: 最大限の最適化でビルドします。
//
// - [デバッグ] メニュー → [デバッグの開始] でプログラムを実行すると、[出力] ウィンドウに詳細なログが表示されます。
//
// - Visual Studio を更新した直後は、[ビルド] メニュー → [ソリューションのリビルド] が必要な場合があります。
//
