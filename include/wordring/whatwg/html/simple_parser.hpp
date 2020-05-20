﻿#pragma once

// https://html.spec.whatwg.org/multipage/parsing.html
// https://triple-underscore.github.io/HTML-parsing-ja.html

#include <wordring/whatwg/html/simple_policy.hpp>

//#include <wordring/whatwg/encoding/encoding.hpp>

#include <wordring/whatwg/html/parsing/token.hpp>
//#include <wordring/whatwg/html/parsing/atom_tbl.hpp>
//#include <wordring/whatwg/html/parsing/input_stream.hpp>
#include <wordring/whatwg/html/parsing/parser_defs.hpp>
//#include <wordring/whatwg/html/parsing/tokenization.hpp>
#include <wordring/whatwg/html/parsing/tree_construction_dispatcher.hpp>

#include <cassert>
#include <iterator>
#include <variant>

namespace wordring::whatwg::html
{
	template <typename T, typename String, typename Container, bool IsFragmentsParser = false>
	class simple_parser : public parsing::tree_construction_dispatcher<T, simple_policy<String, Container, IsFragmentsParser>>
	{
	public:
		using policy = simple_policy<String, Container, IsFragmentsParser>;

		using base_type = parsing::tree_construction_dispatcher<T, policy>;
		using this_type = T;

		using string_type  = typename policy::string_type;
		using container    = typename policy::container_type;
		using node_pointer = typename container::iterator;
		using node_type    = typename policy::node_type;

		using document_type               = typename policy::document_type;
		using document_type_type          = typename policy::document_type_type;
		using document_fragment_type      = typename policy::document_fragment_type;
		using element_type                = typename policy::element_type;
		using text_type                   = typename policy::text_type;
		using processing_instruction_type = typename policy::processing_instruction_type;
		using comment_type                = typename policy::comment_type;

		using attribute_type    = typename policy::attribute_type;
		using attribute_pointer = typename policy::attribute_pointer;

		using namespace_uri_type = basic_html_atom<string_type, ns_name>;
		using lacal_name_type    = basic_html_atom<string_type, tag_name>;

		using base_type::m_stack;

		using base_type::report_error;
		using base_type::eof;

		using typename base_type::mode_name;

	public:
		simple_parser()
			: m_script_nesting_level(0)
			, m_parser_pause_flag(false)
		{
			m_document  = m_c.insert(m_c.end(), document_type());
			m_temporary = m_c.insert(m_c.end(), node_type());
		}

		// ----------------------------------------------------------------------------------------
		// 木
		// ----------------------------------------------------------------------------------------

		node_pointer temp()
		{

		}
		// ----------------------------------------------------------------------------------------
		// ノード
		// ----------------------------------------------------------------------------------------

		//bool is_document(node_pointer it) const { return std::holds_alternative<document_type>(*it); }

		//bool is_document_type(node_pointer it) const { return std::holds_alternative<document_type_type>(*it); }

		//bool is_document_fragment(node_pointer it) const { return std::holds_alternative<document_fragment_type>(*it); }

		//bool is_element(node_pointer it) const { return std::holds_alternative<element_type>(*it); }

		bool is_text(node_pointer it) const { return std::holds_alternative<text_type>(*it); }

		//bool is_processing_instruction(node_pointer it) const { return std::holds_alternative<processing_instruction_type>(*it); }

		//bool is_comment(node_pointer it) const { return std::holds_alternative<comment_type>(*it); }

		//document_type* to_document(node_pointer it) { return std::get_if<document_type>(std::addressof(*it)); }

		//document_type_type const* to_document_type(node_pointer it) const { return std::get_if<document_type_type>(std::addressof(*it)); }

		//document_fragment_type const* to_document_fragment(node_pointer it) const { return std::get_if<document_fragment_type>(std::addressof(*it)); }

		//element_type const* to_element(node_pointer it) const { return std::get_if<element_type>(std::addressof(*it)); }

		//text_type* to_text(node_pointer it) { return std::get_if<text_type>(std::addressof(*it)); }

		//processing_instruction_type const* to_processing_instruction(node_pointer it) const { return std::get_if<processing_instruction_type>(std::addressof(*it)); }

		//comment_type const* to_comment(node_pointer it) const { return std::get_if<comment_type>(std::addressof(*it)); }

