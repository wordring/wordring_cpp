﻿#pragma once

// https://infra.spec.whatwg.org/
// https://triple-underscore.github.io/infra-ja.html

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>

namespace wordring::whatwg
{

	class scalar_value
	{
	public:
		scalar_value() noexcept = default;

		scalar_value(scalar_value const & sv) noexcept : ch_{ sv.ch_ } { assert(!(0xD800 <= ch_ && ch_ <= 0xDFFF)); }

		explicit scalar_value(char32_t ch) noexcept : ch_{ (0xD800 <= ch && ch <= 0xDFFF) ? 0xFFFD : ch }
		{
			assert(!(0xD800 <= ch_ && ch_ <= 0xDFFF));
		}

		~scalar_value() = default;

		// --------------------------------------------------------------------

		explicit operator char32_t() const noexcept
		{
			assert(!(0xD800 <= ch_ && ch_ <= 0xDFFF));
			return ch_;
		}

		explicit operator bool() const noexcept { return static_cast<bool>(ch_); }

		// 算術演算子 ----------------------------------------------------------

		/*! 単純代入（Basic assignment）
		*/
		scalar_value & operator=(char32_t ch) & noexcept
		{
			ch_ = (0xD800 <= ch && ch <= 0xDFFF) ? 0xFFFD : ch;
			assert(!(0xD800 <= ch_ && ch_ <= 0xDFFF));
			return *this;
		}

		scalar_value & operator=(scalar_value const & sv) & noexcept
		{
			ch_ = sv.ch_;
			assert(!(0xD800 <= ch_ && ch_ <= 0xDFFF));
			return *this;
		}

		scalar_value & operator=(scalar_value && sv) & noexcept
		{
			ch_ = sv.ch_;
			assert(!(0xD800 <= ch_ && ch_ <= 0xDFFF));
			return *this;
		}

		/*! 単項プラス（Unary plus）
		*/
		scalar_value operator+() const noexcept { return scalar_value{ ch_ }; }

		/*! 単項マイナス（Unary minus）
		*/
		scalar_value operator-() const noexcept { return scalar_value{ static_cast<char32_t>(-static_cast<int32_t>(ch_)) }; }

		/*! 前置インクリメント（Prefix increment）
		*/
		scalar_value & operator++() noexcept
		{
			operator=(++ch_);
			return *this;
		}

		/*! 後置インクリメント（Postfix increment）
		*/
		scalar_value operator++(int) noexcept
		{
			scalar_value result{ ch_ };
			operator++();
			return result;
		}

		/*! 前置デクリメント（Prefix decrement）
		*/
		scalar_value & operator--() noexcept
		{
			operator=(--ch_);
			return *this;
		}

		/*! 後置デクリメント（Postfix decrement）
		*/
		scalar_value operator--(int) noexcept
		{
			scalar_value result{ ch_ };
			operator--();
			return result;
		}

		// 論理演算子 ----------------------------------------------------------

		/*! 論理否定（Logical NOT）
		*/
		bool operator!() const noexcept { return !static_cast<bool>(*this); }

		// ビット演算子 --------------------------------------------------------

		/*! ビット否定（Bitwise NOT）
			~a
		*/
		scalar_value operator~() const noexcept { return scalar_value{ ~ch_ }; }

		// Compound assignment operators --------------------------------------

		/*! 加算代入（Addition assignment）
		*/
		scalar_value & operator+=(char32_t ch) noexcept
		{
			operator=(ch_ + ch);
			return *this;
		}

		scalar_value & operator+=(scalar_value const & sv) noexcept
		{
			operator=(ch_ + sv.ch_);
			return *this;
		}

		/*! 減算代入（Subtraction assignment）
		*/
		scalar_value & operator-=(char32_t ch) noexcept
		{
			operator=(ch_ - ch);
			return *this;
		}

		scalar_value & operator-=(scalar_value const & sv) noexcept
		{
			operator=(ch_ - sv.ch_);
			return *this;
		}

		/*! 乗算代入（Multiplication assignment）
		*/
		scalar_value & operator*=(char32_t ch) noexcept
		{
			operator=(ch_ * ch);
			return *this;
		}

		scalar_value & operator*=(scalar_value const & sv) noexcept
		{
			operator=(ch_ * sv.ch_);
			return *this;
		}

