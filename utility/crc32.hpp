#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace crc
{
	inline std::array<uint32_t, 0x400> make_crc_table( )
	{
		std::array<uint32_t, 0x400> table{};
		for ( int tbl = 0; tbl < 256; ++tbl )
		{
			uint32_t crc = static_cast< uint32_t >( tbl ) << 24;

			for ( int count = 8; count > 0; count-- )
			{
				// Original code used a signed int and checked for negative (high bit set).
				// Reproduce that behavior by testing the top bit on the unsigned value.
				if ( ( crc & 0x80000000u ) != 0 )
				{
					crc = crc << 1;
				}
				else
				{
					crc = ( crc << 1 ) ^ 0x04C11DB7;
				}
			}

			table[ 255 - tbl ] = crc;
		}
		return table;
	}

	inline const std::array<uint32_t, 0x400> crc_table = make_crc_table( );

	[[nodiscard]]
	inline uint32_t crc32( const char* string ) noexcept
	{
		if ( string == nullptr )
		{
			return 0;
		}

		uint32_t crc = 0;
		size_t i = 0;
		for ( ;; )
		{
			uint8_t b = static_cast< uint8_t >( string[ i ] );
			crc = crc_table[ b ^ ( ( crc >> 24 ) & 0xFF ) ]
				^ ( ( crc << 8 ) & 0xFFFFFF00 );

			if ( b == 0 )
			{
				break;
			}

			++i;
		}
		return crc;
	}

	[[nodiscard]]
	inline std::optional<uint32_t> crc32_hash( std::string_view title ) noexcept
	{
		if ( title.size( ) > 32 )
		{
			return std::nullopt;
		}

		std::array<uint8_t, 33> buf{};
		for ( size_t i = 0; i < title.size( ); ++i )
		{
			unsigned char c = static_cast< unsigned char >( title[ i ] );
			buf[ i ] = ( c < 128 ) ? static_cast< uint8_t >( c ) : '?';
		}

		uint32_t crc = 0;
		for ( size_t i = 0; i <= title.size( ); ++i )
		{
			uint8_t b = buf[ i ];
			crc = crc_table[ b ^ ( ( crc >> 24 ) & 0xFF ) ]
				^ ( ( crc << 8 ) & 0xFFFFFF00 );
		}

		return crc;
	}
}