		/*! @brief ノードを挿入する
		*/
		/*
		template <typename Node>
		node_pointer insert_(node_pointer pos, Node node)
		{
			return m_c.insert(pos, std::forward<Node>(node));
		}
		*/
		// ----------------------------------------------------------------------------------------
		// 文書
		// ----------------------------------------------------------------------------------------

		/*! @brief 文書ノードを返す

		パーサ構築時に文書ノードが挿入される。
		*/
		node_pointer document() { return m_document; }

		/*! @brief 文書が IFRAME ソース文書か調べる

		https://html.spec.whatwg.org/multipage/iframe-embed-object.html#an-iframe-srcdoc-document
		*/
		bool is_iframe_srcdoc_document() const { return false; }

		void set_document_type(document_type_name type)
		{
			std::get_if<document_type>(std::addressof(*document()))->document_type(type);
		}

		document_mode_name get_document_mode() const
		{
			return std::get_if<document_type>(std::addressof(*m_document))->document_mode();
		}

		void set_document_mode(document_mode_name mode)
		{
			std::get_if<document_type>(std::addressof(*document()))->document_mode(mode);
		}

		/*! 文書の準備度を設定する
		
		https://html.spec.whatwg.org/multipage/dom.html#current-document-readiness
		*/
		void set_current_document_readiness(std::u32string const& rediness)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 文書型
		// ----------------------------------------------------------------------------------------

		document_type_type create_document_type(std::u32string const& name, std::u32string const& public_id, std::u32string const& system_id)
		{
			document_type_type doctype;

			doctype.name(encoding_cast<string_type>(name));
			doctype.public_id(encoding_cast<string_type>(public_id));
			doctype.system_id(encoding_cast<string_type>(system_id));

			return doctype;
		}

		node_pointer insert_document_type(node_pointer pos, document_type_type&& doctype)
		{
			return m_c.insert(pos, std::move(doctype));
		}

		// ----------------------------------------------------------------------------------------
		// 要素
		// ----------------------------------------------------------------------------------------

		/*! @brief NULLで初期化された空のポインタを作成する
		*/
		node_pointer create_pointer() const { return node_pointer(); }

		/*! @brief 親要素を返す

		根に親は無い。
		この場合、create_pointer() が返すものと同一のポインタを返す。
		*/
		node_pointer parent(node_pointer it) { return it.parent(); }

		/*! @brief 最初の子を返す
		*/
		node_pointer begin(node_pointer it) { return it.begin(); }

		/*! @brief 最後の子の次を返す
		*/
		node_pointer end(node_pointer it) { return it.end(); }

		/*! @brief 前の兄弟を返す
		*/
		node_pointer prev(node_pointer it) { return std::prev(it); }

		/*! @brief 次の兄弟を返す
		*/
		node_pointer next(node_pointer it) { return std::next(it); }

		/* @brief 要素を作成する

		- https://dom.spec.whatwg.org/#concept-create-element に対応する要素作成関数
		- https://triple-underscore.github.io/DOM4-ja.html#concept-create-element
		*/
		node_pointer create_element(node_pointer doc, std::u32string name, namespace_uri_type ns, std::u32string prefix)
		{
			element_type el(ns, encoding_cast<string_type>(prefix), encoding_cast<string_type>(name));
			return m_c.insert(m_temporary.end(), std::move(el));
		}

		node_pointer create_element(node_pointer doc, tag_name name, namespace_uri_type ns, std::u32string prefix)
		{
			element_type el(ns, encoding_cast<string_type>(prefix), name);

			return m_c.insert(m_temporary.end(), std::move(el));
		}

		node_pointer create_element(node_pointer doc, tag_name name, ns_name ns = ns_name::HTML)
		{
			element_type el(ns, string_type(), name);

			return m_c.insert(m_temporary.end(), std::move(el));
		}

		node_pointer insert_element(node_pointer pos, node_pointer it)
		{
			return m_c.move(pos, it);
		}

		/*! @brief ノードを削除する
		*/
		void erase_element(node_pointer it)
		{
			m_c.erase(it);
		}

		/*! @brief ノードを移動する
		*/
		void move_element(node_pointer pos, node_pointer it)
		{
			m_c.move(pos, it);
		}

		/*! @brief 要素へオーナー文書を設定する

		パーサーは呼び出すが、simple node側は実装していないので、何もしない。
		*/
		void set_document(node_pointer it, node_pointer doc) {}

		/*! @brief script 要素の "non-blocking" flag を設定する

		パーサーは呼び出すが、simple node側は実装していないので、何もしない。
		*/
		void set_non_blocking_flag(node_pointer it, bool b) {}

