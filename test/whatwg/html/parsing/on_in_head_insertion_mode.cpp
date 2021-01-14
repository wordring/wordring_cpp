﻿// wordring_cpp/test/whatwg/html/parsing/on_in_head_insertion_mode.cpp

#include <boost/test/unit_test.hpp>

#include <wordring/html/html_defs.hpp>
#include <wordring/html/simple_node.hpp>
#include <wordring/html/simple_parser.hpp>
#include <wordring/html/simple_traits.hpp>

#include <wordring/whatwg/html/parsing/atom_tbl.hpp>
#include <wordring/whatwg/html/parsing/token.hpp>
#include <wordring/whatwg/html/parsing/tokenization.hpp>
#include <wordring/whatwg/html/parsing/tree_construction_dispatcher.hpp>

#include <wordring/encoding/encoding.hpp>
#include <wordring/tree/tree.hpp>

#include <string>

namespace
{
	using namespace wordring::html;

	using node_type = simple_node<std::string>;
	using tree = wordring::tree<node_type>;
	using adapter_type = node_traits<typename tree::iterator>;

	using document_type = typename adapter_type::document_type;
	using document_type_type = typename adapter_type::document_type_type;
	using document_fragment_type = typename adapter_type::document_fragment_type;
	using element_type = typename adapter_type::element_type;
	using text_type = typename adapter_type::text_type;
	using processing_instruction_type = typename adapter_type::processing_instruction_type;
	using comment_type = typename adapter_type::comment_type;

	struct test_parser : public simple_parser_base<test_parser, tree>
	{
	public:
		using base_type = simple_parser_base<test_parser, tree>;

		using base_type::mode_name;

		test_parser()
			: base_type(encoding_confidence_name::tentative, static_cast<wordring::encoding_name>(0))
		{
		}
	};
}

BOOST_AUTO_TEST_SUITE(on_in_head_insertion_mode_test)

/*
META タグの文字エンコーディング指定
*/
BOOST_AUTO_TEST_CASE(on_in_head_insertion_mode_1)
{
	test_parser tp;
	std::u32string s = UR"(<meta charset="utf-8">)";
	for (char32_t cp : s) tp.push_code_point(cp);
	tp.push_eof();

	BOOST_CHECK(tp.m_encoding_confidence == encoding_confidence_name::certain);
	BOOST_CHECK(tp.m_encoding_name == wordring::encoding_name::UTF_8);
	BOOST_TEST_MESSAGE("Unimplemented, See simple_parser_base::on_change_encoding().");
}

BOOST_AUTO_TEST_CASE(on_in_head_insertion_mode_2)
{
	test_parser tp;
	std::u32string s = UR"(<meta http-equiv="Content-Type" content="text/html; charset=shift_jis">)";
	for (char32_t cp : s) tp.push_code_point(cp);
	tp.push_eof();

	BOOST_CHECK(tp.m_encoding_confidence == encoding_confidence_name::certain);
	BOOST_CHECK(tp.m_encoding_name == wordring::encoding_name::Shift_JIS);
	BOOST_TEST_MESSAGE("Unimplemented, See simple_parser_base::on_change_encoding().");
}

BOOST_AUTO_TEST_CASE(on_in_head_insertion_mode_3)
{
	test_parser tp;
	std::u32string s = UR"(<content="text/html; charset=shift_jis">)";
	for (char32_t cp : s) tp.push_code_point(cp);
	tp.push_eof();

	BOOST_CHECK(tp.m_encoding_confidence == encoding_confidence_name::tentative);
	BOOST_CHECK(tp.m_encoding_name == static_cast<wordring::encoding_name>(0));
}

BOOST_AUTO_TEST_CASE(on_in_head_insertion_mode_4)
{
	test_parser tp;
	std::u32string s = UR"(<meta http-equiv="Content-Type" content=" charset=shift_jis text/html;">)";
	for (char32_t cp : s) tp.push_code_point(cp);
	tp.push_eof();

	BOOST_CHECK(tp.m_encoding_confidence == encoding_confidence_name::certain);
	BOOST_CHECK(tp.m_encoding_name == wordring::encoding_name::Shift_JIS);
	BOOST_TEST_MESSAGE("Unimplemented, See simple_parser_base::on_change_encoding().");
}

BOOST_AUTO_TEST_CASE(on_in_head_insertion_mode_5)
{
	test_parser tp;
	std::u32string s = UR"(<meta http-equiv="Content-Type" content=" charset=shift_jis; text/html;">)";
	for (char32_t cp : s) tp.push_code_point(cp);
	tp.push_eof();

	BOOST_CHECK(tp.m_encoding_confidence == encoding_confidence_name::certain);
	BOOST_CHECK(tp.m_encoding_name == wordring::encoding_name::Shift_JIS);
	BOOST_TEST_MESSAGE("Unimplemented, See simple_parser_base::on_change_encoding().");
}

BOOST_AUTO_TEST_SUITE_END()
