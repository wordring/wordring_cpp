﻿#pragma once

#include <deque>
#include <iterator>

namespace wordring::whatwg
{
	template <typename EncodingStream>
	class encoding_stream_iterator
	{
	public:
		using container_type = EncodingStream;
	};

	// 3. Terminology ---------------------------------------------------------

	template <typename InputIterator>
	class encoding_stream
	{
	public:
		using iterator_type = InputIterator;
		using value_type = typename std::iterator_traits<iterator_type>::value_type;

	public:
		encoding_stream(iterator_type first, iterator_type last) : m_first(first), m_last(last) {}

		bool operator !() const { return m_front.empty() && m_back.empty() && m_first == m_last; }

		operator bool() const { return !(operator!()); }

		value_type read()
		{
			if (!m_front.empty())
			{
				value_type result{ m_front.front() };
				m_front.pop_front();
				return result;
			}
			if (m_first != m_last) return *m_first++;
			if (!m_back.empty())
			{
				value_type result{ m_back.front() };
				m_back.pop_front();
				return result;
			}
			assert(false);
			return 0;
		}

		void prepend(value_type token) { m_front.push_front(token); }

		template <typename InputIterator>
		void prepend(InputIterator first, InputIterator last) { m_front.insert(m_front.begin(), first, last); }

		void push(value_type token) { m_back.push_back(token); }

		template <typename InputIterator>
		void push(InputIterator first, InputIterator last) { m_back.insert(m_back.end(), first, last); }

	private:
		std::deque<value_type> m_front;
		std::deque<value_type> m_back;
		iterator_type m_first;
		iterator_type m_last;
	};

}