		/*! 除算代入（Division assignment）
		*/

		scalar_value & operator/=(char32_t ch)
		{
			operator=(ch_ / ch);
			return *this;
		}

		scalar_value & operator/=(scalar_value const & sv)
		{
			operator=(ch_ / sv.ch_);
			return *this;
		}

		/*! 剰余代入（Modulo assignment）
		*/
		scalar_value & operator%=(char32_t ch) noexcept
		{
			operator=(ch_ % ch);
			return *this;
		}

		scalar_value & operator%=(scalar_value const & sv) noexcept
		{
			operator=(ch_ % sv.ch_);
			return *this;
		}

		/*! ビット積代入（Bitwise AND assignment	）
			a &= b
		*/
		scalar_value & operator&=(char32_t ch) noexcept
		{
			operator=(ch_ & ch);
			return *this;
		}

		scalar_value & operator&=(scalar_value const & sv) noexcept
		{
			operator=(ch_ & sv.ch_);
			return *this;
		}

		/*! ビット和代入（Bitwise OR assignment）
			a |= b
		*/
		scalar_value & operator|=(char32_t ch) noexcept
		{
			operator=(ch_ | ch);
			return *this;
		}

		scalar_value & operator|=(scalar_value const & sv) noexcept
		{
			operator=(ch_ | sv.ch_);
			return *this;
		}

		/*! ビット排他的論理和代入（Bitwise XOR assignment	）
			a ^= b
		*/
		scalar_value & operator^=(char32_t ch) noexcept
		{
			operator=(ch_ ^ ch);
			return *this;
		}

		scalar_value & operator^=(scalar_value const & sv) noexcept
		{
			operator=(ch_ ^ sv.ch_);
			return *this;
		}

		/*! 左シフト代入（Bitwise left shift assignment）
			a <<= b
		*/
		scalar_value & operator<<=(char32_t ch) noexcept
		{
			operator=(ch_ << ch);
			return *this;
		}

		scalar_value & operator<<=(scalar_value const & sv) noexcept
		{
			operator=(ch_ << sv.ch_);
			return *this;
		}

		/*! 右シフト代入（Bitwise right shift assignment）
			a >>= b
		*/
		scalar_value & operator>>=(char32_t ch) noexcept
		{
			operator=(ch_ >> ch);
			return *this;
		}

		scalar_value & operator>>=(scalar_value const & sv) noexcept
		{
			operator=(ch_ >> sv.ch_);
			return *this;
		}


		// その他の演算子 ------------------------------------------------------

	private:
		char32_t ch_;
	};

	// 算術演算子 --------------------------------------------------------------

