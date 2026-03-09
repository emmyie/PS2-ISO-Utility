#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace str_util
{
	[[nodiscard]]
	inline std::string upper_copy( std::string_view s )
	{
		std::string result( s );
		std::transform( result.begin( ), result.end( ), result.begin( ),
			[ ] ( unsigned char c )
		{
			return std::toupper( c );
		} );
		return result;
	}

	[[nodiscard]]
	inline std::string trim_copy( std::string_view s )
	{
		size_t a = s.find_first_not_of( " \t\r\n" );
		size_t b = s.find_last_not_of( " \t\r\n" );

		if ( a == std::string_view::npos ) return {};
		return std::string( s.substr( a, b - a + 1 ) );
	}

	[[nodiscard]]
	inline std::string sanitize_title( std::string_view title )
	{
		std::string result;
		result.reserve( std::min<size_t>( 32, title.size( ) ) );

		for ( char c : title )
		{
			if ( c == '\n' || c == '\r' ) continue;
			if ( result.size( ) == 32 ) break;
			result.push_back( static_cast< unsigned char >( c ) < 128 ? c : '?' );
		}

		return result;
	}
}