		/*! @brief script 要素の "already started" flag を設定する

		パーサーは呼び出すが、simple node側は実装していないので、何もしない。
		*/
		void set_already_started_flag(node_pointer it, bool b) {}

		/*! @brief 要素の名前空間を返す
		*/
		ns_name get_namespace_uri_id(node_pointer it) const
		{
			return std::get_if<element_type>(std::addressof(*it))->namespace_uri_id();
		}

		/*! @brief 要素のローカル名を返す
		*/
		tag_name get_local_name_id(node_pointer it) const
		{
			return std::get_if<element_type>(std::addressof(*it))->local_name_id();
		}

		std::u32string get_local_name(node_pointer it) const
		{
			return encoding_cast<std::u32string>(std::get_if<element_type>(std::addressof(*it))->local_name());
		}

		/*! @brief 二つの要素が同じシグネチャを持つか調べる

		push_active_formatting_element_list() から呼び出される。
		*/
		bool equals(node_pointer lhs, node_pointer rhs) const
		{
			return *lhs == *rhs;
		}

		// ----------------------------------------------------------------------------------------
		// 属性
		// ----------------------------------------------------------------------------------------

		attribute_type create_attribute(node_pointer el, ns_name ns, std::u32string const& prefix, std::u32string const& local_name)
		{
			return attribute_type(ns, encoding_cast<string_type>(prefix), encoding_cast<string_type>(local_name));
		}

		/*! @brief 要素へ属性を付加する
		*/
		void append_attribute(node_pointer it, attribute_type&& attr)
		{
			push_back(*it, std::move(attr));
		}

		void append_attribute(node_pointer it, ns_name ns, std::u32string const& prefix, std::u32string const& name, std::u32string const& value)
		{
		}

		bool contains(node_pointer it, ns_name ns, std::u32string const& prefix, std::u32string const& name)
		{
			return find(*it, ns, encoding_cast<string_type>(prefix), encoding_cast<string_type>(name)) != wordring::whatwg::html::end(*it);
		}

		/*
		attribute_pointer null_attribute(node_pointer it) const
		{
			return end(*it);
		}

		attribute_pointer find_attribute(node_pointer it, attribute_name name) const
		{
			return find(*it, name);
		}
		*/

		// ----------------------------------------------------------------------------------------
		// テキスト
		// ----------------------------------------------------------------------------------------

		text_type create_text(char32_t cp)
		{
			text_type text;
			to_string(cp, std::back_inserter(text));
			return text;
		}

		void append_text(node_pointer it, char32_t cp)
		{
			to_string(cp, std::back_inserter(*std::get_if<text_type>(std::addressof(*it))));
		}

		node_pointer insert_text(node_pointer pos, text_type&& text)
		{
			return m_c.insert(pos, std::move(text));
		}

		// ----------------------------------------------------------------------------------------
		// コメント
		// ----------------------------------------------------------------------------------------

		comment_type create_comment(std::u32string data)
		{
			return comment_type(encoding_cast<string_type>(data));
		}

		node_pointer insert_comment(node_pointer pos, comment_type&& comment)
		{
			return m_c.insert(pos, std::move(comment));
		}

		// ----------------------------------------------------------------------------------------
		// ノード
		// ----------------------------------------------------------------------------------------

		/*! @brief NULLで初期化された空のポインタを作成する
		*/
		node_pointer create_pointer_() const
		{
			return node_pointer();
		}

		// ノード作成 ----------------------------------------------------------


		document_fragment_type create_document_fragment() {}


		processing_instruction_type create_processing_instruction() {}



		// 


		/*void insert_comment(node_pointer pos, comment_token& comment)
		{
			m_c.insert(pos, comment_type(comment.m_data));
		}*/

		/*! @brief 与えられたノードがHTML名前空間に属するか調べる
		*/
		/*bool in_html_namespace(node_pointer it) const
		{
			if (std::holds_alternative<element_type>(*it))
			{
				element_type const& elm = std::get<element_type>(*it);
				return elm.namespace_uri_atom() == ns_name::HTML;
			}

			return false;
		}*/


		//void change_encoding(wordring::whatwg::encoding::name name) {}

		void on_report_error(parsing::error_name e) {}

	protected:

		//encoding_confidence m_confidence;

		std::uint32_t m_script_nesting_level;
		bool m_parser_pause_flag;

		container    m_c;
		node_pointer m_document;
		node_pointer m_temporary;
	};
}
