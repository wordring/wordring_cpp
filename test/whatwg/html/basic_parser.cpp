﻿// test/whatwg/html/basic_parser.cpp

#include <boost/test/unit_test.hpp>

#include <wordring/whatwg/html/basic_parser.hpp>

#include <wordring/whatwg/html/parsing/context.hpp>
#include <wordring/whatwg/html/parsing/atom_tbl.hpp>

BOOST_AUTO_TEST_SUITE(whatwg_html__basic_parser__test)

BOOST_AUTO_TEST_CASE(parser_context__1)
{
	using namespace wordring::whatwg::html;
	using namespace wordring::whatwg::html::parsing;

	BOOST_CHECK(true);

	context<int, int> pc;

	//attribute_set.contains(std::u32string());
}

BOOST_AUTO_TEST_SUITE_END()
