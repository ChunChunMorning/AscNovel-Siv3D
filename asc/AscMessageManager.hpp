# pragma once
# include <Siv3D.hpp>

namespace asc
{
	using namespace s3d;

	class MessageManager
	{
	private:

		bool m_isAutomatic;

		Stopwatch m_stopwatch;

		String m_name;

		String m_text;

		uint32 m_charCount;

		std::function<void()> m_onCountChar;

	public:

		MessageManager() = default;

		MessageManager(std::function<void()> onCountChar) :
			m_charCount(0U),
			m_onCountChar(onCountChar)
		{
			m_stopwatch.start();
			m_stopwatch.pause();
		}

		virtual ~MessageManager() = default;

		void start(bool isAutomatic = false)
		{
			m_stopwatch.restart();
			m_charCount = 0U;
			m_isAutomatic = isAutomatic;
		}

		void update()
		{
			// ToDo Configurable
			const int32 m_textSpeed = 100;
			const int32 m_textWait = 100;



			if(m_stopwatch.isPaused())
				return;

			const auto typingTime = static_cast<int32>(m_text.length) * m_textSpeed;

			if (m_stopwatch.ms() >= typingTime)
			{
				if (m_stopwatch.ms() >= typingTime + m_textWait && (Input::KeyEnter.clicked || Input::KeyQ.pressed || m_isAutomatic))
				{
					m_stopwatch.pause();
				}
			}
			else if(Input::KeyEnter.clicked || Input::KeyQ.pressed)
			{
				m_stopwatch.set(static_cast<Milliseconds>(m_text.length * m_textSpeed));
			}

			const auto charCount = Min<uint32>(m_stopwatch.ms() / m_textSpeed, m_text.length);

			if (charCount > m_charCount)
			{
				m_charCount = charCount;

				if (m_text[charCount - 1] != L' ')
				{
					m_onCountChar();
				}
			}
		}

		void setName(const String& name)
		{
			m_name = name;
		}

		void setText(const String& text)
		{
			m_text = text;
		}

		void clear()
		{
			m_name.clear();
			m_text.clear();
			m_stopwatch.pause();
		}

		void draw() const
		{
			// ToDo Configurable
			const Rect m_messageBox(6, 440, 1268, 285);
			const Point m_namePosition(40, 525);
			const Point m_textPosition(60, 575);
			const String m_messageBoxTexture = L"test_message_box";
			const String m_nameFont = L"test_name";
			const String m_textFont = L"test_text";
			const Color m_messageColor = Palette::Black;



			m_messageBox(TextureAsset(m_messageBoxTexture)).draw();

			FontAsset(m_nameFont).draw(m_name, m_namePosition, m_messageColor);
			FontAsset(m_textFont).draw(m_text.substr(0U, m_charCount), m_textPosition, m_messageColor);
		}

		bool isUpdating() const
		{
			return !m_stopwatch.isPaused();
		}

	};
}
