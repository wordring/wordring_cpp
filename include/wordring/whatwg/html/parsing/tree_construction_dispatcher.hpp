﻿#pragma once

// https://html.spec.whatwg.org/multipage/parsing.html
// https://triple-underscore.github.io/HTML-parsing-ja.html

#include <wordring/whatwg/html/parsing/parser_defs.hpp>
#include <wordring/whatwg/html/parsing/tokenization.hpp>

#include <wordring/whatwg/html/dom_defs.hpp>

#include <wordring/whatwg/infra/infra.hpp>
#include <wordring/whatwg/infra/unicode.hpp>

#include <wordring/whatwg/encoding/encoding.hpp>

#include <algorithm>
#include <deque>
#include <string>
#include <string_view>

namespace wordring::whatwg::html::parsing
{
	template <typename T, typename NodePolicy>
	class tree_construction_dispatcher : public tokenizer<T, NodePolicy>
	{
		friend tokenizer<T, NodePolicy>;

	protected:
		using base_type = tokenizer<T, NodePolicy>;
		using this_type = T;

		using policy = NodePolicy;

		using string_type    = typename policy::string_type;
		using container_type = typename policy::container_type;
		using node_type      = typename policy::node_type;
		using node_pointer   = typename container_type::iterator;

		using document_node_type               = typename policy::document_node_type;
		using document_type_node_type          = typename policy::document_type_node_type;
		using document_fragment_node_type      = typename policy::document_fragment_node_type;
		using element_node_type                = typename policy::element_node_type;
		using text_node_type                   = typename policy::text_node_type;
		using processing_instruction_node_type = typename policy::processing_instruction_node_type;
		using comment_node_type                = typename policy::comment_node_type;

	public:
		//using state_type = base_type::state_type;

		using base_type::report_error;
		using base_type::eof;

	protected:
		// ----------------------------------------------------------------------------------------
		// 入力バイトストリーム
		//
		// 12.2.3 The input byte stream
		// https://html.spec.whatwg.org/multipage/parsing.html#the-input-byte-stream
		// ----------------------------------------------------------------------------------------

		encoding_confidence_name m_encoding_confidence;

		// ----------------------------------------------------------------------------------------
		// 挿入モード
		//
		// 12.2.4.1 The insertion mode
		// https://html.spec.whatwg.org/multipage/parsing.html#the-insertion-mode
		// ----------------------------------------------------------------------------------------

		using mode_type  = void(tree_construction_dispatcher::*)();
		using scope_type = bool(tree_construction_dispatcher::*)(node_pointer) const;

		/*! @brief 挿入モード
		- https://html.spec.whatwg.org/multipage/parsing.html#insertion-mode
		*/
		enum class mode_name : std::uint32_t
		{
			initial_insertion_mode,
			before_html_insertion_mode,
			before_head_insertion_mode,
			in_head_insertion_mode,
			in_head_noscript_insertion_mode,
			after_head_insertion_mode,
			in_body_insertion_mode,
			text_insertion_mode,
			in_table_insertion_mode,
			in_table_text_insertion_mode,
			in_caption_insertion_mode,
			in_column_group_insertion_mode,
			in_table_body_insertion_mode,
			in_row_insertion_mode,
			in_cell_insertion_mode,
			in_select_insertion_mode,
			in_select_in_table_insertion_mode,
			in_template_insertion_mode,
			after_body_insertion_mode,
			in_frameset_insertion_mode,
			after_frameset_insertion_mode,
			after_after_body_insertion_mode,
			after_after_frameset_insertion_mode,
		};

		mode_name m_insertion_mode;
		mode_name m_original_insertion_mode;
		std::deque<mode_name> m_template_insertion_mode_stack;

		// ----------------------------------------------------------------------------------------
		// スタック
		//
		// 12.2.4.2 The stack of open elements
		// https://html.spec.whatwg.org/multipage/parsing.html#the-stack-of-open-elements
		// ----------------------------------------------------------------------------------------

		using stack_entry_type = open_element_stack_entry<policy>;
		using stack_pointer = typename open_element_stack<policy>::iterator;

		open_element_stack<policy> m_open_element_stack;

		// ----------------------------------------------------------------------------------------
		// アクティブ整形要素のリスト
		//
		// 12.2.4.3 The list of active formatting elements
		// https://html.spec.whatwg.org/multipage/parsing.html#the-list-of-active-formatting-elements
		// ----------------------------------------------------------------------------------------

		struct active_formatting_element
		{
			node_pointer    m_it;
			start_tag_token m_token;
			bool            m_marker;
		};

		/*! @brief アクティブ整形要素のリスト
		
		逆順の検索が多いため、 front() へ新しい項目を追加していく。
		*/
		std::deque<active_formatting_element> m_active_formatting_element_list;

		// ----------------------------------------------------------------------------------------
		// 要素ポインタ
		//
		// 12.2.4.4 The element pointers
		// https://html.spec.whatwg.org/multipage/parsing.html#the-element-pointers
		// ----------------------------------------------------------------------------------------

		node_pointer m_head_element_pointer;
		node_pointer m_form_element_pointer;

		// ----------------------------------------------------------------------------------------
		// 他のパーサ状態フラグ
		//
		// 12.2.4.5 Other parsing state flags
		// https://html.spec.whatwg.org/multipage/parsing.html#other-parsing-state-flags
		// ----------------------------------------------------------------------------------------

		bool m_scripting_flag;
		bool m_frameset_ok_flag;

		// ----------------------------------------------------------------------------------------
		// ノードの作成と挿入
		//
		// 12.2.6.1 Creating and inserting nodes
		// https://html.spec.whatwg.org/multipage/parsing.html#creating-and-inserting-nodes
		// ----------------------------------------------------------------------------------------

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#foster-parent
		*/
		bool m_foster_parenting;

		// ----------------------------------------------------------------------------------------
		// HTML フラグメント
		//
		// 12.4 Parsing HTML fragments
		// https://html.spec.whatwg.org/multipage/parsing.html#parsing-html-fragments
		// ----------------------------------------------------------------------------------------

		static bool constexpr is_fragments_parser = policy::is_fragments_parser;

		stack_entry_type m_context_entry;



		// 
		bool m_omit_lf;