	/*! 加算（Addition）
	*/
	inline scalar_value operator+(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } += sv2; }

	inline scalar_value operator+(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } += ch; }

	inline scalar_value operator+(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } += sv; }

	/*! 減算（Subtraction）
	*/
	inline scalar_value operator-(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } -= sv2; }

	inline scalar_value operator-(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } -= ch; }

	inline scalar_value operator-(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } -= sv; }

	/*! 乗算（Multiplication）
	*/
	inline scalar_value operator*(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } *= sv2; }

	inline scalar_value operator*(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } *= ch; }

	inline scalar_value operator*(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } *= sv; }

	/*! 除算（Division）
	*/
	inline scalar_value operator/(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } /= sv2; }

	inline scalar_value operator/(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } /= ch; }

	inline scalar_value operator/(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } /= sv; }

	/*! 剰余（Modulo）
	*/
	inline scalar_value operator%(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } %= sv2; }

	inline scalar_value operator%(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } %= ch; }

	inline scalar_value operator%(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } %= sv; }

	// 比較演算子 ----------------------------------------------------------

	/*! 等価（Equal to）
	*/
	inline bool operator==(scalar_value const & sv1, scalar_value const & sv2) noexcept
	{
		return static_cast<char32_t>(sv1) == static_cast<char32_t>(sv2);
	}

	inline bool operator==(scalar_value const & sv, char32_t ch) noexcept { return static_cast<char32_t>(sv) == ch; }

	inline bool operator==(char32_t ch, scalar_value const & sv) noexcept { return ch == static_cast<char32_t>(sv); }

	/*! 非等価（Not equal to）
	*/
	inline bool operator!=(scalar_value const & sv1, scalar_value const & sv2) noexcept
	{
		return static_cast<char32_t>(sv1) != static_cast<char32_t>(sv2);
	}

	inline bool operator!=(scalar_value const & sv, char32_t ch) noexcept { return static_cast<char32_t>(sv) != ch; }

	inline bool operator!=(char32_t ch, scalar_value const & sv) noexcept { return ch != static_cast<char32_t>(sv); }

	/*! 大なり（Greater than）
	*/
	inline bool operator>(scalar_value const & sv1, scalar_value const & sv2) noexcept
	{
		return static_cast<char32_t>(sv1) > static_cast<char32_t>(sv2);
	}

	inline bool operator>(scalar_value const & sv, char32_t ch) noexcept { return static_cast<char32_t>(sv) > ch; }

	inline bool operator>(char32_t ch, scalar_value const & sv) noexcept { return ch > static_cast<char32_t>(sv); }

	/*! 小なり（Less than）
	*/
	inline bool operator<(scalar_value const & sv1, scalar_value const & sv2) noexcept
	{
		return static_cast<char32_t>(sv1) < static_cast<char32_t>(sv2);
	}

	inline bool operator<(scalar_value const & sv, char32_t ch) noexcept { return static_cast<char32_t>(sv) < ch; }

	inline bool operator<(char32_t ch, scalar_value const & sv) noexcept { return ch < static_cast<char32_t>(sv); }

	/*! 大なりイコール（Greater than or equal to）
	*/
	inline bool operator>=(scalar_value const & sv1, scalar_value const & sv2) noexcept
	{
		return static_cast<char32_t>(sv1) >= static_cast<char32_t>(sv2);
	}

	inline bool operator>=(scalar_value const & sv, char32_t ch) noexcept { return static_cast<char32_t>(sv) >= ch; }

	inline bool operator>=(char32_t ch, scalar_value const & sv) noexcept { return ch >= static_cast<char32_t>(sv); }

	/*! 小なりイコール（Less than or equal to）
	*/
	inline bool operator<=(scalar_value const & sv1, scalar_value const & sv2) noexcept
	{
		return static_cast<char32_t>(sv1) <= static_cast<char32_t>(sv2);
	}

	inline bool operator<=(scalar_value const & sv, char32_t ch) noexcept { return static_cast<char32_t>(sv) <= ch; }

	inline bool operator<=(char32_t ch, scalar_value const & sv) noexcept { return ch <= static_cast<char32_t>(sv); }

	// ビット演算子 --------------------------------------------------------

	/*! ビット積	（Bitwise AND）
		a & b
	*/
	inline scalar_value operator&(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } &= sv2; }

	inline scalar_value operator&(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } &= ch; }

	inline scalar_value operator&(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } &= sv; }

	/*! ビット和	（Bitwise OR	）
		a | b
	*/
	inline scalar_value operator|(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } |= sv2; }

	inline scalar_value operator|(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } |= ch; }

	inline scalar_value operator|(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } |= sv; }

	/*! ビット排他的論理和（Bitwise XOR）
		a ^ b
	*/
	inline scalar_value operator^(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } ^= sv2; }

	inline scalar_value operator^(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } ^= ch; }

	inline scalar_value operator^(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } ^= sv; }

	/*! 左シフト	（Bitwise left shift）
		a << b
	*/
	inline scalar_value operator<<(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } <<= sv2; }

	inline scalar_value operator<<(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } <<= ch; }

	inline scalar_value operator<<(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } <<= sv; }

	/*! 右シフト	（Bitwise right shift）
		a >> b
	*/
	inline scalar_value operator>>(scalar_value const & sv1, scalar_value const & sv2) noexcept { return scalar_value{ sv1 } >>= sv2; }

	inline scalar_value operator>>(scalar_value const & sv, char32_t ch) noexcept { return scalar_value{ sv } >>= ch; }

	inline scalar_value operator>>(char32_t ch, scalar_value const & sv) noexcept { return scalar_value{ ch } >>= sv; }

	// --------------------------------------------------------------

	using usv_string = std::basic_string<scalar_value>;

	inline usv_string make_usv_string(std::u32string const & str)
	{
		usv_string result{};
		for (auto ch : str) result += scalar_value{ ch };
		return result;
	}

	inline usv_string make_usv_string(char32_t const * str)
	{
		return make_usv_string(std::u32string{ str });
	}
}