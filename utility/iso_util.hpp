#pragma once
#include "file_util.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace iso_util
{
	inline constexpr uint64_t sector_size = 2048;

	inline uint32_t le32( const uint8_t* p ) noexcept
	{
		return ( uint32_t )p[ 0 ] | ( ( uint32_t )p[ 1 ] << 8 ) |
			( ( uint32_t )p[ 2 ] << 16 ) | ( ( uint32_t )p[ 3 ] << 24 );
	}

	inline std::string read_system_cnf( const std::filesystem::path& iso )
	{
		std::ifstream f( iso, std::ios::binary );
		if ( !f )
		{
			throw std::runtime_error( "Cannot open ISO: " + iso.string( ) );
		}

		f.seekg( sector_size * 16, std::ios::beg );
		std::array<char, sector_size> pvd{};
		file_util::read_exact( f, pvd.data( ), pvd.size( ) );

		if ( ( uint8_t )pvd[ 0 ] != 1 || std::string( pvd.data( ) + 1, 5 ) != "CD001" || ( uint8_t )pvd[ 6 ] != 1 )
		{
			throw std::runtime_error( "Unsupported ISO9660 PVD" );
		}

		const size_t RDR_OFF = 0x9c;
		std::vector<uint8_t> rdr( pvd.begin( ) + RDR_OFF, pvd.begin( ) + RDR_OFF + 34 );

		uint32_t rootLBA = le32( &rdr[ 2 ] );
		uint32_t rootSize = le32( &rdr[ 10 ] );

		f.seekg( static_cast< uint64_t >( rootLBA ) * sector_size, std::ios::beg );
		std::vector<uint8_t> dir( rootSize );
		file_util::read_exact( f, reinterpret_cast< char* >( dir.data( ) ), dir.size( ) );

		size_t off = 0;
		while ( off < dir.size( ) )
		{
			uint8_t len = dir[ off ];
			if ( !len ) break;
			if ( off + len > dir.size( ) )
			{
				break;
			}

			// Ensure the directory record is large enough to contain the fields
			// we read below (minimum 34 bytes for a dir record with name length byte).
			if ( len < 34 )
			{
				off += len;
				continue;
			}

			const uint8_t* rec = &dir[ off ];
			uint8_t id_len = rec[ 32 ];

			// Cap id_len to the remaining bytes in the record to avoid OOB reads
			size_t max_id_len = static_cast<size_t>( len ) - 33;
			if ( id_len > max_id_len ) 
			{
				id_len = static_cast<uint8_t>( max_id_len );
			}

			// Try ASCII filename first
			std::string ident( reinterpret_cast< const char* >( rec ) + 33, id_len );
			std::string ident_upper;
			ident_upper.resize( ident.size( ) );
			std::transform( ident.begin( ), ident.end( ), ident_upper.begin( ),
				[ ] ( unsigned char c ) { return std::toupper( c ); } );

			bool is_system_cnf = ( id_len > 1 && ( ident_upper == "SYSTEM.CNF" || ident_upper == "SYSTEM.CNF;1" ) );

			// If not ASCII, attempt Joliet (UCS-2 BE) filename extraction by taking every
			// second byte (low byte) as done by many ISO parsers (Open-PS2-Loader does this).
			if ( !is_system_cnf && ( id_len > 1 ) && ( ( id_len % 2 ) == 0 ) )
			{
				size_t ulen = id_len / 2;
				std::string ident_u;
				ident_u.resize( ulen );
				for ( size_t i = 0; i < ulen; ++i )
				{
					ident_u[ i ] = static_cast< char >( rec[ 33 + ( i * 2 ) + 1 ] );
				}

				std::string ident_u_upper;
				ident_u_upper.resize( ident_u.size( ) );
				std::transform( ident_u.begin( ), ident_u.end( ), ident_u_upper.begin( ),
					[ ] ( unsigned char c ) { return std::toupper( c ); } );

				if ( ident_u_upper == "SYSTEM.CNF" || ident_u_upper == "SYSTEM.CNF;1" )
				{
					is_system_cnf = true;
				}
			}

			if ( is_system_cnf )
			{
				uint32_t lba = le32( rec + 2 );
				uint32_t size = le32( rec + 10 );

				std::vector<char> sysbuf( size );
				f.seekg( static_cast< uint64_t >( lba ) * sector_size, std::ios::beg );
				file_util::read_exact( f, sysbuf.data( ), sysbuf.size( ) );
				return { sysbuf.begin( ), sysbuf.end( ) };
			}
			off += len;
		}

		throw std::runtime_error( "SYSTEM.CNF not found" );
	}

	inline std::string extract_boot_id( const std::string& system_cnf )
	{
		// First try to parse explicit BOOT/BOOT2 lines like:
		// BOOT2 = cdrom0:\SLES_123.45;1
		std::string up( system_cnf.size( ), '\0' );
		std::transform( system_cnf.begin( ), system_cnf.end( ), up.begin( ), [ ] ( unsigned char c ) { return std::toupper( c ); } );

		const std::string boot2 = "BOOT2";
		const std::string boot1 = "BOOT";
		auto parse_after_equals = [&up] ( size_t pos ) -> std::optional<std::string>
		{
			size_t eq = up.find( '=', pos );
			if ( eq == std::string::npos )
			{
				return std::nullopt;
			}

			size_t val_start = eq + 1;
			// skip whitespace
			while ( val_start < up.size() && ( up[val_start] == ' ' || up[val_start] == '\t' ) ) ++val_start;
			if ( val_start >= up.size() ) 
			{
				return std::nullopt;
			}

			size_t val_end = up.find_first_of("\r\n", val_start);
			std::string val = ( val_end == std::string::npos ) ? up.substr( val_start ) : up.substr( val_start, val_end - val_start );

			// find CDROM0:\ inside the value
			size_t cdpos = val.find( "CDROM0:\\" );
			if ( cdpos == std::string::npos ) 
			{
				return std::nullopt;
			}

			size_t start = cdpos + 8; // length of "CDROM0:\\"
			size_t semi = val.find( ';', start );
			std::string boot = ( semi == std::string::npos ) ? val.substr( start ) : val.substr( start, semi - start );

			// trim trailing whitespace
			while ( !boot.empty() && ( boot.back() == ' ' || boot.back() == '\t' ) )
			{ 
				boot.pop_back();
			}

			return boot;
		};

		size_t pos = up.find( boot2 );
		if ( pos != std::string::npos ) 
		{
			auto parsed = parse_after_equals( pos );
			if ( parsed )
			{ 
				return *parsed;
			}
		}

		pos = up.find( boot1 );
		if ( pos != std::string::npos ) 
		{
			auto parsed = parse_after_equals( pos );
			if ( parsed ) 
			{
				return *parsed;
			}
		}

		// Fallback: search for any occurrence of CDROM0:\ and take up to ';'
		const std::string needle = "CDROM0:\\";
		pos = up.find( needle );
		if ( pos == std::string::npos )
		{
			throw std::runtime_error( "BOOT line not found in SYSTEM.CNF" );
		}

		size_t start = pos + needle.size( );
		size_t end = up.find( ';', start );
		std::string boot = ( end == std::string::npos ) ? up.substr( start ) : up.substr( start, end - start );

		// Trim trailing whitespace
		auto last = boot.find_last_not_of( " \t" );
		if ( last == std::string::npos )
		{
			boot.clear();
		}
		else
		{
			boot.erase( last + 1 );
		}

		// If the path contains directories, keep only the final component
		auto sep = boot.find_last_of("\\/");
		if ( sep != std::string::npos && sep + 1 < boot.size() )
		{
			boot = boot.substr( sep + 1 );
		}

		// Strip surrounding quotes if present
		if ( !boot.empty() && ( boot.front() == '"' || boot.front() == '\'' ) )
		{
			boot.erase( 0, 1 );
		}
		if ( !boot.empty() && ( boot.back() == '"' || boot.back() == '\'' ) )
		{
			boot.pop_back();
		}

		return boot;
	}
};
