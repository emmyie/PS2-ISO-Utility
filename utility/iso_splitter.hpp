#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "crc32.hpp"
#include "iso_util.hpp"
#include "string_util.hpp"
#include "ul_cfg_mgr.hpp"

struct split_options
{
	std::string m_title;
	std::optional<bool> m_force_dvd = std::nullopt;
	std::size_t m_part_size = 1ull << 30;
	std::size_t m_block_size = 4ull * 1024 * 1024;
	bool m_verify_sizes = true;
	bool m_dry_run = false;
	std::function<void( std::uint64_t, std::uint64_t )> m_on_progress_fn;
};

class iso_splitter
{
public:
	struct split_result
	{
		std::string m_boot_id;
		std::string m_title;
		std::string m_crc;
		bool m_is_dvd = true;
		std::uint8_t m_parts = 0;
		std::vector<std::filesystem::path> m_written_files;
		std::filesystem::path m_ul_cfg_path;
		std::uint64_t m_iso_size = 0;
	};

	static split_result split( const std::filesystem::path& iso_path,
		const std::filesystem::path& out_dir,
		const split_options& opt = {} )
	{
		using namespace std;
		namespace fs = std::filesystem;

		if ( !fs::exists( iso_path ) )
		{
			throw runtime_error( "ISO not found: " + iso_path.string( ) );
		}

		if ( !fs::exists( out_dir ) )
		{
			fs::create_directories( out_dir );
		}

		const string system_cnf = iso_util::read_system_cnf( iso_path );
		string boot_id = iso_util::extract_boot_id( system_cnf );

		string title;
		if ( !opt.m_title.empty( ) )
		{
			title = str_util::sanitize_title( opt.m_title );
		}
		else
		{
			try
			{
				title = str_util::sanitize_title( fs::path( iso_path ).stem( ).string( ) );
			}
			catch ( ... )
			{
				title = {};
			}
			if ( title.empty( ) )
			{
				title = boot_id;
			}
		}

		if ( title.size( ) > 32 )
		{
			title.resize( 32 );
		}

		auto crc_opt = crc::crc32_hash( title );
		if ( !crc_opt )
		{
			throw runtime_error( "Title exceeds 32 chars after sanitize; cannot compute OPL CRC" );
		}

		uint32_t crc_val = *crc_opt;
		char crcHex[ 9 ];
		std::snprintf( crcHex, sizeof( crcHex ), "%08X", crc_val );
		string crc_hex = crcHex;

		const uint64_t iso_size = fs::file_size( iso_path );
		bool is_dvd = opt.m_force_dvd.has_value( )
			? *opt.m_force_dvd
			: ( iso_size > ( 700ull * 1024ull * 1024ull ) );

		const uint64_t part_size = static_cast< uint64_t >( std::max<std::size_t>( 1, opt.m_part_size ) );
		const uint64_t parts64 = ( iso_size + part_size - 1 ) / part_size;

		if ( parts64 > 255 )
		{
			throw runtime_error( "ISO too large: would require > 255 parts" );
		}

		if ( parts64 == 0 )
		{
			throw runtime_error( "Computed zero parts unexpectedly" );
		}

		const uint8_t parts = static_cast< uint8_t >( parts64 );

		const string base = string( "ul." ) + crc_hex + "." + boot_id + ".";

		split_result result;
		result.m_boot_id = boot_id;
		result.m_title = title;
		result.m_crc = crc_hex;
		result.m_is_dvd = is_dvd;
		result.m_parts = parts;
		result.m_iso_size = iso_size;
		result.m_ul_cfg_path = out_dir / "ul.cfg";

		if ( opt.m_dry_run )
		{
			return result;
		}

		vector<char> buffer( std::max<std::size_t>( 1, opt.m_block_size ) );
		ifstream in( iso_path, ios::binary );
		if ( !in )
		{
			throw runtime_error( "Failed to open ISO for reading: " + iso_path.string( ) );
		}

		uint64_t processed = 0;
		for ( uint8_t idx = 0; idx < parts; ++idx )
		{
			char suffix[ 4 ];
			std::snprintf( suffix, sizeof( suffix ), "%02u", static_cast< unsigned >( idx ) );
			fs::path part_path = out_dir / ( base + suffix );
			ofstream out( part_path, ios::binary );
			if ( !out )
			{
				throw runtime_error( "Cannot create part file: " + part_path.string( ) );
			}

			uint64_t bytes_in_this_part = 0;
			while ( bytes_in_this_part < part_size && processed < iso_size )
			{
				const uint64_t remaining_in_iso = iso_size - processed;
				const uint64_t remaining_in_part = part_size - bytes_in_this_part;
				const size_t to_read = static_cast< size_t >(
					std::min<uint64_t>( buffer.size( ), std::min( remaining_in_iso, remaining_in_part ) ) );

				in.read( buffer.data( ), static_cast< std::streamsize >( to_read ) );
				const std::streamsize got = in.gcount( );
				if ( got <= 0 )
				{
					break;
				}

				out.write( buffer.data( ), got );
				if ( !out.good( ) )
				{
					throw runtime_error( "Write error: " + part_path.string( ) );
				}

				processed += static_cast< uint64_t >( got );
				bytes_in_this_part += static_cast< uint64_t >( got );

				if ( opt.m_on_progress_fn )
				{
					opt.m_on_progress_fn( processed, iso_size );
				}
			}

			out.close( );
			result.m_written_files.push_back( part_path );
		}

		if ( opt.m_verify_sizes )
		{
			uint64_t sum = 0;
			for ( const auto& p : result.m_written_files )
			{
				sum += std::filesystem::file_size( p );
			}

			if ( sum != iso_size )
			{
				throw runtime_error( "Split size mismatch: sum(parts) != ISO size" );
			}
		}

		{
			ul_cfg_manager cfg{ out_dir };

			uint64_t size_mb = iso_size / ( 1024 * 1024 );
			if ( size_mb > 0xFFFFFFFFULL )
			{
				throw runtime_error( "ISO too large: size exceeds uint32_t limit" );
			}

			const auto rec = ul_cfg_record::make( title, boot_id, parts, is_dvd, static_cast< uint32_t >( size_mb ) );
			cfg.append( rec );
		}

		return result;
	}
};