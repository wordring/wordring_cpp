﻿#pragma once

#include <wordring/static_vector/static_vector.hpp>
#include <wordring/trie/list_trie_iterator.hpp>
#include <wordring/trie/trie_construct_iterator.hpp>
#include <wordring/trie/trie_heap.hpp>

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <deque>
#include <istream>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace wordring
{
	struct trie_base_value_proxy
	{
		using index_type = typename trie_node::index_type;

		index_type* m_base;

		trie_base_value_proxy(index_type* base)
			: m_base(base)
		{
		}

		void operator=(index_type val)
		{
			if (val < 0) throw std::invalid_argument("");
			*m_base = -val;
		}

		operator index_type() const
		{
			return -*m_base;
		}
	};

	template <typename Container>
	class const_trie_base_iterator
	{
		template <typename Allocator>
		friend class trie_base;

		template <typename Container1>
		friend bool operator==(const_trie_base_iterator<Container1> const&, const_trie_base_iterator<Container1> const&);

		template <typename Container1>
		friend bool operator!=(const_trie_base_iterator<Container1> const&, const_trie_base_iterator<Container1> const&);

	public:
		using difference_type = std::ptrdiff_t;
		using value_type = std::uint8_t;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::input_iterator_tag;

	protected:
		using index_type = typename trie_node::index_type;
		using container = Container const;

		static constexpr std::uint16_t null_value = 256;

	public:
		const_trie_base_iterator()
			: m_c(nullptr)
			, m_index(0)
		{
		}

	protected:
		const_trie_base_iterator(container& c, index_type index)
			: m_c(std::addressof(c))
			, m_index(index)
		{
		}

	public:
		/*! 文字列終端の場合trueを返す*/
		operator bool() const
		{
			if (m_index <= 1) return false;

			auto data = m_c->data();
			index_type idx = (data + m_index)->m_base; // 子のBASEインデックス。

			// 子が無い（BASEが1未満）場合。
			if (idx <= 0) return true;
			// 子はあるが、途中で終端もある（文字null_valueによる遷移）場合。
			if ((idx + null_value) < tail() && (data + idx + null_value)->m_check == m_index) return true;

			return false;
		}

		bool operator!() const { return operator bool() == false; }

		value_type operator*() const
		{
			assert(base() <= m_index && m_index < base() + null_value);

			return m_index - base();
		}

		const_trie_base_iterator operator[](value_type label) const
		{
			auto data = m_c->data();

			index_type idx = (data + m_index)->m_base + label;
			idx = (idx < tail() && (data + idx)->m_check == m_index) ? idx
				: 0;
			return const_trie_base_iterator(*m_c, idx);
		}

		const_trie_base_iterator& operator++()
		{
			m_index = find(m_index + 1, base() + null_value, mother());

			return *this;
		}

		const_trie_base_iterator operator++(int)
		{
			auto result = *this;

			operator++();

			return result;
		}

		template <typename String>
		String string() const
		{
			std::basic_string<value_type> tmp;
			for (auto p = *this; 1 < p.m_index; p = *p.parent()) tmp.push_back(*p);
			return String(tmp.rbegin(), tmp.rend());
		}

		std::optional<const_trie_base_iterator> parent() const
		{
			return 1 < m_index ? std::optional<const_trie_base_iterator>(const_trie_base_iterator(*m_c, (m_c->data() + m_index)->m_check))
				: std::nullopt;
		}

		/*! 0-255に相当する文字で遷移できる最初の子を指すイテレータを返す
		- 256による空遷移は含めない。
		- 遷移先（子）が無い場合、end()を返す。
		*/
		const_trie_base_iterator begin() const
		{
			assert(1 <= m_index);

			index_type i = (m_c->data() + m_index)->m_base;
			i = (0 < i) ? find(i, i + null_value, m_index) : 0;
			return const_trie_base_iterator(*m_c, i);
		}

		const_trie_base_iterator end() const
		{
			return const_trie_base_iterator(*m_c, 0);
		}

	protected:
		/*! 引数firstからlastの範囲で、m_checkが引数checkと一致する状態番号を返す

		- 見つからない場合、0を返す。
		- 状態番号は1から始まるので0と衝突しない。
		- 引数lastがコンテナの大きさを超える場合、そこで検索を終了する。
		*/
		index_type find(index_type first, index_type last, index_type check) const
		{
			index_type limit = std::min(last, static_cast<index_type>(m_c->size()));
			for (; first < limit; ++first) if ((m_c->data() + first)->m_check == check) break;

			return (first != limit) ? first : 0;
		}

		/*! 親の状態番号を返す
		*/
		index_type mother() const
		{
			assert(1 <= m_index && m_index < tail());

			auto data = m_c->data();
			index_type parent = (data + m_index)->m_check;

			return 1 <= parent ? parent : 0;
		}

		/*! 親のBASEを返す
		- つまりコレはm_indexからラベルを差し引いた値にに相当する。
		*/
		index_type base() const
		{
			auto data = m_c->data();

			index_type parent = mother();
			assert(parent != 0);

			index_type idx = (parent != 0) ? (data + parent)->m_base : 0;
			assert(1 <= idx); // thisが子である以上、親のBASEが存在する。

			return idx;
		}

		/*! コンテナの最終状態番号の次を返す
		- キャストで行が増えるのを抑制するために用意した。
		*/
		index_type tail() const { return m_c->size(); }

		/*! 空遷移がある場合trueを返す
		*/
		bool has_null() const
		{
			if (m_index == 1) return false;

			auto data = m_c->data();
			index_type idx = (data + m_index)->m_base + null_value;

			return (idx < tail() && (data + idx)->m_check == m_index);
		}

		/*! 兄弟にあたる状態がある場合、trueを返す
		*/
		bool has_sibling() const
		{
			if (m_index == 1) return false;

			index_type check = (m_c->data() + m_index)->m_check;

			index_type first = base();
			index_type last = first + null_value;

			while (true)
			{
				first = find(first, last, check);
				if (first == 0) return false;
				if (first != m_index) return true;
				++first;
			}

			return false;
		}

		/*! 遷移可能な子ラベルを列挙する

		- null_value を含む
		*/
		void children(static_vector<std::uint16_t, 257>& labels) const
		{
			assert(1 <= m_index);
			assert(labels.empty());

			index_type base = (m_c->data() + m_index)->m_base; // 子の開始
			if (base <= 0) return; // 0以下は子が無い

			index_type first = base;
			index_type last = std::min(base + null_value + 1, static_cast<index_type>(m_c->size()));
			for (; first < last; ++first)
			{
				index_type check = (m_c->data() + first)->m_check;
				if (check == m_index) labels.push_back(static_cast<std::uint16_t>(first - base));
			}
		}

	protected:
		container* m_c;
		index_type m_index;
	};

	template <typename Container1>
	inline bool operator==(const_trie_base_iterator<Container1> const& lhs, const_trie_base_iterator<Container1> const& rhs)
	{
		assert(lhs.m_c == rhs.m_c);
		return lhs.m_index == rhs.m_index;
	}

	template <typename Container1>
	inline bool operator!=(const_trie_base_iterator<Container1> const& lhs, const_trie_base_iterator<Container1> const& rhs)
	{
		return !(lhs == rhs);
	}

	// ------------------------------------------------------------------------
	// trie_base
	// ------------------------------------------------------------------------

	/*!
	- 挿入や削除によってすべてのイテレータが無効となる。
	*/
	template <typename Allocator = std::allocator<trie_node>>
	class trie_base : public trie_heap<Allocator>
	{
		template <typename Allocator1>
		friend std::basic_ostream<char>& operator<<(std::basic_ostream<char>&, trie_base<Allocator1> const&);

		template <typename Allocator1>
		friend std::basic_istream<char>& operator>>(std::basic_istream<char>&, trie_base<Allocator1>&);

		template <typename Allocator1>
		friend std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>&, trie_base<Allocator1> const&);

		template <typename Allocator1>
		friend std::vector<std::uint8_t>& operator>>(std::vector<std::uint8_t>&, trie_base<Allocator1>&);

		template <typename Allocator1>
		friend std::vector<std::uint32_t>& operator<<(std::vector<std::uint32_t>&, trie_base<Allocator1> const&);

		template <typename Allocator1>
		friend std::vector<std::uint32_t>& operator>>(std::vector<std::uint32_t>&, trie_base<Allocator1>&);

	public:
		using base_type = trie_heap<Allocator>;
		using container = typename base_type::container;

		using label_vector = typename base_type::label_vector;

		using value_type = trie_base_value_proxy;
		using size_type = typename container::size_type;
		using allocator_type = Allocator;
		using reference = trie_base_value_proxy;
		using const_reference = trie_base_value_proxy const;
		using const_iterator = const_trie_base_iterator<container const>;

		using index_type = typename trie_node::index_type;

		static constexpr std::uint16_t null_value = base_type::null_value;

	protected:
		using typename base_type::node_type;

		using base_type::trie_heap;
		using base_type::reserve;
		using base_type::allocate;
		using base_type::relocate;
		using base_type::free;
		using base_type::locate;
		using base_type::is_free;

		using base_type::m_c;

	public:
		trie_base()
			: base_type()
		{
		}

		template <typename InputIterator>
		void assign(InputIterator first, InputIterator last)
		{
			clear();

			std::sort(first, last);
			last = std::unique(first, last);

			using list_iterator = const_list_trie_iterator<InputIterator>;
			using construct_iterator = trie_construct_iterator<list_iterator>;

			auto li = list_iterator(first, last);
			auto it = construct_iterator(li);

			while (!it.empty() && !it.children().empty())
			{
				auto view = it.parent();
				auto it1 = search(view.begin(), view.end());
				if (it1.first.m_index == 0) it1.first.m_index = 1; // rootの場合。
				add(it1.first, it.children());
				++it;
			}

			m_size = std::distance(first, last);
		}

		// 要素アクセス --------------------------------------------------------

		template <typename Key>
		reference at(Key const& key)
		{
			auto it = find(key);
			if (it == cend()) throw std::out_of_range("");

			// 子遷移が有り、なおかつ文字列終端の場合に対応する。
			index_type idx = ((m_c.data() + it.m_index)->m_base <= 0)
				? it.m_index
				: it.base() + null_value;

			return reference(std::addressof((m_c.data() + idx)->m_base));
		}

		template <typename Key>
		const_reference const at(Key const& key) const
		{
			return const_cast<trie_base*>(this)->at(key);
		}

		template <typename Key>
		reference operator[](Key const& key)
		{
			auto it = find(key);
			if (it == cend()) it = insert(key);

			// 子遷移が有り、なおかつ文字列終端の場合に対応する。
			index_type idx = ((m_c.data() + it.m_index)->m_base <= 0)
				? it.m_index
				: it.base() + null_value;

			return reference(std::addressof((m_c.data() + idx)->m_base));
		}

		// イテレータ ----------------------------------------------------------

		const_iterator begin() const noexcept { return const_iterator(m_c, 1); }

		const_iterator cbegin() const noexcept { return const_iterator(m_c, 1); }

		const_iterator end() const noexcept { return const_iterator(m_c, 0); }

		const_iterator cend() const noexcept { return const_iterator(m_c, 0); }

		// 容量 ---------------------------------------------------------------

		bool empty() const noexcept { return size() == 0; }

		size_type size() const noexcept { return m_c.front().m_base; }

		static constexpr size_type max_size() noexcept { return std::numeric_limits<std::int32_t>::max() / 4; }

		// 変更 ---------------------------------------------------------------

		void clear() noexcept
		{
			m_c.clear();
			m_c.insert(m_c.begin(), 2, trie_node{ 0, 0 });
		}

		template <typename Key>
		const_iterator insert(Key const& key)
		{
			auto it1 = std::begin(key);
			auto it2 = std::end(key);
			if (it1 == it2) return cend();

			index_type parent = 1;

			// 登録済み遷移をスキップする
			for (index_type idx = at(parent, *it1); idx != 0; idx = at(parent, *it1))
			{
				parent = idx;
				++it1;
				if (it1 == it2) break;
			}
			if (it1 != it2) ++m_c.front().m_base;

			// 空遷移の解決
			if (has_child(parent)) // 既存遷移有り、新規文字列無し
			{
				if (it1 == it2 && !has_null(parent))
				{
					add(parent, null_value);
					++m_c.front().m_base;
				}
			}
			else if (parent != 1 && !has_null(parent) && it1 != it2) // 後続遷移無し、新規文字列有り
				add(parent, null_value);

			// 新規文字列による遷移を追加
			while (it1 != it2)
			{
				std::uint16_t label = static_cast<std::uint8_t>(*it1++);
				index_type base = add(parent, label);
				parent = base + label;
			}

			assert(parent != 1); // keyが空文字列の場合？
			//if (parent == 1) parent = 0;

			return const_iterator(m_c, parent);
		}

		template <typename InputIterator>
		void insert(InputIterator first, InputIterator last)
		{
			while (first != last) insert(*first++);
		}

		void erase(const_iterator pos)
		{
			if (!pos) return;

			while (true)
			{
				if (pos.has_null())
				{
					index_type idx = (m_c.data() + pos.m_index)->m_base + null_value;
					assert((m_c.data() + idx)->m_check == pos.m_index);

					free(idx);
					if (pos.begin() == pos.end()) (m_c.data() + pos.m_index)->m_base = 0;

					break;
				}

				std::optional<const_iterator> parent;
				if (!pos.has_sibling()) parent = pos.parent(); // 兄弟が無い場合

				free(pos.m_index);

				if (!parent.has_value()) break;
				pos = parent.value();
			}

			--m_c.front().m_base;
		}

		template <typename Key>
		void erase(Key const& key) { erase(find(key)); }

		// 検索 ---------------------------------------------------------------

		/*! 部分一致検索

		- 引数first, lastは検索文字列のbegin(), end()。
		- 戻り値は一致した最後の状態と文字を指すイテレータのペア。
		*/
		template <typename Iterator>
		auto search(Iterator first, Iterator last) const->std::pair<const_iterator, Iterator>
		{
			const_iterator it1 = cbegin();
			const_iterator it2 = cend();

			for (; first != last; ++first)
			{
				auto it = it1[static_cast<std::uint8_t>(*first)];
				if (it == it2) break;
				it1 = it;
			}

			return std::make_pair(it1 == cbegin() ? cend() : it1, first);
		}

		/*! 前方一致検索
		*/
		template <typename Key>
		const_iterator search(Key const& key) const
		{
			auto pair = search(key.begin(), key.end());
			return pair.second == key.end() ? pair.first : cend();
		}

		/*! 完全一致検索

		- 子遷移があり、文字列終端でもある場合に注意。
		*/
		template <typename Key>
		const_iterator find(Key const& key) const
		{
			auto it = search(key);
			return it ? it : cend();
		}

		template <typename Key>
		bool contains(Key const& key) const
		{
			auto it1 = std::begin(key);
			auto it2 = std::end(key);

			index_type parent = 1;

			index_type limit = m_c.size();
			node_type const* d = m_c.data();
			while (it1 != it2)
			{
				if (parent <= 0) return false;

				index_type base = (d + parent)->m_base;
				if (base < 1) return false;

				std::uint16_t label = static_cast<std::uint8_t>(*it1++);
				index_type idx = base + label;
				if (limit <= idx || (d + idx)->m_check != parent) return false;

				parent = idx;
				assert(parent < limit);
			}
			assert(it1 == it2);

			// 空遷移検出
			if ((d + parent)->m_base != 0)
			{
				index_type base = (d + parent)->m_base;
				if (base < 1) return false;
				index_type idx = base + null_value;
				if (limit <= idx) return false;
				if ((d + idx)->m_check != parent) return false;
			}

			return true;
		}

	protected:
		bool has_child(index_type parent) const
		{
			node_type const* d = m_c.data();

			index_type base = (d + parent)->m_base;
			index_type limit = std::min(base + null_value, static_cast<index_type>(m_c.size()));

			for (index_type idx = base; idx < limit; ++idx) if ((d + idx)->m_check == parent) return true;

			return false;
		}

		bool has_null(index_type parent) const
		{
			node_type const* d = m_c.data();

			index_type base = (d + parent)->m_base;
			index_type idx = base + null_value;
			index_type limit = static_cast<index_type>(m_c.size());
			
			return idx < limit && (d + idx)->m_check == parent;
		}

		/*! parentからlabelで遷移したINDEXを返す。
		- 遷移先が無ければ0を返す。
		*/
		index_type at(index_type parent, std::uint16_t label) const
		{
			node_type const* d = m_c.data();
			index_type base = (d + parent)->m_base;
			index_type idx = base + label;
			index_type check = (d + idx)->m_check;

			return (check == parent) ? idx : 0;
		}

		index_type add(index_type parent, std::uint16_t label)
		{
			return add(parent, label_vector(1, label));
		}

		/*! parentの子としてラベル集合labels内の各ラベルによる遷移を挿入する

		- 配置起点を返す。
		*/
		index_type add(index_type parent, label_vector const& labels)
		{
			assert(!labels.empty());
			assert(std::is_sorted(labels.begin(), labels.end()));

			index_type before = 0;
			index_type base = (m_c.data() + parent)->m_base; // 遷移先配置の起点（遷移先が定義されていない場合0）

			if (base == 0)
			{
				base = locate(labels, before);
				allocate(base, labels, before);
			}
			else if (is_free(parent, base, labels)) allocate(base, labels, before);
			else base = relocate(parent, base, labels);

			node_type* d = m_c.data();

			(d + parent)->m_base = base;
			for (std::uint16_t label : labels) (d + base + label)->m_check = parent;

			return base;
		}
	};

	template <typename Allocator1>
	inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, trie_base<Allocator1> const& trie)
	{
		std::uint32_t n = trie.m_size;
		for (int i = 3; 0 <= i; --i) os.put(static_cast<std::uint8_t>((n >> i * 8) & 0xFFu));

		n = trie.m_c.size();
		for (int i = 3; 0 <= i; --i) os.put(static_cast<std::uint8_t>((n >> i * 8) & 0xFFu));

		for (auto const& p : trie.m_c)
		{
			std::uint32_t base = p.m_base;
			for (int i = 3; 0 <= i; --i) os.put(static_cast<std::uint8_t>((base >> i * 8) & 0xFFu));
			std::uint32_t check = p.m_check;
			for (int i = 3; 0 <= i; --i) os.put(static_cast<std::uint8_t>((check >> i * 8) & 0xFFu));
		}

		return os;
	}

	template <typename Allocator1>
	inline std::basic_istream<char>& operator>>(std::basic_istream<char>& is, trie_base<Allocator1>& trie)
	{
		trie.m_c.clear();

		char ch;
		std::uint32_t n = 0;

		std::uint32_t base = 0;
		std::uint32_t check = 0;

		std::int32_t i = 0;
		std::uint32_t j = 0;

		for (i = 3; 0 <= i; --i)
		{
			is.get(ch);
			if (!is || is.eof()) goto Error;

			trie.m_size += static_cast<std::uint8_t>(ch) << i * 8;
		}

		for (i = 3; 0 <= i; --i)
		{
			is.get(ch);
			if (!is || is.eof()) goto Error;

			n += static_cast<std::uint8_t>(ch) << i * 8;
		}
		n *= 8;

		for (i = 0; i < n; ++i)
		{
			is.get(ch);
			if (!is || is.eof()) goto Error;

			j = i % 8;

			if (j == 0) base = check = 0;

			if (j < 4) base += static_cast<std::uint8_t>(ch) << (3 - j) * 8;
			else check += static_cast<std::uint8_t>(ch) << (7 - j) * 8;

			if (j == 7) trie.m_c.emplace_back(base, check);
		}
		goto End;

	Error:
		trie.clear();
		throw std::invalid_argument("");
	End:
		return is;
	}

	template <typename Allocator1>
	inline std::vector<std::uint8_t>&
		operator<<(std::vector<std::uint8_t>& v, trie_base<Allocator1> const& trie)
	{
		v.reserve(trie.size() * 8 + 8);

		std::uint32_t n = trie.m_size;
		for (int i = 3; 0 <= i; --i) v.push_back(static_cast<std::uint8_t>((n >> i * 8) & 0xFFu));

		n = trie.m_c.size();
		for (int i = 3; 0 <= i; --i) v.push_back(static_cast<std::uint8_t>((n >> i * 8) & 0xFFu));

		for (auto const& p : trie.m_c)
		{
			std::uint32_t base = p.m_base;
			for (int i = 3; 0 <= i; --i) v.push_back(static_cast<std::uint8_t>((base >> i * 8) & 0xFFu));
			std::uint32_t check = p.m_check;
			for (int i = 3; 0 <= i; --i) v.push_back(static_cast<std::uint8_t>((check >> i * 8) & 0xFFu));
		}

		return v;
	}

	template <typename Allocator1>
	inline std::vector<std::uint8_t>&
		operator>>(std::vector<std::uint8_t>& v, trie_base<Allocator1>& trie)
	{
		trie.m_c.clear();

		std::uint32_t n = 0;

		std::uint32_t base = 0;
		std::uint32_t check = 0;

		std::int32_t i = 0;
		std::uint32_t j = 0;

		auto it1 = v.begin();
		auto it2 = v.end();

		for (i = 3; 0 <= i; --i)
		{
			if (it1 == it2) goto Error;
			trie.m_size += *it1 << i * 8;
			++it1;
		}

		for (i = 3; 0 <= i; --i)
		{
			if (it1 == it2) goto Error;
			n += *it1 << i * 8;
			++it1;
		}
		n *= 8;

		for (i = 0; i < n; ++i)
		{
			if (it1 == it2) goto Error;
			j = i % 8;

			if (j == 0) base = check = 0;

			if (j < 4) base += *it1 << (3 - j) * 8;
			else check += *it1 << (7 - j) * 8;

			if (j == 7) trie.m_c.emplace_back(base, check);

			++it1;
		}
		goto End;

	Error:
		trie.clear();
		throw std::invalid_argument("");
	End:
		return v;
	}

	template <typename Allocator1>
	inline std::vector<std::uint32_t>&
		operator<<(std::vector<std::uint32_t>& v, trie_base<Allocator1> const& trie)
	{
		v.reserve(trie.size() * 2 + 2);
		v.push_back(trie.size());
		v.push_back(trie.m_c.size());
		for (auto n : trie.m_c)
		{
			v.push_back(n.m_base);
			v.push_back(n.m_check);
		}

		return v;
	}

	template <typename Allocator1>
	inline std::vector<std::uint32_t>&
		operator>>(std::vector<std::uint32_t>& v, trie_base<Allocator1>& trie)
	{
		trie.m_c.clear();
		trie.m_c.reserve((v.size() - 2) / 2);

		auto it1 = v.begin();
		auto it2 = v.end();

		std::uint32_t size = 0;

		if (it1 != it2) trie.m_size = *it1++;
		if (it1 != it2) size = *it1++;

		if (size * 2 + 2 != v.size())
		{
			trie.clear();
			throw std::invalid_argument("");
		}

		while (it1 != it2)
		{
			std::int32_t base = *it1++;
			std::int32_t check = *it1++;
			trie.m_c.push_back(trie_node{ base, check });
		}

		return v;
	}
}