	protected:
		tree_construction_dispatcher()
			: m_encoding_confidence(encoding_confidence_name::irrelevant)
			, m_insertion_mode(mode_name::initial_insertion_mode)
			, m_original_insertion_mode(static_cast<mode_name>(0))
			, m_scripting_flag(false)
			, m_frameset_ok_flag(true)
			, m_foster_parenting(false)
			, m_omit_lf(false)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 挿入モード
		//
		// 12.2.4.1 The insertion mode
		// https://html.spec.whatwg.org/multipage/parsing.html#the-insertion-mode
		// ----------------------------------------------------------------------------------------

		void insertion_mode(mode_name mode)
		{
			m_insertion_mode = mode;
		}

		mode_name current_template_insertion_mode() const
		{
			return m_template_insertion_mode_stack.back();
		}

		void reset_insertion_mode_appropriately()
		{
			assert(!m_open_element_stack.empty());

			this_type const* P = static_cast<this_type const*>(this);

			bool last = false;
			auto it = --m_open_element_stack.end();
			node_pointer node = it->m_it;

			tag_name tag;

		Loop1:
			if (it == m_open_element_stack.begin())
			{
				last = true;
				if (is_fragments_parser) node = m_context_entry.m_it;
			}

			tag = P->local_name_name(node);

			if (tag == tag_name::Select)
			{
				node_pointer ancestor;

				if (last) goto Done;
				ancestor = node;
			Loop2:
				if (it == m_open_element_stack.begin()) goto Done;
				
				--it;
				ancestor = it->m_it;

				tag = P->local_name_name(ancestor);
				if (tag == tag_name::Template) goto Done;

				if (tag == tag_name::Table)
				{
					insertion_mode(mode_name::in_select_in_table_insertion_mode);
					return;
				}

				goto Loop2;
			Done:
				insertion_mode(mode_name::in_select_insertion_mode);
				return;
			}

			switch (tag)
			{
			case tag_name::Td: case tag_name::Th:
				if (last == false) return insertion_mode(mode_name::in_cell_insertion_mode);
				break;
			case tag_name::Tr:
				return insertion_mode(mode_name::in_row_insertion_mode);
			case tag_name::Tbody: case tag_name::Thead: case tag_name::Tfoot:
				return insertion_mode(mode_name::in_cell_insertion_mode);
			case tag_name::Caption:
				return insertion_mode(mode_name::in_caption_insertion_mode);
			case tag_name::Colgroup:
				return insertion_mode(mode_name::in_column_group_insertion_mode);
			case tag_name::Table:
				return insertion_mode(mode_name::in_table_insertion_mode);
			case tag_name::Template:
				return insertion_mode(current_template_insertion_mode());
			case tag_name::Head:
				if(last == false) return insertion_mode(mode_name::in_head_insertion_mode);
				break;
			case tag_name::Body:
				return insertion_mode(mode_name::in_body_insertion_mode);
			case tag_name::Frameset:
				return insertion_mode(mode_name::in_frameset_insertion_mode);
			case tag_name::Html:
				if(m_head_element_pointer == node_pointer()) return insertion_mode(mode_name::before_head_insertion_mode);
				else return insertion_mode(mode_name::after_head_insertion_mode);
			default:
				break;
			}

			if (last == true) return insertion_mode(mode_name::in_body_insertion_mode);
			
			--it;
			node = it->m_it;

			goto Loop1;
		}

		// ----------------------------------------------------------------------------------------
		// スタック
		//
		// 12.2.4.2 The stack of open elements
		// https://html.spec.whatwg.org/multipage/parsing.html#the-stack-of-open-elements
		// ----------------------------------------------------------------------------------------

		stack_entry_type& current_node() { return m_open_element_stack.back(); }

		stack_entry_type const& current_node() const { return m_open_element_stack.back(); }

		stack_entry_type& adjusted_current_node()
		{
			if constexpr (is_fragments_parser) { if (m_open_element_stack.size() == 1) return m_context_entry; }
			return current_node();
		}

