#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

struct ul_cfg_record
{
	std::array<uint8_t, 0x40> m_raw{};

	ul_cfg_record( ) = default;

	static ul_cfg_record make(
		const std::string& title,
		const std::string& boot_id,
		uint8_t chunk_count,
		bool is_dvd,
		uint32_t size_mb )
	{
		ul_cfg_record record{};
		record.m_raw.fill( 0x20 ); // default fill with spaces

		// Title (32 bytes, space padded)
		for ( size_t i = 0; i < std::min<size_t>( title.size( ), 32 ); i++ )
		{
			record.m_raw[ i ] = static_cast< uint8_t >( title[ i ] );
		}

		// "ul.<bootid>" marker at 0x20 (16 bytes total)
		const std::string mark = "ul." + boot_id;
		if ( mark.size( ) > 16 )
		{
			throw std::runtime_error( "boot_id too long: 'ul." + boot_id + "' exceeds 16 bytes" );
		}
		for ( size_t i = 0; i < mark.size( ) && ( 0x20 + i ) < 0x30; ++i )
		{
			record.m_raw[ 0x20 + i ] = static_cast< uint8_t >( mark[ i ] );
		}

		// Chunk count @ 0x30
		record.m_raw[ 0x30 ] = chunk_count;

		// Media type @ 0x31
		record.m_raw[ 0x31 ] = is_dvd ? 0x14 : 0x12;

		// Always 0x08 @ 0x36
		record.m_raw[ 0x36 ] = 0x08;

		// Game size in MB @ 0x39–0x3C (LE)
		record.m_raw[ 0x39 ] = ( uint8_t )( size_mb & 0xFF );
		record.m_raw[ 0x3A ] = ( uint8_t )( ( size_mb >> 8 ) & 0xFF );
		record.m_raw[ 0x3B ] = ( uint8_t )( ( size_mb >> 16 ) & 0xFF );
		record.m_raw[ 0x3C ] = ( uint8_t )( ( size_mb >> 24 ) & 0xFF );

		return record;
	}

	[[nodiscard]]
	std::string get_title( ) const
	{
		size_t len = 32;
		while ( len > 0 && m_raw[ len - 1 ] == ' ' ) --len;
		return std::string( reinterpret_cast< const char* >( m_raw.data( ) ), len );
	}

	[[nodiscard]]
	std::string get_boot_id( ) const
	{
		std::string result( reinterpret_cast< const char* >( m_raw.data( ) + 0x20 ), 16 );
		auto pos = result.find_last_not_of( " \t" );
		if ( pos == std::string::npos ) return std::string( );
		result.erase( pos + 1 );
		return result;
	}

	[[nodiscard]]
	uint8_t get_chunks( ) const
	{
		return m_raw[ 0x30 ];
	}

	[[nodiscard]]
	bool is_dvd( ) const
	{
		return m_raw[ 0x31 ] == 0x14;
	}
};

class ul_cfg_manager
{
public:
	explicit ul_cfg_manager( std::filesystem::path outdir )
		: m_cfg_path( std::move( outdir ) / "ul.cfg" )
	{ }

	void append( const ul_cfg_record& record ) const
	{
		std::ofstream out{ m_cfg_path, std::ios::binary | std::ios::app };
		if ( !out )
		{
			throw std::runtime_error( "Cannot open ul.cfg for append" );
		}

		out.write( reinterpret_cast< const char* >( record.m_raw.data( ) ), record.m_raw.size( ) );
		if ( !out.good( ) )
		{
			throw std::runtime_error( "Writing ul.cfg failed" );
		}
	}

private:
	std::filesystem::path m_cfg_path;
};
