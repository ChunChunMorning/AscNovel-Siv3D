# include "AscNovel.hpp"
# include <Siv3D.hpp>
# include "AscChoiceManager.hpp"
# include "AscMessageManager.hpp"
# include "AscIMessgeButton.hpp"
# include "AscSoundManager.hpp"
# include "AscSpriteManager.hpp"
# include "AscTimeManager.hpp"

namespace asc
{
	using namespace s3d;

	const auto EndOfCommand = Array<String>{ L"0", L"-1" };

	enum class State { None, Start, Update, Finish };

	class Novel::CNovel
	{
	public:

		State state;

		int32 currentLine;

		int32 lastSeekPoint;

		KeyCombination skip;

		Array<Array<String>> commands;

		ChoiceManager choiceManager;

		MessageManager messageManager;

		SoundManager soundManager;

		SpriteManager spriteManager;

		TimeManager timeManager;

		CNovel() :
			state(State::None),
			currentLine(0),
			lastSeekPoint(-1),
			commands({ EndOfCommand }),
			choiceManager(
				[&] { soundManager.playMoveSound(); },
				[&] { soundManager.playSubmitSound(); }
				),
			messageManager(
				[&] { soundManager.playCharSound(); }
				) {}

		void clearManager()
		{
			choiceManager.clear();
			messageManager.clear();
			spriteManager.clear();
			timeManager.clear();
		}

		void execute()
		{
			switch (Parse<int32>(commands[currentLine][0]))
			{
			// SeekPoint
			case 0:
				state = State::Finish;
				return;

			// Text
			case 1:
				messageManager.setText(commands[currentLine][1]);
				messageManager.start(commands[currentLine].size() > 2);
				break;

			// Name
			case 2:
				messageManager.setName(commands[currentLine][1]);
				break;

			// Sprite
			case 3:
				spriteManager.add<Sprite>(
					Parse<int32>(commands[currentLine][1]),
					commands[currentLine][2],
					Parse<double>(commands[currentLine][3]),
					Parse<double>(commands[currentLine][4]),
					Parse<double>(commands[currentLine][5]),
					Parse<double>(commands[currentLine][6])
					);
				break;

			// FixedSprite
			case 4:
				spriteManager.add<FixedSprite>(
					Parse<int32>(commands[currentLine][1]),
					commands[currentLine][2],
					Parse<double>(commands[currentLine][3]),
					Parse<double>(commands[currentLine][4]),
					Parse<double>(commands[currentLine][5]),
					Parse<double>(commands[currentLine][6])
					);
				break;

			// Bring
			case 5:
				spriteManager.bring(Parse<int32>(commands[currentLine][1]));
				break;

			// Lihgt
			case 6:
				spriteManager.lightUp(Parse<int32>(commands[currentLine][1]), commands[currentLine].size() < 3);
				break;

			// Spot
			case 7:
				spriteManager.lightUpSpot(Parse<int32>(commands[currentLine][1]));
				break;

			// Erase
			case 8:
				spriteManager.erase(Parse<int32>(commands[currentLine][1]));
				break;

			// Choice
			case 9:
			{
				Array<std::pair<int32, String>> choices;

				for (auto i = 1U; i < commands[currentLine].size(); i += 2)
				{
					choices.push_back(std::make_pair(Parse<int32>(commands[currentLine][i]), commands[currentLine][i + 1]));
				}

				choiceManager.start(choices);
				break;
			}

			// Play
			case 10:
				soundManager.playBGM(commands[currentLine][1], Parse<double>(commands[currentLine][2]));
				break;

			// Stop
			case 11:
				soundManager.stopBGM(commands[currentLine][1], Parse<double>(commands[currentLine][2]));
				break;

			// Jump
			case 12:
				start(Parse<int32>(commands[currentLine][1]));
				return;

			// Wait
			case 13:
				timeManager.wait(Parse<double>(commands[currentLine][1]));
				break;

			// Speed
			case 14:
				messageManager.setSpeed(Parse<double>(commands[currentLine][1]));
				break;

			// Time
			case 15:
				messageManager.setTime(Parse<double>(commands[currentLine][1]));
				break;

			// TextureLoad
			case 16:
				TextureAsset::Preload(commands[currentLine][1]);
				break;

			// TextureRelease
			case 17:
				TextureAsset::Release(commands[currentLine][1]);
				break;

			// SoundLoad
			case 18:
				SoundAsset::Preload(commands[currentLine][1]);
				break;

			// SoundRelease
			case 19:
				SoundAsset::Release(commands[currentLine][1]);
				break;

			default:
				break;
			}

			currentLine++;
		}

		bool start(int32 seekPoint)
		{
			const auto size = commands.size() - 1U;
			for (auto i = 0u; i < size; i++)
			{
				const auto index = (currentLine + i) % commands.size();

				if (
					Parse<int32>(commands[index][0]) == 0 &&
					Parse<int32>(commands[index][1]) == seekPoint
					)
				{
					clearManager();
					currentLine = index + 1;
					lastSeekPoint = seekPoint;
					state = State::Start;

					return true;
				}
			}

			return false;
		}

		void updateState()
		{
			switch (state)
			{
			case State::Start:
				state = State::Update;
				break;

			case State::Finish:
				state = State::None;
				break;

			default:
				break;
			}
		}

		void skipCommand()
		{
			while (state == State::Update && !choiceManager.isUpdating())
			{
				execute();
			}

			messageManager.skip();
			timeManager.clear();
		}

	};
}