		stack_entry_type const& adjusted_current_node() const
		{
			if constexpr (is_fragments_parser) { if (m_open_element_stack.size() == 1) return m_context_entry; }
			return current_node();
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#special
		*/
		bool is_special(node_pointer it) const
		{
			this_type const* P = static_cast<this_type const*>(this);

			ns_name  ns  = P->namespace_uri_name(it);
			tag_name tag = P->local_name_name(it);

			if (ns == ns_name::HTML)
			{
				switch (tag)
				{
				case tag_name::Address:    case tag_name::Applet:  case tag_name::Area:   case tag_name::Article: case tag_name::Aside:     case tag_name::Base:     case tag_name::Basefont: case tag_name::Bgsound:
				case tag_name::Blockquote: case tag_name::Body:    case tag_name::Br:     case tag_name::Button:  case tag_name::Caption:   case tag_name::Center:   case tag_name::Col:      case tag_name::Colgroup:
				case tag_name::Dd:         case tag_name::Details: case tag_name::Dir:    case tag_name::Div:     case tag_name::Dl:        case tag_name::Dt:       case tag_name::Embed:    case tag_name::Fieldset:
				case tag_name::Figcaption: case tag_name::Figure:  case tag_name::Footer: case tag_name::Form:    case tag_name::Frame:     case tag_name::Frameset: case tag_name::H1:       case tag_name::H2:
				case tag_name::H3:         case tag_name::H4:      case tag_name::H5:     case tag_name::H6:      case tag_name::Head:      case tag_name::Header:   case tag_name::Hgroup:   case tag_name::Hr:
				case tag_name::Html:       case tag_name::Iframe:  case tag_name::Img:    case tag_name::Input:   case tag_name::Keygen:    case tag_name::Li:       case tag_name::Link:     case tag_name::Listing:
				case tag_name::Main:       case tag_name::Marquee: case tag_name::Menu:   case tag_name::Meta:    case tag_name::Nav:       case tag_name::Noembed:  case tag_name::Noframes: case tag_name::Noscript:
				case tag_name::Object:     case tag_name::Ol:      case tag_name::P:      case tag_name::Param:   case tag_name::Plaintext: case tag_name::Pre:      case tag_name::Script:   case tag_name::Section:
				case tag_name::Select:     case tag_name::Source:  case tag_name::Style:  case tag_name::Summary: case tag_name::Table:     case tag_name::Tbody:    case tag_name::Td:       case tag_name::Template:
				case tag_name::Textarea:   case tag_name::Tfoot:   case tag_name::Th:     case tag_name::Thead:   case tag_name::Title:     case tag_name::Tr:       case tag_name::Track:    case tag_name::Ul:
				case tag_name::Wbr:        case tag_name::Xmp:
					return true;
				default:
					break;
				}
			}
			else if (ns == ns_name::MathML)
			{
				switch (tag)
				{
				case tag_name::Mi: case tag_name::Mo: case tag_name::Mn: case tag_name::Ms: case tag_name::Mtext: case tag_name::Annotation_xml:
					return true;
				default:
					break;
				}
			}
			else if(ns == ns_name::SVG)
			{
				switch (tag)
				{
				case tag_name::ForeignObject: case tag_name::Desc: case tag_name::Title:
					return true;
				default:
					break;
				}
			}

			return false;
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#formatting
		*/
		bool is_formatting(node_pointer it) const
		{
			this_type const* P = static_cast<this_type const*>(this);

			if (P->namespace_uri_name(it) == ns_name::HTML)
			{
				switch (P->local_name_name(it))
				{
				case tag_name::A: case tag_name::B:     case tag_name::Big:    case tag_name::Code:   case tag_name::Em: case tag_name::Font: case tag_name::I: case tag_name::Nobr:
				case tag_name::S: case tag_name::Small: case tag_name::Strike: case tag_name::Strong: case tag_name::Tt: case tag_name::U:
					return true;
				default:
					return false;
				}
			}

			return false;
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-the-specific-scope
		*/
		template <typename Scope, typename Target>
		bool in_specific_scope(Scope s, Target target) const
		{
			this_type const* P = static_cast<this_type const*>(this);

			auto it1 = m_open_element_stack.rbegin();
			auto it2 = m_open_element_stack.rend();

			while (it1 != it2)
			{
				auto it = it1->m_it;
				// ターゲット確認
				if constexpr (std::is_same_v<Target, std::pair<ns_name, tag_name>>)
				{
					ns_name  ns = P->namespace_uri_name(it);
					tag_name t = P->local_name_name(it);

					if (ns == target.first && t == target.second) return true;
				}
				else if constexpr (std::is_same_v<Target, node_pointer>)
				{
					if (it == target) return true;
				}
				else
				{
					ns_name  ns = P->namespace_uri_name(it);
					tag_name t =  P->local_name_name(it);

					for (tag_name tag : target) if (ns == ns_name::HTML && t == tag) return true;
				}

				// スコープ確認
				if ((this->*s)(it)) return false;
				++it1;
			}

			assert(false);
			return false;
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-scope
		*/
		bool is_default_scope(node_pointer it) const
		{
			this_type const* P = static_cast<this_type const*>(this);
			ns_name  ns  = P->namespace_uri_name(it);
			tag_name tag = P->local_name_name(it);

			if (ns == ns_name::HTML)
			{
				switch (tag)
				{
				case tag_name::Applet:   case tag_name::Caption: case tag_name::Html: case tag_name::Table: case tag_name::Td: case tag_name::Th: case tag_name::Marquee: case tag_name::Object:
				case tag_name::Template:
					return true;
				default:
					return false;
				}
			}
			else if (ns == ns_name::MathML)
			{
				switch (tag)
				{
				case tag_name::Mi: case tag_name::Mo: case tag_name::Mn: case tag_name::Ms: case tag_name::Mtext: case tag_name::Annotation_xml:
					return true;
				default:
					return false;
				}
			}
			else if (ns == ns_name::SVG)
			{
				switch (tag)
				{
				case tag_name::Factorial: case tag_name::ForeignObject: case tag_name::Desc: case tag_name::Title:
					return true;
				default:
					return false;
				}
			}

			return false;
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-list-item-scope
		*/
		bool is_list_item_scope(node_pointer it) const
		{
			if (is_default_scope(it)) return true;

			this_type const* P = static_cast<this_type const*>(this);
			ns_name  ns  = P->namespace_uri_name(it);
			tag_name tag = P->local_name_name(it);

			if (ns == ns_name::HTML && (tag == tag_name::Ol || tag == tag_name::Ul)) return true;

			return false;
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-button-scope
		*/
		bool is_button_scope(node_pointer it) const
		{
			if (is_default_scope(it)) return true;

			this_type const* P = static_cast<this_type const*>(this);
			ns_name  ns = P->namespace_uri_name(it);
			tag_name tag = P->local_name_name(it);

			if (ns == ns_name::HTML && tag == tag_name::Button) return true;

			return false;
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-table-scope
		*/
		bool is_table_scope(node_pointer it) const
		{
			this_type const* P = static_cast<this_type const*>(this);
			ns_name  ns = P->namespace_uri_name(it);
			tag_name tag = P->local_name_name(it);

			if (ns == ns_name::HTML && (tag == tag_name::Html || tag == tag_name::Table || tag == tag_name::Template)) return true;

			return false;
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#has-an-element-in-select-scope
		*/
		bool is_select_scope(node_pointer it) const
		{
			this_type const* P = static_cast<this_type const*>(this);
			ns_name  ns = P->namespace_uri_name(it);
			tag_name tag = P->local_name_name(it);

			if (ns == ns_name::HTML && (tag == tag_name::Optgroup || tag == tag_name::Option)) return false;

			return true;
		}

		static scope_type constexpr default_scope   = &tree_construction_dispatcher::is_default_scope;
		static scope_type constexpr list_item_scope = &tree_construction_dispatcher::is_list_item_scope;
		static scope_type constexpr button_scope    = &tree_construction_dispatcher::is_button_scope;
		static scope_type constexpr table_scope     = &tree_construction_dispatcher::is_table_scope;
		static scope_type constexpr select_scope    = &tree_construction_dispatcher::is_select_scope;

		/*
		https://triple-underscore.github.io/HTML-parsing-ja.html#_pop-until
		*/
		void pop_until(tag_name condition)
		{
			this_type* P = static_cast<this_type*>(this);

			while (!m_open_element_stack.empty())
			{
				tag_name tag = P->local_name_name(m_open_element_stack.back().m_it);
				m_open_element_stack.pop_back();
				if (tag == condition) break;
			}
		}

		template <typename T>
		void pop_until(T conditions)
		{
			this_type* P = static_cast<this_type*>(this);

			while (!m_open_element_stack.empty())
			{
				tag_name tag = P->local_name_name(m_open_element_stack.back().m_it);
				m_open_element_stack.pop_back();
				if (std::find_if(std::begin(conditions), std::end(conditions), [&tag](tag_name c)->bool {
						return tag == c; }) != std::end(conditions)) break;
			}
		}

		void pop_until(node_pointer it)
		{
			while (!m_open_element_stack.empty())
			{
				node_pointer p = m_open_element_stack.back().m_it;
				m_open_element_stack.pop_back();
				if (p == it) break;
			}
		}

		// ----------------------------------------------------------------------------------------
		// 整形要素のリスト
		// 
		// 12.2.4.3 The list of active formatting elements
		// https://html.spec.whatwg.org/multipage/parsing.html#the-list-of-active-formatting-elements
		// ----------------------------------------------------------------------------------------

		/*! @brief アクティブ整形要素リストへ整形要素を挿入する

		@todo
			要素が作成されたときと同じ比較とは、ケース・インセンシティブ、名前空間接頭辞の分離前の比較を意味するかもしれない。
			あるいは、実際にトークンと比較するのかもしれない。
			この実装は、スクリプトをサポートしないため、トークンと要素の属性は違いが無い。
			したがって、要素同士を比較している。
		- https://html.spec.whatwg.org/multipage/parsing.html#the-list-of-active-formatting-elements
		*/
		void push_active_formatting_element_list(node_pointer it, start_tag_token const& token)
		{
			this_type* P = static_cast<this_type*>(this);

			std::uint32_t n = 0;
			auto pos = m_active_formatting_element_list.end();

			auto it1 = m_active_formatting_element_list.begin();
			auto it2 = m_active_formatting_element_list.end();
			while (it1 != it2)
			{
				if (it1->m_marker) break;
				if (P->equals(it, it1->m_it))
				{
					pos = it1;
					++n;
				}

				++it1;
			}

			if (3 <= n) m_active_formatting_element_list.erase(pos);

			m_active_formatting_element_list.push_front({ it, token, false });
		}

		/*! @brief アクティブ整形要素リストへマーカーを挿入する

		https://html.spec.whatwg.org/multipage/parsing.html#the-list-of-active-formatting-elements
		*/
		void push_active_formatting_element_list()
		{
			m_active_formatting_element_list.push_front({ node_pointer(), start_tag_token(), true });
		}

		/*! @brief アクティブ整形要素リストを再構築する

		https://html.spec.whatwg.org/multipage/parsing.html#reconstruct-the-active-formatting-elements
		*/
		void reconstruct_active_formatting_element_list()
		{
			auto it1 = m_active_formatting_element_list.begin();
			auto it2 = m_active_formatting_element_list.end();

			if (it1 == it2) return;

			if (it1->m_marker) return;
			for (auto const& entry : m_open_element_stack) if (it1->m_it == entry.m_it) return;

		Rewind:
			if (std::distance(it1, it2) <= 1) goto Create;
			
			++it1;

			if (!it1->m_marker)
			{
				if(std::find_if(m_open_element_stack.begin(), m_open_element_stack.end(), [&it1](auto const& entry)->bool {
					return it1->m_it == entry.m_it; }) == m_open_element_stack.end()) goto Rewind;
			}

		Advance:
			--it1;

		Create:
			auto it = insert_html_element(it1->m_token);

			it1->m_it = it;

			if (it1 != m_active_formatting_element_list.begin()) goto Advance;
		}

		/*! @brief アクティブ整形要素リストをマーカーまでクリアする
		
		https://html.spec.whatwg.org/multipage/parsing.html#clear-the-list-of-active-formatting-elements-up-to-the-last-marker
		*/
		void clear_active_formatting_element_list()
		{
			while (!m_active_formatting_element_list.empty())
			{
				bool marker = m_active_formatting_element_list.front().m_marker;
				m_active_formatting_element_list.pop_front();
				if (marker) break;
			}
		}

		// ----------------------------------------------------------------------------------------
		// ツリー構築
		//
		// 12.2.6 Tree construction
		// https://html.spec.whatwg.org/multipage/parsing.html#tree-construction
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void process_token(mode_name mode, Token& token)
		{
			this_type* P = static_cast<this_type*>(this);

			if constexpr (std::is_same_v<end_tag_token, Token>)
			{
				// https://html.spec.whatwg.org/multipage/parsing.html#tokenization

				if (!token.m_attributes.empty()) report_error(error_name::end_tag_with_attributes);
				if (token.m_self_closing_flag) report_error(error_name::end_tag_with_trailing_solidus);
			}

			if (m_open_element_stack.empty()) goto Html;
			else
			{
				stack_entry_type& entry = adjusted_current_node();

				if (P->namespace_uri_name(entry.m_it) == ns_name::HTML) goto Html;
				if constexpr (std::is_same_v<start_tag_token, Token>)
				{
					if (is_mathml_text_integration_point(entry)
						&& token.m_tag_name_id != tag_name::Mglyph
						&& token.m_tag_name_id != tag_name::Malignmark) goto Html;

					if (P->local_name_name(entry.m_it) == tag_name::Annotation_xml
						&& token.m_tag_name_id == tag_name::Svg) goto Html;

					if (is_html_integration_point(entry)) goto Html;
				}
				if constexpr (std::is_same_v<character_token, Token>)
				{
					if (is_mathml_text_integration_point(entry)) goto Html;
					if (is_html_integration_point(entry)) goto Html;
				}
				if constexpr (std::is_same_v<end_of_file_token, Token>) goto Html;
			}

			on_foreign_content(token);
			goto Finish;

		Html:
			switch (mode)
			{
			case mode_name::initial_insertion_mode:
				on_initial_insertion_mode(token);
				break;
			case mode_name::before_html_insertion_mode:
				on_before_html_insertion_mode(token);
				break;
			case mode_name::before_head_insertion_mode:
				on_before_head_insertion_mode(token);
				break;
			case mode_name::in_head_insertion_mode:
				on_in_head_insertion_mode(token);
				break;
			case mode_name::in_head_noscript_insertion_mode:
				on_in_head_noscript_insertion_mode(token);
				break;
			case mode_name::after_head_insertion_mode:
				on_after_head_insertion_mode(token);
				break;
			case mode_name::in_body_insertion_mode:
				on_in_body_insertion_mode(token);
				break;
			case mode_name::text_insertion_mode:
				on_text_insertion_mode(token);
				break;
			case mode_name::in_table_insertion_mode:
				on_in_table_insertion_mode(token);
				break;
			case mode_name::in_table_text_insertion_mode:
				on_in_table_text_insertion_mode(token);
				break;
			case mode_name::in_caption_insertion_mode:
				on_in_caption_insertion_mode(token);
				break;
			case mode_name::in_column_group_insertion_mode:
				on_in_column_group_insertion_mode(token);
				break;
			case mode_name::in_table_body_insertion_mode:
				on_in_table_body_insertion_mode(token);
				break;
			case mode_name::in_row_insertion_mode:
				on_in_row_insertion_mode(token);
				break;
			case mode_name::in_cell_insertion_mode:
				on_in_cell_insertion_mode(token);
				break;
			case mode_name::in_select_insertion_mode:
				on_in_select_insertion_mode(token);
				break;
			case mode_name::in_select_in_table_insertion_mode:
				on_in_select_in_table_insertion_mode(token);
				break;
			case mode_name::in_template_insertion_mode:
				on_in_template_insertion_mode(token);
				break;
			case mode_name::after_body_insertion_mode:
				on_after_body_insertion_mode(token);
				break;
			case mode_name::in_frameset_insertion_mode:
				on_in_frameset_insertion_mode(token);
				break;
			case mode_name::after_frameset_insertion_mode:
				on_after_frameset_insertion_mode(token);
				break;
			case mode_name::after_after_body_insertion_mode:
				on_after_after_body_insertion_mode(token);
				break;
			case mode_name::after_after_frameset_insertion_mode:
				on_after_after_frameset_insertion_mode(token);
				break;
			}

		Finish:
			if constexpr (std::is_same_v<start_tag_token, Token>)
			{
				// https://html.spec.whatwg.org/multipage/parsing.html#acknowledge-self-closing-flag

				if(token.m_self_closing_flag && !token.m_acknowledged_self_closing_flag)
					report_error(error_name::non_void_html_element_start_tag_with_trailing_solidus);
			}
		}

		template <typename Token>
		void reprocess_token(Token& token)
		{
			on_emit_token(token);
		}

		template <typename Token>
		void on_emit_token(Token& token)
		{
			process_token(m_insertion_mode, token);
		}

		bool is_mathml_text_integration_point(stack_entry_type const& entry) const
		{
			this_type const* P = static_cast<this_type const*>(this);

			if (P->namespace_uri_name(entry.m_it) == ns_name::MathML)
			{
				switch (P->local_name_name(entry.m_it))
				{
				case tag_name::Mi: case tag_name::Mo: case tag_name::Mn: case tag_name::Ms: case tag_name::Mtext:
					return true;
				default:
					break;
				}
			}

			return false;
		}

		bool is_html_integration_point(stack_entry_type const& entry) const
		{
			this_type const* P = static_cast<this_type const*>(this);

			ns_name  ns  = P->namespace_uri_name(entry.m_it);
			tag_name tag = P->local_name_name(entry.m_it);

			if (ns == ns_name::MathML && tag == tag_name::Annotation_xml)
			{
				auto it = entry.m_token.find(attribute_name::Encoding);
				if (it != entry.m_token.end())
				{
					std::u32string s;
					to_ascii_lowercase(it->m_value.begin(), it->m_value.end(), std::back_inserter(s));
					if (s == U"text/html" || s == U"application/xhtml+xml") return true;
				}
			}
			else if (ns == ns_name::SVG)
			{
				if (   tag == tag_name::ForeignObject
					|| tag == tag_name::Desc
					|| tag == tag_name::Title) return true;
			}

			return false;
		}

		// ----------------------------------------------------------------------------------------
		// ノードの作成と挿入
		//
		// 12.2.6.1 Creating and inserting nodes
		// https://html.spec.whatwg.org/multipage/parsing.html#creating-and-inserting-nodes
		// ----------------------------------------------------------------------------------------

		/*! @brief 妥当なノード挿入先を調べる

		https://html.spec.whatwg.org/multipage/parsing.html#appropriate-place-for-inserting-a-node
		*/
		node_pointer appropriate_place_for_inserting_node(node_pointer target)
		{
			this_type* P = static_cast<this_type*>(this);

			node_pointer adjusted_insertion_location = P->end(target);

			bool tbl;
			switch (P->local_name_name(target))
			{
			case tag_name::Table: case tag_name::Tbody: case tag_name::Tfoot: case tag_name::Thead: case tag_name::Tr:
				tbl = true;
				break;
			default:
				tbl = false;
			}

			if (m_foster_parenting && tbl)
			{
				auto it = std::find_if(m_open_element_stack.rbegin(), m_open_element_stack.rend(), [=](stack_entry_type const& entry) {
					return P->local_name_name(entry.m_it) == tag_name::Template; });
				auto last_template = (it == m_open_element_stack.rend()) ? m_open_element_stack.end() : (++it).base();

				it = std::find_if(m_open_element_stack.rbegin(), m_open_element_stack.rend(), [=](stack_entry_type const& entry) {
					return P->local_name_name(entry.m_it) == tag_name::Table; });
				auto last_table = (it == m_open_element_stack.rend()) ? m_open_element_stack.end() : (++it).base();

				if (last_template != m_open_element_stack.end())
				{
					std::int32_t distance = 0;
					if (last_table != m_open_element_stack.end()) distance = std::distance(last_table, last_template);

					if (0 < distance)
					{
						adjusted_insertion_location = P->end(last_template->m_it);
						goto Abort;
					}
				}

				if (last_table == m_open_element_stack.end())
				{
					adjusted_insertion_location = m_open_element_stack.front().m_it.end();
					goto Abort;
				}

				if (P->parent(last_table->m_it) != P->document())
				{
					adjusted_insertion_location = last_table->m_it;
					goto Abort;
				}

				// スクリプトをサポートしないため、ここに到達することはないはず。
				// しかしテストでは通過させるため表明はしない。
				adjusted_insertion_location = P->end((++it).base()->m_it);
			}
			else
			{
				adjusted_insertion_location = P->end(target);
			}

			Abort:

			return adjusted_insertion_location;
		}

		/*! @brief トークンから要素を作成する
		
		https://html.spec.whatwg.org/multipage/parsing.html#create-an-element-for-the-token
		*/
		element_node_type create_element_for_token(start_tag_token& token, ns_name ns, node_pointer parent)
		{
			//TODO: 未実装
			this_type* P = static_cast<this_type*>(this);

			element_node_type el = P->create_element(token.m_tag_name, ns, U"");
			
			return el;
		}

		/*!
		https://html.spec.whatwg.org/multipage/parsing.html#insert-a-foreign-element
		*/
		node_pointer insert_foreign_element(start_tag_token& token, ns_name ns)
		{
			this_type* P = static_cast<this_type*>(this);

			node_pointer adjusted_insertion_location = appropriate_place_for_inserting_node(current_node().m_it);
			element_node_type element = create_element_for_token(token, ns, P->parent(adjusted_insertion_location));
			node_pointer it;

			//TODO: 要素を挿入可能か検査する機構が必要
			if (true)
			{
				it = P->insert(adjusted_insertion_location, element);
			}

			m_open_element_stack.push_back({ token, it });

			return it;
		}

		/*!
		https://html.spec.whatwg.org/multipage/parsing.html#insert-an-html-element
		*/
		node_pointer insert_html_element(start_tag_token& token)
		{
			return insert_foreign_element(token, ns_name::HTML);
		}

		/*!
		https://html.spec.whatwg.org/multipage/parsing.html#adjust-mathml-attributes
		*/
		template <typename Token>
		void adjust_mathml_attributes(Token& token)
		{
			if constexpr (std::is_base_of_v<tag_token, Token>)
			{
				for (auto& attr : token.m_attributes)
				{
					if (attr.m_name == U"definitionurl") attr.m_name = U"definitionURL";
				}
			}
		}

		/*!	
		https://html.spec.whatwg.org/multipage/parsing.html#adjust-svg-attributes
		*/
		template <typename Token>
		void adjust_svg_attributes(Token& token)
		{
			if constexpr (std::is_base_of_v<tag_token, Token>)
			{
				for (auto& attr : token.m_attributes)
				{
					auto it = svg_attributes_conversion_tbl.find(attr.m_name);
					if(it != svg_attributes_conversion_tbl.end()) attr.m_name = it->second;
				}
			}
		}

		/*!
		https://html.spec.whatwg.org/multipage/parsing.html#adjust-foreign-attributes
		*/
		template <typename Token>
		void adjust_foreign_attributes(Token& token)
		{
			if constexpr (std::is_base_of_v<tag_token, Token>)
			{
				for (auto& attr : token.m_attributes)
				{
					auto it = foreign_attributes_conversion_tbl.find(attr.m_name);
					if (it != foreign_attributes_conversion_tbl.end())
					{
						attr.m_prefix     = it->second.m_prefix;
						attr.m_local_name = it->second.m_local_name;
						attr.m_namespace  = it->second.m_namespace;
					}
				}
			}
		}

		/*!
		https://html.spec.whatwg.org/multipage/parsing.html#insert-a-character
		*/
		void insert_character(char32_t cp)
		{
			this_type* P = static_cast<this_type*>(this);

			node_pointer it1 = appropriate_place_for_inserting_node(current_node().m_it);

			node_pointer parent = P->parent(it1);
			if (parent == P->document()) return;

			if (it1 != P->begin(parent))
			{
				node_pointer prev = P->prev(it1);
				if (P->is_text(prev)) P->append_text(prev, cp);
			}
			else
			{
				text_node_type text = P->create_text(cp);
				node_pointer it2 = P->insert(it1, text);
				P->set_document(it2, P->document());
			}
		}

		/*!
		https://html.spec.whatwg.org/multipage/parsing.html#insert-a-comment
		*/
		void insert_comment(comment_token const& token, node_pointer pos)
		{
			this_type* P = static_cast<this_type*>(this);

			comment_node_type comment = P->create_comment(token.m_data);

			P->insert(pos, std::move(comment));
		}

		void insert_comment(comment_token const& token)
		{
			insert_comment(token, appropriate_place_for_inserting_node(current_node().m_it));
		}

		// ----------------------------------------------------------------------------------------
		// テキストのみ含む要素の解析
		//
		// 12.2.6.2 Parsing elements that contain only text
		// https://html.spec.whatwg.org/multipage/parsing.html#parsing-elements-that-contain-only-text
		// ----------------------------------------------------------------------------------------

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#generic-raw-text-element-parsing-algorithm
		*/
		void parse_generic_raw_text_element(start_tag_token& token)
		{
			insert_html_element(token);
			base_type::change_state(base_type::RAWTEXT_state);
			m_original_insertion_mode = m_insertion_mode;
			insertion_mode(mode_name::text_insertion_mode);
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#generic-rcdata-element-parsing-algorithm
		*/
		void parse_generic_raw_rcdata_element(start_tag_token& token)
		{
			insert_html_element(token);
			base_type::change_state(base_type::RCDATA_state);
			m_original_insertion_mode = m_insertion_mode;
			insertion_mode(mode_name::text_insertion_mode);
		}

		// ----------------------------------------------------------------------------------------
		// 暗黙の終了タグを持つ要素を閉じる
		//
		// 12.2.6.3 Closing elements that have implied end tags
		// https://html.spec.whatwg.org/multipage/parsing.html#closing-elements-that-have-implied-end-tags
		// ----------------------------------------------------------------------------------------

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#generate-implied-end-tags
		*/
		void generate_implied_end_tags(tag_name without)
		{
			this_type* P = static_cast<this_type*>(this);

			while (!m_open_element_stack.empty())
			{
				tag_name tag = P->local_name_name(m_open_element_stack.back().m_it);
				if (tag == without) return;
				switch (tag)
				{
				case tag_name::Dd: case tag_name::Dt: case tag_name::Li: case tag_name::Optgroup: case tag_name::Option:
				case tag_name::P:  case tag_name::Rb: case tag_name::Rp: case tag_name::Rt:       case tag_name::Rtc:
					m_open_element_stack.pop_back();
					break;
				default:
					return;
				}
			}
		}

		/*
		https://html.spec.whatwg.org/multipage/parsing.html#generate-all-implied-end-tags-thoroughly
		*/
		void generate_all_implied_end_tags_thoroughly()
		{
			this_type* P = static_cast<this_type*>(this);

			while (!m_open_element_stack.empty())
			{
				tag_name tag = P->local_name_name(m_open_element_stack.back().m_it);
				switch (tag)
				{
				case tag_name::Caption: case tag_name::Colgroup: case tag_name::Dd:    case tag_name::Dt: case tag_name::Li:    case tag_name::Optgroup:
				case tag_name::Option:  case tag_name::P:        case tag_name::Rb:    case tag_name::Rp: case tag_name::Rt:    case tag_name::Rtc:
				case tag_name::Tbody:   case tag_name::Td:       case tag_name::Tfoot: case tag_name::Th: case tag_name::Thead: case tag_name::Tr:
					m_open_element_stack.pop_back();
					break;
				default:
					return;
				}
			}
		}

		// ----------------------------------------------------------------------------------------
		// 12.2.6.4.1 The "initial" insertion mode
		//
		// https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_initial_insertion_mode(Token& token)
		{
			this_type* P = static_cast<this_type*>(this);

			if constexpr (std::is_same_v<character_token, Token>)
			{
				if (is_ascii_white_space(token.m_data)) return;
			}

			if constexpr (std::is_same_v<comment_token, Token>)
			{
				insert_comment(token, P->document().end());
				return;
			}

			if constexpr (std::is_same_v<DOCTYPE_token, Token>)
			{
				document_type_node_type doctype = P->create_document_type(
					token.m_name, token.m_public_identifier, token.m_system_identifier);
				P->insert(P->end(P->document()), doctype);

				if (!P->is_iframe_srcdoc_document())
				{
					if (in_quirks_condition(token)) P->set_document_mode(document_mode_name::quirks);
					else if (in_limited_quirks_condition(token)) P->set_document_mode(document_mode_name::limited_quirks);
				}

				insertion_mode(mode_name::before_html_insertion_mode);
				return;
			}

			if (!P->is_iframe_srcdoc_document())
			{
				P->report_error();
				P->set_document_mode(document_mode_name::quirks);
			}

			insertion_mode(mode_name::before_html_insertion_mode);
			reprocess_token(token);
		}

		template <typename Token>
		bool in_quirks_condition(Token const& token) const
		{
			if (token.m_force_quirks_flag) return true;
			if (token.m_name != U"html") return true;

			std::u32string ipublic;
			to_ascii_lowercase(token.m_public_identifier.begin(), token.m_public_identifier.end(), std::back_inserter(ipublic));

			std::u32string isystem;
			to_ascii_lowercase(token.m_system_identifier.begin(), token.m_system_identifier.end(), std::back_inserter(isystem));

			if (ipublic == U"-//w3o//dtd w3 html strict 3.0//en//") return true;
			if (ipublic == U"-/w3c/dtd html 4.0 transitional/en") return true;
			if (ipublic == U"html") return true;

			if (isystem == U"http://www.ibm.com/data/dtd/v11/ibmxhtml1-transitional.dtd") return true;

			if (quirks_mode_tbl.lookup(ipublic.begin(), ipublic.end()).first) return true;

			if (token.m_system_identifier.empty())
			{
				std::u32string_view constexpr sv1(U"-//w3c//dtd html 4.01 frameset//");
				if (sv1.size() <= ipublic.size() && std::equal(sv1.begin(), sv1.end(), ipublic.begin())) return true;

				std::u32string_view constexpr sv2(U"-//w3c//dtd html 4.01 transitional//");
				if (sv2.size() <= ipublic.size() && std::equal(sv2.begin(), sv2.end(), ipublic.begin())) return true;
			}

			return false;
		}

		template <typename Token>
		bool in_limited_quirks_condition(Token const& token) const
		{
			if (istart_with(U"-//w3c//dtd xhtml 1.0 frameset//", token.m_public_identifier)) return true;
			if (istart_with(U"-//w3c//dtd xhtml 1.0 transitional//", token.m_public_identifier)) return true;

			if (!token.m_system_identifier.empty())
			{
				if (istart_with(U"-//w3c//dtd html 4.01 frameset//", token.m_public_identifier)) return true;
				if (istart_with(U"-//w3c//dtd html 4.01 transitional//", token.m_public_identifier)) return true;
			}

			return false;
		}

		bool istart_with(std::u32string_view s1, std::u32string const& s2) const
		{
			if (s2.size() < s1.size()) return false;

			auto it = s2.begin();
			for (char32_t c : s1)
			{
				char32_t cp = *it++;
				if (is_ascii_upper_alpha(cp)) cp += 0x20;
				if (cp != c) return false;
			}

			return true;
		}

		// ----------------------------------------------------------------------------------------
		// 12.2.6.4.2 The "before html" insertion mode
		//
		// https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_before_html_insertion_mode(Token& token)
		{
			this_type* P = static_cast<this_type*>(this);

			if constexpr (std::is_same_v<DOCTYPE_token, Token>)
			{
				report_error();
				return;
			}

			if constexpr (std::is_same_v<comment_token, Token>)
			{
				insert_comment(token, P->document().end());
				return;
			}

			if constexpr (std::is_same_v<character_token, Token>)
			{
				if (is_ascii_white_space(token.m_data)) return;
			}

			if constexpr (std::is_same_v<start_tag_token, Token>)
			{
				if (token.m_tag_name_id == tag_name::Html)
				{
					element_node_type el = create_element_for_token(token, ns_name::HTML, P->document());
					node_pointer it = P->insert(P->document().end(), std::move(el));
					m_open_element_stack.push_back({ token, it });
					insertion_mode(mode_name::before_head_insertion_mode);

					return;
				}
			}

			if constexpr (std::is_same_v<end_tag_token, Token>)
			{
				switch (token.m_tag_name_id)
				{
				case tag_name::Head: case tag_name::Body: case tag_name::Html: case tag_name::Br:
					goto AnythingElse;
				default:
					report_error();
					return;
				}
			}

			goto AnythingElse;
		AnythingElse:
			element_node_type el = P->create_element(tag_name::Html);
			node_pointer it = P->insert(P->document().end(), std::move(el));
			P->set_document(it, P->document());
			m_open_element_stack.push_back({ start_tag_token(), it });
			insertion_mode(mode_name::before_head_insertion_mode);
			reprocess_token(token);
		}

		// ----------------------------------------------------------------------------------------
		// 12.2.6.4.3 The "before head" insertion mode
		//
		// https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_before_head_insertion_mode(Token& token)
		{
			if constexpr (std::is_same_v<character_token, Token>)
			{
				if (is_ascii_white_space(token.m_data)) return ;
			}

			if constexpr (std::is_same_v<comment_token, Token>)
			{
				insert_comment(token);
				return;
			}

			if constexpr (std::is_same_v<DOCTYPE_token, Token>)
			{
				report_error();
				return;
			}

			if constexpr (std::is_same_v<start_tag_token, Token>)
			{
				if (token.m_tag_name_id == tag_name::Html)
				{
					on_in_body_insertion_mode(token);
					return;
				}

				if (token.m_tag_name_id == tag_name::Head)
				{
					node_pointer it = insert_html_element(token);
					m_head_element_pointer = it;
					insertion_mode(mode_name::in_head_insertion_mode);
					return;
				}
			}
			
			if constexpr (std::is_same_v<end_tag_token, Token>)
			{
				switch (token.m_tag_name_id)
				{
				case tag_name::Head: case tag_name::Body: case tag_name::Html: case tag_name::Br:
					goto AnythingElse;
				default:
					report_error();
					return;
				}
			}

			goto AnythingElse;
		AnythingElse:
			start_tag_token t(tag_name::Head);
			node_pointer it = insert_html_element(t);
			m_head_element_pointer = it;
			insertion_mode(mode_name::in_head_insertion_mode);
			reprocess_token(token);
		}

		// ----------------------------------------------------------------------------------------
		// 12.2.6.4.4 The "in head" insertion mode
		//
		// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_head_insertion_mode(Token& token)
		{
			bool state_flag = false;

			this_type* P = static_cast<this_type*>(this);

			if constexpr (std::is_same_v<character_token, Token>)
			{
				if (is_ascii_white_space(token.m_data)) insert_character(token.m_data);
				return;
			}

			if constexpr (std::is_same_v<comment_token, Token>)
			{
				insert_comment(token);
				return;
			}

			if constexpr (std::is_same_v<DOCTYPE_token, Token>)
			{
				report_error();
				return;
			}

			if constexpr (std::is_same_v<start_tag_token, Token>)
			{
				switch (token.m_tag_name_id)
				{
				case tag_name::Html:
					on_in_body_insertion_mode(token);
					return;
				case tag_name::Base: case tag_name::Basefont: case tag_name::Bgsound: case tag_name::Link:
					insert_html_element(token);
					m_open_element_stack.pop_back();
					if (token.m_self_closing_flag == true) token.m_acknowledged_self_closing_flag = true;
					return;
				case tag_name::Meta:
				{
					node_pointer it = insert_html_element(token);
					m_open_element_stack.pop_back();
					if (token.m_self_closing_flag == true) token.m_acknowledged_self_closing_flag = true;
					if (m_encoding_confidence != encoding_confidence_name::tentative) return;
					//TODO:

					return;
				}
				case tag_name::Title:
					parse_generic_raw_rcdata_element(token);
					return;
				case tag_name::Noframes: case tag_name::Style:
					parse_generic_raw_text_element(token);
					return;
				case tag_name::Noscript:
					if (m_scripting_flag)
					{
						parse_generic_raw_text_element(token);
						return;
					}
					insert_html_element(token);
					insertion_mode(mode_name::in_head_noscript_insertion_mode);
					return;
				case tag_name::Script:
				{
					node_pointer adjusted_insertion_location = appropriate_place_for_inserting_node(current_node().m_it);
					element_node_type el = create_element_for_token(token, ns_name::HTML, P->parent(adjusted_insertion_location));
					node_pointer it = P->insert(adjusted_insertion_location, el);
					P->set_document(it, P->document());
					P->set_non_blocking_flag(it, false);
					if (is_fragments_parser) P->set_already_started_flag(it, true);

					//TODO: スクリプトの実行を防ぐを無条件に設定している
					P->set_already_started_flag(it, true);
					m_open_element_stack.push_back({ token, it });
					base_type::change_state(base_type::script_data_state);
					m_original_insertion_mode = m_insertion_mode;
					insertion_mode(mode_name::text_insertion_mode);
					return;
				}
				default:
					break;
				}
			}

			if constexpr (std::is_same_v<end_tag_token, Token>)
			{
				switch (token.m_tag_name_id)
				{
				case tag_name::Head:
					assert(P->local_name_name(current_node().m_it) == tag_name::Head);
					m_open_element_stack.pop_back();
					insertion_mode(mode_name::after_head_insertion_mode);
					return;
				case tag_name::Body: case tag_name::Html: case tag_name::Br:
					goto AnythingElse;
				default:
					break;
				}
			}

			if constexpr (std::is_same_v<start_tag_token, Token>)
			{
				if (token.m_tag_name_id == tag_name::Template)
				{
					insert_html_element(token);
					push_active_formatting_element_list();
					m_frameset_ok_flag = false;
					insertion_mode(mode_name::in_template_insertion_mode);
					m_template_insertion_mode_stack.push_back(mode_name::in_template_insertion_mode);
					return;
				}
			}

			if constexpr (std::is_same_v<end_tag_token, Token>)
			{
				if (token.m_tag_name_id == tag_name::Template)
				{
					if (std::find_if(m_open_element_stack.begin(), m_open_element_stack.end(), [P](auto const& entry)->bool {
							return P->local_name_name(entry.m_it) == tag_name::Template; }) == m_open_element_stack.end())
					{
						report_error();
						return;
					}
					generate_all_implied_end_tags_thoroughly();
					if (P->local_name_name(current_node().m_it) != tag_name::Template) report_error();
					pop_until(tag_name::Template);
					clear_active_formatting_element_list();
					m_template_insertion_mode_stack.pop_back();
					reset_insertion_mode_appropriately();
					return;
				}
			}

			state_flag = false;
			if constexpr (std::is_same_v<start_tag_token, Token>) if (token.m_tag_name_id == tag_name::Head) state_flag = true;
			if constexpr (std::is_same_v<end_tag_token, Token>) state_flag = true;
			if(state_flag)
			{
				report_error();
				return;
			}

			goto AnythingElse;
		AnythingElse:
			assert(P->local_name_name(current_node().m_it) == tag_name::Head);
			m_open_element_stack.pop_back();
			insertion_mode(mode_name::after_head_insertion_mode);
			reprocess_token(token);
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_head_noscript_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_after_head_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_body_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_text_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_table_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_table_text_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_caption_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_column_group_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_table_body_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_row_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_cell_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_select_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_select_in_table_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_template_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_after_body_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_in_frameset_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_after_frameset_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_after_after_body_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 
		//
		// 
		// ----------------------------------------------------------------------------------------

		template <typename Token>
		void on_after_after_frameset_insertion_mode(Token& token)
		{
		}

		// ----------------------------------------------------------------------------------------
		// 12.2.6.5 The rules for parsing tokens in foreign content
		//
		// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inforeign
		// ----------------------------------------------------------------------------------------
		
		template <typename Token>
		void on_foreign_content(Token& token)
		{

		}
	};
}
