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
			if ( off + len > dir.size( ) ) break;

			const uint8_t* rec = &dir[ off ];
			uint8_t id_len = rec[ 32 ];
			std::string ident( reinterpret_cast< const char* >( rec ) + 33, id_len );

			std::string ident_upper;
			ident_upper.resize( ident.size( ) );
			std::transform( ident.begin( ), ident.end( ), ident_upper.begin( ),
				[ ] ( unsigned char c )
			{
				return std::toupper( c );
			} );

			if ( id_len > 1 && ( ident_upper == "SYSTEM.CNF" || ident_upper == "SYSTEM.CNF;1" ) )
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
		std::string s = system_cnf;
		s.erase( std::remove( s.begin( ), s.end( ), '\n' ), s.end( ) );
		s.erase( std::remove( s.begin( ), s.end( ), '\r' ), s.end( ) );

		std::string up( s.size( ), '\0' );
		std::transform( s.begin( ), s.end( ), up.begin( ), [ ] ( unsigned char c )
		{
			return std::toupper( c );
		} );

		const std::string needle = "CDROM0:\\";
		size_t pos = up.find( needle );
		if ( pos == std::string::npos )
		{
			throw std::runtime_error( "BOOT line not found in SYSTEM.CNF" );
		}

		size_t start = pos + needle.size( );
		size_t end = up.find( ';', start );
		std::string boot = ( end == std::string::npos ) ? up.substr( start ) : up.substr( start, end - start );

		boot.erase( boot.find_last_not_of( " \t" ) + 1 );
		return boot;
	}
};