asc::Novel::Novel() : pImpl(std::make_shared<CNovel>()) {}

bool asc::Novel::load(const FilePath& path, const Optional<TextEncoding>& encoding, bool isAdditive)
{
	TextReader reader(path, encoding);

	if (!reader.isOpened())
		return false;

	loadByString(reader.readAll(), isAdditive);
	return true;
}

void asc::Novel::loadByString(const String& scenario, bool isAdditive)
{
	isAdditive ? pImpl->commands.pop_back() : pImpl->commands.clear();

	const auto lines = scenario.trim().split(L'\n');

	for (const auto& line : lines)
	{
		pImpl->commands.push_back(line.replace(L"\\n", L"\n").split(L','));
	}

	pImpl->commands.push_back(EndOfCommand);
}

void asc::Novel::clear()
{
	pImpl->clearManager();
	pImpl->commands.clear();
}

bool asc::Novel::start(int32 seekPoint)
{
	return pImpl->start(seekPoint);
}

void asc::Novel::update()
{
	pImpl->updateState();

	if (pImpl->skip.clicked)
	{
		pImpl->skipCommand();
	}

	while (
		pImpl->state == State::Update &&
		!pImpl->choiceManager.isUpdating() &&
		!pImpl->messageManager.isUpdating() &&
		!pImpl->timeManager.isUpdating()
		)
	{
		pImpl->choiceManager.lastSelectedSeekPoint().then([&](int32 seekPoint) { start(seekPoint); });
		pImpl->execute();
	}

	pImpl->choiceManager.update();
	pImpl->messageManager.update();
}

Optional<int32> asc::Novel::isStarted() const
{
	if(pImpl->state != State::Start)
		return none;

	return pImpl->lastSeekPoint;
}

Optional<int32> asc::Novel::isUpdating() const
{
	if (pImpl->state != State::Update)
		return none;

	return pImpl->lastSeekPoint;
}

Optional<int32> asc::Novel::isFinished() const
{
	if (pImpl->state != State::Finish)
		return none;

	return pImpl->lastSeekPoint;
}

int32 asc::Novel::seekPoint() const
{
	return pImpl->lastSeekPoint;
}

void asc::Novel::draw() const
{
	pImpl->spriteManager.draw();
	pImpl->messageManager.draw();
	pImpl->choiceManager.draw();
}

asc::Novel& asc::Novel::setSpeed(double charPerSecond)
{
	pImpl->messageManager.setSpeed(charPerSecond);

	return *this;
}

asc::Novel& asc::Novel::setWaitingTime(double second)
{
	pImpl->messageManager.setTime(second);

	return *this;
}

asc::Novel& asc::Novel::setKey(const KeyCombination& submit, const KeyCombination& skip)
{
	pImpl->skip = skip;
	pImpl->messageManager.setKey(submit);

	return *this;
}

asc::Novel& asc::Novel::setKey(const KeyCombination& submit, const KeyCombination& skip, const KeyCombination& up, const KeyCombination& down)
{
	pImpl->skip = skip;
	pImpl->choiceManager.setKey(submit, up, down);
	pImpl->messageManager.setKey(submit);

	return *this;
}

asc::Novel& asc::Novel::setButton(std::unique_ptr<IMessageButton>&& button)
{
	pImpl->messageManager.setButton(std::move(button));

	return *this;
}

asc::Novel& asc::Novel::setFont(const FontAssetName& text, const FontAssetName& name)
{
	pImpl->choiceManager.setFont(text);
	pImpl->messageManager.setFont(text, name);

	return *this;
}

asc::Novel& asc::Novel::setColor(const Color& color, const Color& selectedColor)
{
	pImpl->choiceManager.setColor(color, selectedColor);
	pImpl->messageManager.setColor(color);

	return *this;
}

asc::Novel& asc::Novel::setMessageTexture(const TextureAssetName& texture, const Rect& region)
{
	pImpl->messageManager.setTexture(texture, region);

	return *this;
}

asc::Novel& asc::Novel::setMessagePosition(const Point& text, const Point& name)
{
	pImpl->messageManager.setPosition(text, name);

	return *this;
}

asc::Novel& asc::Novel::setChoiceTexture(const TextureAssetName texture, const Rect& region)
{
	pImpl->choiceManager.setTexture(texture, region);

	return *this;
}

asc::Novel& asc::Novel::setChoicePosition(const Point& position)
{
	pImpl->choiceManager.setPosition(position);

	return *this;
}

asc::Novel& asc::Novel::setBGMVolume(double volume)
{
	pImpl->soundManager.setBGMVolume(volume);

	return *this;
}

asc::Novel& asc::Novel::setSEVolume(double volume)
{
	pImpl->soundManager.setSEVolume(volume);

	return *this;
}

asc::Novel& asc::Novel::setSound(const SoundAssetName& charCount)
{
	pImpl->soundManager.setSE(charCount);

	return *this;
}

asc::Novel& asc::Novel::setSound(const SoundAssetName& charCount, const SoundAssetName& move, const SoundAssetName& submit)
{
	pImpl->soundManager.setSE(charCount, move, submit);

	return *this;
}

asc::Novel& asc::Novel::setSilentChars(const Array<wchar> silentChars)
{
	pImpl->messageManager.setSilentChars(silentChars);

	return *this;
}
